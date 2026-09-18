#include "../include/generate_matrix.h"
#include "../include/pgaussElim.h"
#include "../include/thread_handler.h"
#include <stdlib.h>
#include <stdio.h>


void free_ptrs(Thread_Pool* pool, FDMResult* r, double* x);
void save_to_dat(FDMResult r, double* x);

int main(int argc, char *argv[])  {
  //Defaults
  float step_size = 0.5;
  int num_threads = 2;
  float track_a_voltage = 5;

  if (argc > 3) {
    char* endptr;
    step_size = strtof(argv[1], &endptr);
    num_threads = atoi(argv[2]);
    if(num_threads <= 0) { fprintf(stderr, "Error: number of threads must be positive and > than 0\n"); }
    track_a_voltage = strtof(argv[3], & endptr);
  }

  FDMResult r = generate_matrix(step_size, track_a_voltage);
  if(r.fdm_matrix == nullptr || r.sol_matrix == nullptr || r.N == 0){
    fprintf(stderr, "generate matrix failed\n");
    return 1;
  }
  double* x = malloc(r.N * sizeof(double));
  if(x == nullptr) {
    fprintf(stderr, "x malloc failed\n");
    return 1;}
  
  
  Thread_Pool* pool = nullptr;
  pool = generate_pool(num_threads);
  pgaussElim(r.fdm_matrix, r.sol_matrix, x, r.N, pool);

  free_ptrs(pool, &r, x);
  return 0;
}





void save_to_dat(FDMResult r, double* x) {
  FILE *fp = fopen("output.dat", "w");
  for(int i = 0; i < r.h; i++){
    for(int j = 0; j < r.w; j++){
      if(i> 0 && i < r.h - 1 && j > 0 && j < r.w -1){
        int idx = (i-1) * (r.w-2) + (j-1);
        fprintf(fp, "%i %i %f\n", i,j, x[idx]);
      }
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
}


void free_ptrs(Thread_Pool* pool, FDMResult* r, double* x){
  free(pool->chunks);
  free(pool->threads);
  free(pool);
  free(r->fdm_matrix);
  free(r->sol_matrix);
  free(x);
}
