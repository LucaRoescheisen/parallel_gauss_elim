#ifndef PGAUSS_ELIM_FORK_H
#define PGAUSS_ELIM_FORK_H

#include "process_handler.h"

void eliminate_my_rows(double A[], double b[], int n, int j,
                       int id, int num_procs, int layout);
void back_substitute(double A[], double b[], double x[], int n);
int pgaussElimFork(double A[], double b[], double x[], int n, Process_Pool* pool);

#endif
