#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define FREE_ALL  1
#define FREE_ARG  2
#define FREE_FUNC 3
#define FREE_NONE 4

typedef struct _queue_node {
	struct _queue_node *prev;
	struct _queue_node *next;
	unsigned int destroy_flag;
	void (*function)(void *);
	void *arg;
} queue_node_t;
typedef queue_node_t* queue_node_p;

int create_queue_node(void (*function)(void*), void*, unsigned int, queue_node_p*);

void destroy_queue_node(queue_node_p);

typedef struct _sync_queue {
	size_t size;
	queue_node_p back;
	queue_node_p front;
	pthread_mutex_t rw_mutex;
	pthread_mutex_t cond_mutex;
	pthread_cond_t cond;
} sync_queue_t;
typedef sync_queue_t* sync_queue_p;

int create_sync_queue(sync_queue_p*);

void queue_push(queue_node_p, sync_queue_p);

queue_node_p queue_pull(sync_queue_p);

void destroy_sync_queue(sync_queue_p);

typedef struct _thread_pool {
	size_t pool_size, threads_alive, threads_working;
	sync_queue_p queue;
	pthread_t *threads;
	pthread_mutex_t rw_mutex;
	pthread_mutex_t cond_mutex;
	pthread_cond_t cond;
} thread_pool_t;
typedef thread_pool_t* thread_pool_p;

void thread_fn(thread_pool_p);

int create_thread_pool(unsigned int, thread_pool_p*);

void destroy_thread_pool(thread_pool_p);

void thread_pool_wait(thread_pool_p);

void add_job_to_thread_pool(void (*function)(void*), void*, unsigned int, thread_pool_p);

#endif