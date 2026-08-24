#ifndef THREAD_HANDLER_H
#define THREAD_HANDLER_H
#include <stdint.h>
#include <pthread.h>
#define CORES 2
#define PROCESSES 2

enum io { READ, WRITE };

typedef struct{
  pthread_t* threads;
  int num_threads;
  int kill;
  int* chunks;
  pthread_mutex_t lock;
} Thread_Pool;


Thread_Pool* generate_pool(int num_threads);


#endif
