#include <stdbool.h>
#include <stdlib.h>
#include "../include/generate_matrix.h"
#include <stdio.h>
#include <math.h>

//Function Prototypes
bool validate_step_size(float step_size);
void classify_nodes(int** matrix, struct Dimensions dimensions);
void populate_fdm(int** ref_matrix, double* fdm_matrix, double* sol_matrix, struct Dimensions dimensions, int N);



FDMResult generate_matrix(const float step_size) {

  FDMResult result = {.fdm_matrix = nullptr, .sol_matrix=nullptr};
  if(!validate_step_size(step_size)) { 
    printf("Invalid step size");
    return result; 
  }
  struct Dimensions dimensions  = {0};
  //Generate Matrix given step_size and parameters
  
  //Board dimensions
   dimensions.w = (int)roundf(WIDTH/step_size);
   dimensions.h =  (int)roundf(HEIGHT/step_size);
  //PCB placement
   dimensions.w_pcb =  (int)roundf(PCB_WIDTH/step_size);
   dimensions.h_pcb =  (int)roundf(PCB_HEIGHT/step_size);
   dimensions.x_pcb =  (int)roundf(PCB_START_X/step_size);
   dimensions.y_pcb =  (int)roundf(PCB_START_Y/step_size);
  //Tracks placements
   dimensions.w_track   =  (int)roundf(TRACK_WIDTH/step_size);
   dimensions.x_track_a =  (int)roundf(TRACK_A_START_X/step_size);
   dimensions.x_track_b =  (int)roundf(TRACK_B_START_X/step_size);
   dimensions.y_track_a =  (int)roundf(TRACK_A_Y/step_size);
   dimensions.y_track_b =  (int)roundf(TRACK_B_Y/step_size);

  result.N = (dimensions.h-2) *(dimensions.w-2);
  //Generate 2D Matrix
  int** ref_matrix = malloc(dimensions.h * sizeof(int*));
  if(ref_matrix == nullptr) { return result; }

  for(int i = 0; i < dimensions.h; i++){
    ref_matrix[i] = malloc(dimensions.w * sizeof(int));
    if(ref_matrix[i] == nullptr) { return result; }
  }
  classify_nodes(ref_matrix, dimensions);

  //DEBUG PRINT :) 
  for(int i = 0; i < dimensions.h; i++){
    for(int j = 0; j < dimensions.w; j++){
      printf("%i, ", ref_matrix[i][j]);
    }
    printf("\n");
  }

  
  //Second run generate FDM Matrix
  result.fdm_matrix = calloc(result.N * result.N, sizeof(double));
  if(result.fdm_matrix == nullptr) { return result; }



  result.sol_matrix = calloc(result.N, sizeof(double));
  if(result.sol_matrix == nullptr) { return result; }
  for(int i = 0; i < result.N; i++) {result.sol_matrix[i] = 0;}


  populate_fdm(ref_matrix, result.fdm_matrix, result.sol_matrix, dimensions, result.N);
//So far it seems to work :)
  result.w = dimensions.w;
  result.h = dimensions.h;

  for(int i=0; i < dimensions.h; i++){
    free(ref_matrix[i]);
  }
  free(ref_matrix);


  return result;
}



void classify_nodes(int** matrix, struct Dimensions dimensions) {

  //Populate the 2D Matrix;
  for(int row = 0; row < dimensions.h; row++){
    for(int col = 0; col < dimensions.w; col++){
      if(row == 0 || col == 0 || row == dimensions.h-1 || col == dimensions.w-1){ //GND
        matrix[row][col] = GND;
      }
      else if(row == dimensions.y_track_a && col >= (dimensions.x_track_a) && col < (dimensions.x_track_a + dimensions.w_track)) {
        matrix[row][col] = TRACK_A;
      }
      else if(row == dimensions.y_track_b && col >= (dimensions.x_track_b) && col < (dimensions.x_track_b + dimensions.w_track - 1)) {
        matrix[row][col] = TRACK_B;
      }
      else if(row == dimensions.y_pcb && (col > dimensions.x_pcb && col < (dimensions.x_pcb + dimensions.w_pcb))){
        matrix[row][col] = INTERFACE;
      }
      else if(row == (dimensions.y_pcb + dimensions.h_pcb ) && (col > dimensions.x_pcb && col < (dimensions.x_pcb + dimensions.w_pcb))){
        matrix[row][col] = INTERFACE;
      }
      else if((row > dimensions.y_pcb &&  row < dimensions.y_pcb + dimensions.h_pcb) && (col == dimensions.x_pcb)){
        matrix[row][col] = INTERFACE;
      }
      else if(row > dimensions.y_pcb &&  (row < dimensions.y_pcb + dimensions.h_pcb) && col == (dimensions.x_pcb + dimensions.w_pcb)){
        matrix[row][col] = INTERFACE;
      }
      else if((row == dimensions.y_pcb && col == dimensions.x_pcb) || (row == dimensions.y_pcb && col == (dimensions.x_pcb + dimensions.w_pcb)) ||
             (row == (dimensions.y_pcb + dimensions.h_pcb) && col == dimensions.x_pcb) || (row == (dimensions.y_pcb + dimensions.h_pcb) && col == (dimensions.x_pcb + dimensions.w_pcb))){
        matrix[row][col] = INTERFACE_CORNER;
      }
      else if(row > dimensions.y_pcb && row < (dimensions.y_pcb + dimensions.h_pcb) && //Interface
              col >= dimensions.x_pcb && col < (dimensions.x_pcb + dimensions.w_pcb)){
        matrix[row][col] = PCB;
      }
      else { matrix[row][col] = FREE_SPACE; }
    }
  }
}

static bool divides(float value, float step_size) {
  float rem = fmodf(value, step_size);
  return rem < STEP_EPSILON || fabsf(rem-step_size) < STEP_EPSILON;
}


bool validate_step_size(const float step_size) {
  return step_size <= MAX_STEP
        && divides(WIDTH , step_size)
        && divides(HEIGHT , step_size)
        && divides(PCB_START_X , step_size)
        && divides(PCB_START_Y , step_size)
        && divides((PCB_START_X + PCB_WIDTH), step_size) 
        && divides((PCB_START_Y + PCB_HEIGHT) , step_size)
        && divides(TRACK_B_Y , step_size)
        && divides(TRACK_A_Y , step_size)
        && divides(TRACK_A_START_X , step_size)
        && divides(TRACK_B_START_X , step_size)
        && divides((TRACK_A_START_X + TRACK_WIDTH) , step_size)
        && divides((TRACK_B_START_X + TRACK_WIDTH) , step_size);
}





void populate_fdm(int** ref_matrix, double* fdm_matrix, double* sol_matrix, struct Dimensions dimensions, int N){
  constexpr int dirs[4][2] = {
    {-1,0},
    {1, 0},
    {0, 1},
    {0,-1}
  };
  int fdm_row = 0;
  for(int row = 1; row < dimensions.h - 1; row++){ //We skip outer GND edge
    for(int col = 1; col < dimensions.w - 1; col++){
      int current_node = ref_matrix[row][col];
      if(current_node == FREE_SPACE || current_node == PCB) {fdm_matrix[fdm_row* N +fdm_row] = -4;}
      else if(current_node == TRACK_A) {
        sol_matrix[fdm_row] = -TRACK_A_VOLTAGE;
        fdm_matrix[fdm_row * N + fdm_row] = 1;
      }
      else if(current_node == TRACK_B) {
        sol_matrix[fdm_row] = -TRACK_B_VOLTAGE;
        fdm_matrix[fdm_row * N + fdm_row] = 1;
      }
      else if(current_node == INTERFACE) { fdm_matrix[fdm_row * N + fdm_row] = -4*(PERM_1 + PERM_2);}
      else if(current_node == INTERFACE_CORNER) { fdm_matrix[fdm_row * N + fdm_row] = (-6*PERM_1 + 2*PERM_2);}
      for(int i = 0; i < 4; i++){
        int x = row + dirs[i][0];
        int y= col + dirs[i][1];
        int temp = ref_matrix[x][y]; // stop redundant calculations
        int neighbour = (x-1)*(dimensions.w-2) + (y-1);
        if((current_node == FREE_SPACE || current_node == PCB) && temp != GND) {
          fdm_matrix[fdm_row * N + neighbour] = 1;
        }
        else if(current_node == INTERFACE || current_node == INTERFACE_CORNER) {
          if(temp == FREE_SPACE) {
            fdm_matrix[fdm_row * N + neighbour] = 2 * PERM_1;  
          }
          else if(temp == PCB) {
            fdm_matrix[fdm_row * N + neighbour] = 2 * PERM_2;
          }
          else if(temp == INTERFACE || temp == INTERFACE_CORNER) {
            fdm_matrix[fdm_row * N + neighbour] = PERM_1 + PERM_2;
          }
          else if(temp == TRACK_A){
            fdm_matrix[fdm_row * N + neighbour] = (PERM_1 + PERM_2); //NOTE not too sure about the addition here
            //In the lab the +5V was on the perm_1 area, so I assume if the track is on the interface we add both perms???
          }
          else if(temp == TRACK_B){
            fdm_matrix[fdm_row * N + neighbour] = (PERM_1 + PERM_2);
          }
        }
      }
      fdm_row++;
    }
  }
  
  
}
