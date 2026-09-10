#include "../include/generate_matrix.h"
#include "../include/process_handler.h"
#include "../include/pgaussElimFork.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char* prog) {
  fprintf(stderr, "usage: %s <step_size> <num_procs> <track_a_voltage>"
                  " [chunk_size] [cyclic|block] [pipe|socket]\n", prog);
}

int main(int argc, char *argv[]) {
  int layout = CYCLIC;
  int chunk_size = DEFAULT_CHUNK_SIZE;
  int ipc_type = PIPE;

  if(argc < 4) { 
    usage(argv[0]); return 1;
  }

  float step_size = strtof(argv[1], nullptr);
  int num_procs = atoi(argv[2]);
  float track_a_voltage = strtof(argv[3], nullptr);

  if(argc > 4) { 
    chunk_size = atoi(argv[4]); 
  }
  if(argc > 5) {
    if(strcmp(argv[5], "block") == 0) { layout = BLOCK; }
    else if(strcmp(argv[5], "cyclic") == 0) { layout = CYCLIC; }
    else { fprintf(stderr, "unknown layout '%s'\n", argv[5]); usage(argv[0]); return 1; }
  }
  if(argc > 6) {
    if(strcmp(argv[6], "socket") == 0) { ipc_type = SOCKETPAIR; }
    else if(strcmp(argv[6], "pipe") == 0) { ipc_type = PIPE; }
    else { fprintf(stderr, "unknown ipc type '%s'\n", argv[6]); usage(argv[0]); return 1; }
  }

  if(num_procs <= 0) { 
    fprintf(stderr, "num_procs must be > 0\n"); return 1;
  }
  if(chunk_size <= 0) { 
    fprintf(stderr, "chunk size must be > 0\n"); return 1;
  }

  FDMResult r = generate_matrix(step_size, track_a_voltage);
  if(r.fdm_matrix == nullptr || r.sol_matrix == nullptr || r.N == 0) {
    fprintf(stderr, "generate matrix failed\n");
    return 1;
  }

  double* x = malloc(r.N * sizeof(double));
  if(x == nullptr) { 
    return 1; 
  }

  Process_Pool* pool = generate_process_pool(num_procs, chunk_size, layout, ipc_type);
  if(pool == nullptr) {
    fprintf(stderr, "could not create process pool\n");
    free(r.fdm_matrix); free(r.sol_matrix); free(x);
    return 1;
  }

  if(pgaussElimFork(r.fdm_matrix, r.sol_matrix, x, r.N, pool) < 0) {
    fprintf(stderr, "parallel solve failed\n");
    reap_process_pool(pool); free_process_pool(pool);
    free(r.fdm_matrix); free(r.sol_matrix); free(x);
    return 1;
  }

  reap_process_pool(pool);
  free_process_pool(pool);

  FILE* fp = fopen("output.dat", "w");
  if(fp == nullptr) {
    perror("output.dat");
    free(r.fdm_matrix); free(r.sol_matrix); free(x);
    return 1;
  }

  for(int i = 0; i < r.h; i++) {
    for(int j = 0; j < r.w; j++) {
      if(i > 0 && i < r.h - 1 && j > 0 && j < r.w - 1) {
        int idx = (i - 1) * (r.w - 2) + (j - 1);
        fprintf(fp, "%i %i %f\n", i, j, x[idx]);
      }
    }
    fprintf(fp, "\n");
  }
  fclose(fp);

  free(r.fdm_matrix);
  free(r.sol_matrix);
  free(x);

  return 0;
}
