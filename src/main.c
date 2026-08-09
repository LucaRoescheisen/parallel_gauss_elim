#include "../include/generate_matrix.h"
#include "../include/gaussElim.h"
#include <stdlib.h>
#include <stdio.h>




int main()  {

  int step_size = 1;
  FDMResult r = generate_matrix(1);
  
  double* x = malloc(r.N * sizeof(double));
  if(x == nullptr) return 0 ;
  
  


  gaussElim(r.fdm_matrix, r.sol_matrix, x, r.N);
  
  for(int i = 0 ; i < r.N; i++){
    printf("%f,", x[i]);
  }
  printf("Finished");

   return 0;
}
