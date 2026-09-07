#include "../include/process_handler.h"
#include "../include/pgaussElimFork.h"
#include <unistd.h>
#include <stdlib.h>

void eliminate_my_rows(double A[], double b[], int n, int j, int id, int num_procs, int layout) {
  double pivot = A[j * n + j];

  for(int i = j + 1; i < n; i++) {
    if(!owns_row(i, id, num_procs, layout, n)) {
      continue;
    }

    double f = A[i * n + j] / pivot;

    for(int k = 0; k < n; k++) {
      A[i * n + k] -= f * A[j * n + k];
    }
    b[i] -= f * b[j];
  }
}

static int broadcast_pivot(double A[], double b[], int n, int j, Process_Pool* pool) {
  int owner = owner_of_row(j, pool->num_procs, pool->layout, n);
  double* row = &A[j * n];
  size_t bytes = (size_t)n * sizeof(double);

  if(pool->is_parent) {
    if(owner != 0) {
      if(chread(pool->to_parent[owner], (char*)row,   bytes,         pool->chunk_size) < 0) {
        return -1;
      }
      
      if(chread(pool->to_parent[owner], (char*)&b[j], sizeof(double), pool->chunk_size) < 0) {
        return -1;
      }
    }

    for(int w = 1; w < pool->num_procs; w++) {
      if(w == owner) { 
        continue;
      }
      
      if(chwrite(pool->to_child[w], (const char*)row,   bytes,         pool->chunk_size) < 0) { 
        return -1;
      }

      if(chwrite(pool->to_child[w], (const char*)&b[j], sizeof(double), pool->chunk_size) < 0) {
        return -1;
      }
    }
  }
  else if(owner == pool->my_id) {
    if(chwrite(pool->my_write, (const char*)row,   bytes,         pool->chunk_size) < 0) { 
      return -1; 
    }

    if(chwrite(pool->my_write, (const char*)&b[j], sizeof(double), pool->chunk_size) < 0) {
      return -1; 
    }
  }
  else {
    if(chread(pool->my_read, (char*)row,   bytes,         pool->chunk_size) < 0) {
      return -1; 
    }

    if(chread(pool->my_read, (char*)&b[j], sizeof(double), pool->chunk_size) < 0) {
      return -1;
    }
  }

  return 0;
}

void back_substitute(double A[], double b[], double x[], int n) {
  x[n - 1] = b[n - 1] / A[(n - 1) * n + (n - 1)];

  for(int i = n - 2; i >= 0; i--) {
    double sum = 0;

    for(int j = i + 1; j < n; j++) {
      sum += A[i * n + j] * x[j];
    }
    x[i] = (b[i] - sum) / A[i * n + i];
  }
}

static int send_my_rows(double A[], double b[], int n, Process_Pool* pool) {
  size_t bytes = (size_t)n * sizeof(double);

  for(int i = 0; i < n; i++) {
    if(!owns_row(i, pool->my_id, pool->num_procs, pool->layout, n)) { 
      continue; 
    }

    if(chwrite(pool->my_write, (const char*)&A[i * n], bytes,          pool->chunk_size) < 0) { 
    return -1; 
  }
    if(chwrite(pool->my_write, (const char*)&b[i],     sizeof(double), pool->chunk_size) < 0) {
      return -1; 
    }
  }

  return 0;
}

static int collect_rows(double A[], double b[], int n, Process_Pool* pool) {
  size_t bytes = (size_t)n * sizeof(double);

  for(int w = 1; w < pool->num_procs; w++) {
    for(int i = 0; i < n; i++) {
      if(!owns_row(i, w, pool->num_procs, pool->layout, n)) {
        continue;
      }

      if(chread(pool->to_parent[w], (char*)&A[i * n], bytes,          pool->chunk_size) < 0) {
        return -1; 
      }
      if(chread(pool->to_parent[w], (char*)&b[i],     sizeof(double), pool->chunk_size) < 0) { 
        return -1; 
      }
    }
  }

  return 0;
}

int pgaussElimFork(double A[], double b[], double x[], int n, Process_Pool* pool) {
  for(int j = 0; j < n - 1; j++) {
    if(broadcast_pivot(A, b, n, j, pool) < 0) { 
      return -1; 
    }

    eliminate_my_rows(A, b, n, j, pool->my_id, pool->num_procs, pool->layout);
  }

  if(!pool->is_parent) {

    if(send_my_rows(A, b, n, pool) < 0) {
      _exit(EXIT_FAILURE); 
    }
    _exit(EXIT_SUCCESS);
  }

  if(collect_rows(A, b, n, pool) < 0) {
    return -1; 
  }

  back_substitute(A, b, x, n);

  return 0;
}
