// Copyright (c) 2024 Rumen G.Bogdanvski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// version history
// 2.0 by Rumen G.Bogdanovski <rumenastro@gmail.com>
// Based on polyfit() by Henry M. Forson

/** Least Squares Polynomial Fitting
 \file indigo_polynomial_fit.c
 */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <indigo/indigo_polynomial_fit.h>

typedef struct matrix_s {
	int	rows;
	int	cols;
	double *data;
} matrix_t;

#define MATRIX_VALUE_PTR( pA, row, col )  (&(((pA)->data)[ (row * (pA)->cols) + col]))

static matrix_t * transpose_matrix( matrix_t *pMat ) {
    matrix_t *rVal = (matrix_t *) calloc(1, sizeof(matrix_t));
    rVal->data = (double *) calloc( pMat->rows * pMat->cols, sizeof( double ));
    rVal->cols = pMat->rows;
    rVal->rows = pMat->cols;
    for( int r = 0; r < rVal->rows; r++ ) {
        for( int c = 0; c < rVal->cols; c++ ) {
            *MATRIX_VALUE_PTR(rVal, r, c) = *MATRIX_VALUE_PTR(pMat, c, r);
        }
    }
    return rVal;
}

static matrix_t * product_matrix( matrix_t *pLeft, matrix_t *pRight ) {
    matrix_t *rVal = NULL;
    if (NULL == pLeft || NULL == pRight || pLeft->cols != pRight->rows) {
        return NULL;
    } else {
        // Allocate the product matrix.
        rVal = (matrix_t *) calloc(1, sizeof(matrix_t));
        rVal->rows = pLeft->rows;
        rVal->cols = pRight->cols;
        rVal->data = (double *) calloc( rVal->rows * rVal->cols, sizeof( double ));

        // Initialize the product matrix contents:
        // product[i,j] = sum{k = 0 .. (pLeft->cols - 1)} (pLeft[i,k] * pRight[ k, j])
        for (int i = 0; i < rVal->rows; i++) {
            for (int j = 0; j < rVal->cols; j++) {
                for (int k = 0; k < pLeft->cols; k++) {
                    *MATRIX_VALUE_PTR(rVal, i, j) += (*MATRIX_VALUE_PTR(pLeft, i, k)) * (*MATRIX_VALUE_PTR(pRight, k, j));
                }
            }
        }
    }

    return rVal;
}

static void destroy_matrix(matrix_t *pMat) {
    if (NULL != pMat) {
        if (NULL != pMat->data) {
            free(pMat->data);
        }
        free(pMat);
    }
}

static matrix_t *create_matrix(int rows, int cols) {
    matrix_t *rVal = (matrix_t *)calloc(1, sizeof(matrix_t));
    if (NULL != rVal) {
        rVal->rows = rows;
        rVal->cols = cols;
        rVal->data = (double *)calloc(rows * cols, sizeof(double ));
        if (NULL == rVal->data) {
            free(rVal);
            rVal = NULL;
        }
    }

    return rVal;
}

/**
 Computes polynomial coefficients that best fit a set of input points using Least Squares.

 The degree of the fitted polynomial is coefficient_count-1.

 Returns   0 if success,
          -1 if passed a NULL pointer,
          -2 if (point_count <= degree),
          -3 if unable to allocate memory,
          -4 if unable to solve equations.
*/
int indigo_polynomial_fit(int point_count, double *x_values, double *y_values, int coefficient_count, double *polynomial_coefficients) {
    int result = 0;
    int degree = coefficient_count - 1;

    if (NULL == x_values || NULL == y_values || NULL == polynomial_coefficients) {
        return -1;
    }

    if (point_count < coefficient_count) {
        return -2;
    }

    matrix_t *pMatA = create_matrix(point_count, coefficient_count);
    if (NULL == pMatA) {
        return -3;
    }

    for (int r = 0; r < point_count; r++) {
        for (int c = 0; c < coefficient_count; c++) {
            *(MATRIX_VALUE_PTR(pMatA, r, c)) = pow((x_values[r]), (double)(degree - c));
        }
    }

    matrix_t *pMatB = create_matrix(point_count, 1);
    if (NULL == pMatB) {
        return -3;
    }

    for (int r = 0; r < point_count; r++) {
        *(MATRIX_VALUE_PTR(pMatB, r, 0)) = y_values[r];
    }

    matrix_t * pMatAT = transpose_matrix(pMatA);
    if (NULL == pMatAT) {
        return -3;
    }

    matrix_t *pMatATA = product_matrix(pMatAT, pMatA);
    if(NULL == pMatATA) {
        return -3;
    }

    matrix_t *pMatATB = product_matrix(pMatAT, pMatB);
    if(NULL == pMatATB) {
        return -3;
    }

    // Now we need to solve the system of linear equations,
    // (AT)Ax = (AT)b for "x", the coefficients of the polynomial.

    for (int c = 0; c < pMatATA->cols; c++) {
        int pr = c;     // pr is the pivot row.
        double prVal = *MATRIX_VALUE_PTR(pMatATA, pr, c);
        // If it's zero, we can't solve the equations.
        if (0.0 == prVal) {
            result = -4;
            break;
        }
        for (int r = 0; r < pMatATA->rows; r++) {
            if (r != pr) {
                double targetRowVal = *MATRIX_VALUE_PTR(pMatATA, r, c);
                double factor = targetRowVal / prVal;
                for (int c2 = 0; c2 < pMatATA->cols; c2++) {
                    *MATRIX_VALUE_PTR(pMatATA, r, c2) -=  *MATRIX_VALUE_PTR(pMatATA, pr, c2) * factor;
                }
                *MATRIX_VALUE_PTR(pMatATB, r, 0) -=  *MATRIX_VALUE_PTR(pMatATB, pr, 0) * factor;
            }
        }
    }
    for (int c = 0; c < pMatATA->cols; c++) {
        int pr = c;
        // now, pr is the pivot row.
        double prVal = *MATRIX_VALUE_PTR(pMatATA, pr, c);
        *MATRIX_VALUE_PTR(pMatATA, pr, c) /= prVal;
        *MATRIX_VALUE_PTR(pMatATB, pr, 0) /= prVal;
    }

    for(int i = 0; i < coefficient_count; i++) {
        polynomial_coefficients[i] = *MATRIX_VALUE_PTR(pMatATB, i, 0);
	}

    destroy_matrix(pMatATB);
    destroy_matrix(pMatATA);
    destroy_matrix(pMatAT);
    destroy_matrix(pMatA);
    destroy_matrix(pMatB);
    return result;
}

/**
 Calculate polynomial value for a given x
*/
double indigo_polynomial_value2(double x, int coefficient_count, double *polynomial_coefficients) {
	double value = 0;
	for(int i = 0; i < coefficient_count; i++) {
		value += pow(x,i) * polynomial_coefficients[i];
	}
	return value;
}

double indigo_polynomial_value(double x, int coefficient_count, double *polynomial_coefficients) {
  double t = 1;
  double value = 0;
  for (int i = coefficient_count-1; i >=0 ; i--) {
    value += polynomial_coefficients[i] * t;
    t *= x;
  }
  return value;
}

double indigo_polynomial_min_at(int coefficient_count, double *polynomial_coefficients, double min_x, double max_x, double *min_x_out) {
	double min = indigo_polynomial_value(min_x, coefficient_count, polynomial_coefficients);
	for (double i = min_x; i < max_x; i+=.001) {
		double v = indigo_polynomial_value(i, coefficient_count, polynomial_coefficients);
		if (v < min) {
			min = v;
			*min_x_out = i;
		}
	}
}

/**
 Calculate polynomial derivative (returns a polynomial of order - 1)
*/
void indigo_polynomial_derivative(int coefficient_count, double *polynomial_coefficients, double *derivative_coefficients) {
	for (int i = 1; i < coefficient_count-1; i++) {
		derivative_coefficients[i] = (i + 1) * polynomial_coefficients[i + 1];
	}
	derivative_coefficients[0] = polynomial_coefficients[1];
}

/**
 Calculate polynomial extremums (minimums and maximums)
 NOTE: Works for polynomials of order 2 and 3 only
*/
int indigo_polynomial_extremums(int coefficient_count, double *polynomial_coefficients, double *extremums) {
	double derivative[coefficient_count - 1];
	indigo_polynomial_derivative(coefficient_count, polynomial_coefficients, derivative);
	if (coefficient_count == 3) {  // order = 2
		extremums[0] = -derivative[0]/derivative[1];
		return 0;
	}
	if (coefficient_count == 4) {  // order = 3
		double det = sqrt(derivative[1] * derivative[1] - 4 * derivative[2] * derivative[0]);
		extremums[0] = (-derivative[1] + det)/(2*derivative[2]);
		extremums[1] = (-derivative[1] - det)/(2*derivative[2]);
		return 0;
	}
	return 1;
}

/*
double regress(double x) {
  double terms[] = {
     3.9172952110050202e+004,
    -5.6042486439263905e+000,
     2.0046695195353507e-004
};
  
  size_t csz = sizeof terms / sizeof *terms;
  
  double t = 1;
  double r = 0;
  for (int i = 0; i < csz;i++) {
    r += terms[i] * t;
    t *= x;
  }
  return r;
}

int main() {

(13825, 9.701900)
(13900, 5.995183)
(13975, 4.809935)
(14050, 5.789945)
(14125, 9.701667)
(14200, 14.546792)

	double x[] = {13825,    13900,     13975,   14050,   14125, 14200};
	double y[] = {9.701900, 5.995183, 4.809935, 5.789945, 9.701667, 14.546792};
	double c[3];
	indigo_polynomial_fit(6, x, y, 3, c);
	for (int i = 0; i < 3; i++) {
		printf("%f\n", c[i]);
	}

	

	printf("min_x = %f min = %f\n", min_x, min);
	
	return 0;
}
*/