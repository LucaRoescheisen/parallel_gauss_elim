#ifndef PGAUSS_ELIM_H
#define PGAUSS_ELIM_H
#include "../include/thread_handler.h"
void pgaussElim(double A[], double b[], double x[], int n, Thread_Pool* pool);

#endif
