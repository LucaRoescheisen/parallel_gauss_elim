#ifndef GENERATE_MATRIX_H
#define GENERATE_MATRIX_H

#define WIDTH 40 //cm
#define HEIGHT 25 //cm
#define TRACK_A_VOLTAGE 5 //V
#define TRACK_B_VOLTAGE 3.3 //V
#define MAX_STEP  5 //cm

//PCB TOPOLOGY (cm)
#define PCB_WIDTH 30
#define PCB_HEIGHT 5
  //Assuming top left is 0,0
#define PCB_START_X 5
#define PCB_START_Y 15 

//Track Positions
#define TRACK_WIDTH   10
#define TRACK_A_START_X   15
#define TRACK_B_START_X   15
#define TRACK_A_Y    15
#define TRACK_B_Y    20

//Permitivities
#define PERM_1 1
#define PERM_2 4.8

#define STEP_EPSILON 1e-3f

typedef enum {
  FREE_SPACE,
  GND,
  PCB,
  TRACK_A,
  TRACK_B,
  INTERFACE_CORNER,
  INTERFACE
} NodeType;

struct Dimensions {
   int w;
   int h;

  //PCB placement
   int w_pcb;
   int h_pcb;
   int x_pcb;
   int y_pcb;
  //Tracks placements
   int w_track;
   int x_track_a;
   int x_track_b;
   int y_track_a;
   int y_track_b;
};

typedef struct {
  double* fdm_matrix;
  double* sol_matrix;
  int N;
  int w, h;
} FDMResult;

FDMResult generate_matrix(float step_size);
#endif
