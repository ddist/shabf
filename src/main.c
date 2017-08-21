#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <openssl/sha.h>
#include "thread_pool.h"

#define HASH_SIZE 64
#define VOCABULARY_SIZE 26
typedef unsigned char byte;
typedef unsigned long long int u64int;

int matches(byte *a, byte *b) {
    for(int i=0; i < HASH_SIZE/2; i++) {
        if(a[i] != b[i]) return 0;
    }
    return 1;
}

byte* stob(const char *s) {
    byte* hash = malloc(HASH_SIZE/2);
    char two[3];
    two[2] = 0;
    for (int i = 0; i < HASH_SIZE/2; i++) {
        two[0] = s[i*2];
        two[1] = s[i*2+1];
        hash[i] = (byte)strtol(two, 0, HASH_SIZE/4);
    }
    return hash;
}

void printResult(byte* password, byte* hash) {
    char sPass[6];
    memcpy(sPass, password, 5);
    sPass[5] = 0;
    printf("%s => ", sPass);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        printf("%02x", hash[i]);
    printf("\n");
}

byte *secretPassword;

/* Function to execute for each queued job. Receives a struct as argument. */
void compute_frame(int *a) {
    byte password[5] = { 97 + *a };
    for (password[1] = 97; password[1] < 123; password[1]++) {
        for (password[2] = 97; password[2] < 123; password[2]++) {
            for (password[3] = 97; password[3] < 123; password[3]++) {
                for (password[4] = 97; password[4] < 123; password[4]++) {
                    byte *hasherino = SHA256(password, 5, 0);
                    if (matches(secretPassword, hasherino)){
                        printResult(password, hasherino);
                    }
                }
            }
        }
    }
    return;
}

int main(int argc, char **argv) {

    if(argc < 3) {
        printf("Wrong number of arguments. Expected 3, read %i\n", argc);
        return 1;
    }

    /* MEASURE RUNTIME */
    struct timespec start_t, end_t;
    clock_gettime(CLOCK_MONOTONIC, &start_t);

    /* READ COMMAND LINE ARGUMENTS */
    int n_workers = atoi(argv[1]);
    FILE *fp;
    char* line = NULL;
    size_t len = 0;

    /* READ HASHES FILE */
    fp = fopen(argv[2], "r");
    if(fp == NULL) {
        printf("Can't open file at %s\n", argv[2]);
        return 1;
    }
    getline(&line, &len, fp);
    int password_len = atoi(line);
    getline(&line, &len, fp);
    int n_hashes = atoi(line);

    char **hashes = malloc(n_hashes * sizeof(char*));

    getline(&line, &len, fp);
    for (int i = 0; i < n_hashes; i++) {
        getline(&line, &len, fp);
        hashes[i] = malloc(HASH_SIZE * sizeof(char));
        memcpy(hashes[i], line, HASH_SIZE);
    }

    /* CREATE THREAD POOL */
    thread_pool_p t_pool;
    create_thread_pool((unsigned int) n_workers, &t_pool);

    secretPassword = stob(hashes[0]);

    /* QUEUE JOBS */
    for (int i = 0; i < VOCABULARY_SIZE; i++) {
        int *a = malloc(sizeof(int));
        *a = i;
        add_job_to_thread_pool((void*)compute_frame, a, FREE_ARG, t_pool);
    }
    thread_pool_wait(t_pool);

    /* SHOW RESULTS */
    clock_gettime(CLOCK_MONOTONIC, &end_t);
    double runtime = (end_t.tv_sec - start_t.tv_sec);
    runtime += (end_t.tv_nsec - start_t.tv_nsec) / 1000000000.0;

    printf("Runtime: %lf\n", runtime);

    return 0;
}