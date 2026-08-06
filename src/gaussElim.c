#include <math.h>

void gaussElim(double A[], double b[], double x[], int n)
{
	int i, j, k, maxIndex;
	double pivot, f, sum, max, temp;

	for (j = 0; j < n - 1; j++) {

		// elimination stage
		pivot = A[j * n + j];

		for (i = j + 1; i < n; i++) {
			f = A[i * n + j] / pivot;
			
			for (k = 0; k < n; k++) {
				A[i * n + k] -= f * A[j * n + k];
			}	
			b[i] -= f * b[j];
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
