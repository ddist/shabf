#include "thread_pool.h"

int create_queue_node(void (*function)(void*), void* arg, unsigned int flag, queue_node_p* node) {
	(*node) = malloc(sizeof(queue_node_t));
	if(!(*node)) return -1;
	(*node)->destroy_flag = flag;
	(*node)->prev = NULL;
	(*node)->next = NULL;
	(*node)->function = function;
	(*node)->arg = arg;
	return 0;
}

void destroy_queue_node(queue_node_p node) {
	if(node) {
		if(node->destroy_flag == FREE_ARG || node->destroy_flag == FREE_ALL) free(node->arg);
		if(node->destroy_flag == FREE_FUNC || node->destroy_flag == FREE_ALL) free(node->function);
		free(node);
	}
}

int create_sync_queue(sync_queue_p* queue) {
	(*queue) = malloc(sizeof(sync_queue_t));
	if(!(*queue)) return -1;
	(*queue)->front = NULL;
	(*queue)->back = NULL;
	(*queue)->size = 0;

	if(pthread_mutex_init(&((*queue)->rw_mutex), NULL)) return -1;
	if(pthread_mutex_init(&((*queue)->cond_mutex), NULL)) return -1;
	if(pthread_cond_init(&((*queue)->cond), NULL)) return -1;
	return 0;
}

void queue_push(queue_node_p node, sync_queue_p queue) {
	pthread_mutex_lock(&(queue->rw_mutex));
	if(queue->front == NULL) {
		queue->front = node;
		queue->back = node;
	} else {
		node->next = queue->back;
		queue->back->prev = node;
		queue->back = node;
	}
	queue->size++;
	pthread_mutex_unlock(&(queue->rw_mutex));
}

queue_node_p queue_pull(sync_queue_p queue) {
	queue_node_p node = NULL;
	pthread_mutex_lock(&(queue->rw_mutex));
	if(queue->size > 0) {
		node = queue->front;
		if(queue->size == 1) {
			queue->front = NULL;
			queue->back = NULL;
		} else {
			queue->front = queue->front->prev;
			queue->front->next = NULL;
		}
		queue->size--;
	}
	pthread_mutex_unlock(&(queue->rw_mutex));
	return node;
}

void destroy_sync_queue(sync_queue_p queue) {
	if(queue) {
		while(queue->size)
			free(queue_pull(queue));
		free(queue);
	}
}

void thread_fn(thread_pool_p pool) {
	pthread_mutex_lock(&(pool->rw_mutex));
	pool->threads_alive++;
	pthread_mutex_unlock(&(pool->rw_mutex));

	while(1) {
		pthread_mutex_lock(&(pool->queue->cond_mutex));
		while(pool->queue->size == 0) {
			pthread_cond_wait(&(pool->queue->cond), &(pool->queue->cond_mutex));
		}
		queue_node_p job = queue_pull(pool->queue);
		pthread_mutex_unlock(&(pool->queue->cond_mutex));

		pthread_mutex_lock(&(pool->rw_mutex));
		pool->threads_working++;
		pthread_mutex_unlock(&(pool->rw_mutex));
		if(job) {
			(*job->function)(job->arg);
		}

		pthread_mutex_lock(&(pool->rw_mutex));
		pool->threads_working--;
		if(!pool->threads_working && !pool->queue->size) {
			pthread_cond_signal(&(pool->cond));
		}
		pthread_mutex_unlock(&(pool->rw_mutex));
		destroy_queue_node(job);
	}
}

int create_thread_pool(unsigned int pool_size, thread_pool_p* pool) {
	(*pool) = malloc(sizeof(thread_pool_t));
	if(!(*pool)) return -1;
	if(pthread_mutex_init(&(*pool)->rw_mutex, NULL)) return -1;
	if(pthread_mutex_init(&(*pool)->cond_mutex, NULL)) return -1;
	if(pthread_cond_init(&(*pool)->cond, NULL)) return -1;
	if(create_sync_queue(&((*pool)->queue))) return -1;
	(*pool)->threads = malloc(pool_size * sizeof(pthread_t));
	if(!((*pool)->threads)) {
		destroy_sync_queue((*pool)->queue);
		free(*pool);
		return -1;
	}
	(*pool)->pool_size = pool_size;
	(*pool)->threads_alive = 0;
	(*pool)->threads_working = 0;
	for(int i=0; i < pool_size; i++) {
		pthread_create(&((*pool)->threads[i]), NULL, (void *)thread_fn, (*pool));
		pthread_detach((*pool)->threads[i]);
	}

	while((*pool)->threads_alive < pool_size) {};
	return 0;
}

void destroy_thread_pool(thread_pool_p pool) {
	if(pool) {
		for(int i=0; i < pool->pool_size ; i++) {
			pthread_cancel(pool->threads[i]);
		}
		destroy_sync_queue(pool->queue);
		free(pool->threads);
		free(pool);
	}
}

void thread_pool_wait(thread_pool_p pool) {
	pthread_mutex_lock(&(pool->rw_mutex));
	while(pool->queue->size || pool->threads_working) {
		pthread_cond_wait(&(pool->cond), &(pool->rw_mutex));
	}
	pthread_mutex_unlock(&(pool->rw_mutex));
}

void add_job_to_thread_pool(void (*function)(void*), void* arg, unsigned int flag, thread_pool_p pool) {

	queue_node_p job;
	create_queue_node(function, arg, flag, &job);
	pthread_mutex_lock(&(pool->queue->cond_mutex));
	queue_push(job, pool->queue);
	pthread_mutex_unlock(&(pool->queue->cond_mutex));
	pthread_cond_signal(&(pool->queue->cond));
}