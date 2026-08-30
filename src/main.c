#include "../include/generate_matrix.h"
#include "../include/pgaussElim.h"
#include "../include/thread_handler.h"
#include <stdlib.h>
#include <stdio.h>




int main(int argc, char *argv[])  {
  float step_size = 0.5; //default
  int num_threads = 2; //default
  

  if (argc > 2) {
    char* endptr;
    step_size = strtof(argv[1], &endptr);
    num_threads = atoi(argv[2]);
    if(num_threads <= 0) { fprintf(stderr, "Error: number of threads must be positive and > than 0"); }
  }
  printf("%f", step_size);

  //Tip dont use a stepsize less than this, otherwise calloc will use approx 70GB of RAM which is non existent and
  //CPU cores start swap-thrashing :(

  FDMResult r = generate_matrix(step_size);
  if(r.fdm_matrix == nullptr || r.sol_matrix == nullptr || r.N == 0){
    fprintf(stderr, "generate matrix failed");
    return 1;
  }
  double* x = malloc(r.N * sizeof(double));
  if(x == nullptr) return 0 ;
  
  
  Thread_Pool* pool = nullptr;
  pool = generate_pool(num_threads);


  pgaussElim(r.fdm_matrix, r.sol_matrix, x, r.N, pool);
  
  for(int i = 0 ; i < r.N; i++){
    printf("%f,", x[i]);
  }
  printf("Finished");
  
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

  
  free(pool->chunks);
  free(pool->threads);
  free(pool);
  free(r.fdm_matrix);
  free(r.sol_matrix);
  free(x);
   return 0;
}
