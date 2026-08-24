#include <math.h>
#include "../include/thread_handler.h"
#include <unistd.h>
#include <math.h>

typedef struct {
  double *chunk_A;
  double *chunk_B;
  double *pivot_row;
  double pivot_b;
  int length;
  int n;
  int j;
  double pivot;

}Thread_Data;



void *worker(void *arg) {
  Thread_Data *td = (Thread_Data *)arg;
  double f;
  for (int i = 0; i < td->length; i++) {
		f = td->chunk_A[i * td->n + td->j] / td->pivot;
		for (int k = 0; k < td->n; k++) {
			td->chunk_A[i * td->n + k] -= f * td->pivot_row[k];
		}
		td->chunk_B[i] -= f * td->pivot_b;
	}
  return NULL;
}


void pgaussElim(double A[], double b[], double x[], int n, Thread_Pool* pool)
{
	int i, j, k, maxIndex;
	double pivot, f, sum, max, temp;

  Thread_Data td[pool->num_threads];


	for (j = 0; j < n - 1; j++) {

		// elimination stage 
    pivot = A[j * n + j];
    int active_rows = n - (j+1);
    int base = active_rows / pool->num_threads;
    int rem = active_rows % pool->num_threads;
    
    int offset = j+1;
    for (int w = 0; w < pool->num_threads; w++){
      int count = base + (w < rem ? 1 : 0);
      pool->chunks[w] = count;
      td[w].chunk_A = A + (offset * n);
      td[w].chunk_B = b + offset;
      td[w].pivot_row = A + j * n;
      td[w].pivot_b = b[j];
      td[w].j = j;
      td[w].length = count;
      td[w].pivot = pivot;
      td[w].n = n;
      offset += count;

      pthread_create(&pool->threads[w], NULL, worker, &td[w]);
    }
    
    for(int w = 0; w < pool->num_threads; w++){
      pthread_join(pool->threads[w], NULL);
    }

    
	}

  

	// back substitution stage
	x[n - 1] = b[n - 1] / A[(n - 1) * n + (n - 1)];

	for (i = n - 2; i >= 0; i--) {
		sum = 0;
		for (j = i + 1; j < n; j++) {
			sum += A[i * n + j] * x[j];
		}
		x[i] = (b[i] - sum) / A[i * n + i];
	}
}
