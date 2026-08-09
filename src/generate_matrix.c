#include <stdbool.h>
#include <stdlib.h>
#include "../include/generate_matrix.h"
#include <stdio.h>

//Function Prototypes
bool validate_step_size(int step_size);
void classify_nodes(int** matrix, struct Dimensions dimensions);
void populate_fdm(int** ref_matrix, int** fdm_matrix, double* sol_matrix, struct Dimensions dimensions);



void generate_matrix(const int step_size) {

  if(!validate_step_size(step_size)) { 
    return; 
  }
  struct Dimensions dimensions  = {0};
  //Generate Matrix given step_size and parameters
  
  //Board dimensions
   dimensions.w = (WIDTH/step_size);
   dimensions.h =  (HEIGHT/step_size);
  //PCB placement
   dimensions.w_pcb =  (PCB_WIDTH/step_size);
   dimensions.h_pcb =  (PCB_HEIGHT/step_size);
   dimensions.x_pcb =  (PCB_START_X/step_size);
   dimensions.y_pcb =  (PCB_START_Y/step_size);
  //Tracks placements
   dimensions.w_track   =  (TRACK_WIDTH/step_size);
   dimensions.x_track_a =  (TRACK_A_START_X/step_size);
   dimensions.x_track_b =  (TRACK_B_START_X/step_size);
   dimensions.y_track_a =  (TRACK_A_Y/step_size);
   dimensions.y_track_b =  (TRACK_B_Y/step_size);


  //Generate 2D Matrix
  int** ref_matrix = malloc(dimensions.h * sizeof(int*));
  if(ref_matrix == NULL) { return; }

  for(int i = 0; i < dimensions.h; i++){
    ref_matrix[i] = malloc(dimensions.w * sizeof(int));
    if(ref_matrix[i] == NULL) { return; }
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
  int** fdm_matrix = malloc(((dimensions.h - 2)* (dimensions.w-2)) * sizeof(int*));
  if(fdm_matrix == nullptr) { return; }

  for(int i = 1; i < ((dimensions.h - 2)* (dimensions.w-2)); i++){
    fdm_matrix[i] = malloc(((dimensions.h - 2)* (dimensions.w-2))  * sizeof(int));
    if(fdm_matrix[i] == nullptr) { return; }
  }

  double* sol_matrix = malloc(((dimensions.h-2) * (dimensions.w-2))*sizeof(double));
  if(sol_matrix == nullptr) { return; }
  for(int i = 0; i < ((dimensions.h-2) * (dimensions.w-2)); i++) {sol_matrix[i] = 0;}
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
        matrix[row][col] = INTERFACE_TOP;
      }
      else if(row == (dimensions.y_pcb + dimensions.h_pcb ) && (col > dimensions.x_pcb && col < (dimensions.x_pcb + dimensions.w_pcb))){
        matrix[row][col] = INTERFACE_BOTTOM;
      }
      else if((row > dimensions.y_pcb &&  row < dimensions.y_pcb + dimensions.h_pcb) && (col == dimensions.x_pcb)){
        matrix[row][col] = INTERFACE_LEFT;
      }
      else if(row > dimensions.y_pcb &&  (row < dimensions.y_pcb + dimensions.h_pcb) && col == (dimensions.x_pcb + dimensions.w_pcb)){
        matrix[row][col] = INTERFACE_RIGHT;
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


bool validate_step_size(const int step_size) {
  return step_size <= MAX_STEP
        && WIDTH % step_size == 0
        && HEIGHT % step_size == 0
        && PCB_START_X % step_size == 0
        && PCB_START_Y % step_size == 0
        && (PCB_START_X + PCB_WIDTH) % step_size == 0
        && (PCB_START_Y + PCB_HEIGHT) % step_size == 0
        && TRACK_B_Y % step_size == 0
        && TRACK_A_Y % step_size == 0
        && TRACK_A_START_X % step_size == 0
        && TRACK_B_START_X % step_size == 0
        && (TRACK_A_START_X + TRACK_WIDTH) % step_size == 0
        && (TRACK_B_START_X + TRACK_WIDTH) % step_size == 0;
}





void populate_fdm(int** ref_matrix, int** fdm_matrix, double* sol_matrix, struct Dimensions dimensions){
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
      if(current_node == FREE_SPACE || current_node == PCB) {fdm_matrix[fdm_row][row*col] = -4;}
      else if(current_node == TRACK_A) {sol_matrix[fdm_row] = -TRACK_A_VOLTAGE;}
      else if(current_node == TRACK_B) {sol_matrix[fdm_row] = -TRACK_B_VOLTAGE;}
      else if(current_node == INTERFACE_LEFT) { sol_matrix[fdm_row] = 67;} //heh 
      else if(current_node == INTERFACE_RIGHT) { sol_matrix[fdm_row] = 67;} //heh 
      else if(current_node == INTERFACE_TOP) { sol_matrix[fdm_row] = 67;} //heh 
      else if(current_node == INTERFACE_BOTTOM) { sol_matrix[fdm_row] = 67;} //heh 
      else if(current_node == INTERFACE_CORNER) { sol_matrix[fdm_row] = 67;} //heh 
      for(int i = 0; i < 3; i++){
        int x = row + dirs[i][0];
        int y= col + dirs[i][1];
        if(current_node == FREE_SPACE || current_node == PCB) {
          fdm_matrix[fdm_row][x* y] = 1;
        }
        else if(current_node == INTERFACE_TOP) {

        }

          

      }










      
      fdm_row++;
    }
  }
    
}
