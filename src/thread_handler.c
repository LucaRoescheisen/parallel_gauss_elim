#include "../include/thread_handler.h"
#include <pthread.h>
#include <stdlib.h>





Thread_Pool* generate_pool(int num_threads){
  Thread_Pool* pool = (Thread_Pool*)malloc(sizeof(Thread_Pool));
  pool->num_threads = num_threads;
  pool->kill = 0;
  pool->chunks = malloc(num_threads * sizeof(int));
  pthread_mutex_init(&(pool->lock), NULL);
  pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
   return pool;
}







