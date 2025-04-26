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

/** Least Squares Polynomial Fitting
 \file indigo_polynomial_fit.c
 */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <indigo/indigo_bus.h>

#include <indigo/indigo_polynomial_fit.h>

/**
 Computes polynomial coefficients that best fit a set
 of input points using Least Squares.

 Returns 0 if success.

 Based on polyfit() by Nate Domin
*/
int indigo_polynomial_fit(int point_count, double *x_values, double *y_values, int coefficient_count, double *polynomial_coefficients) {
	enum {max_order = 20};

	long double B[max_order + 1] = {0.0f};
	long double P[((max_order + 1) * 2) + 1] = {0.0f};
  long double A[(max_order + 1)*2*(max_order + 1)] = {0.0f};

	const int order = coefficient_count - 1;
	long double x, y, powx;
	int ii, jj, kk;

	// This method requires that the point_count > (order+1)
	if (point_count <= order) {
		return -1;
	}

	if (order > max_order) {
		return -1;
	}

	// Identify the column vector
	for (ii = 0; ii < point_count; ii++) {
		x = x_values[ii];
		y = y_values[ii];
		powx = 1;

		for (jj = 0; jj < (order + 1); jj++) {
			B[jj] = B[jj] + (y * powx);
			powx = powx * x;
		}
	}

	// Initialize the PowX array
	P[0] = point_count;

	// Compute the sum of the Powers of X
	for (ii = 0; ii < point_count; ii++) {
		x = x_values[ii];
		powx = x_values[ii];

		for (jj = 1; jj < ((2 * (order + 1)) + 1); jj++) {
			P[jj] = P[jj] + powx;
			powx = powx * x;
		}
	}

	// Initialize the reduction matrix
	for (ii = 0; ii < (order + 1); ii++) {
		for (jj = 0; jj < (order + 1); jj++) {
			A[(ii * (2 * (order + 1))) + jj] = P[ii+jj];
		}
		A[(ii*(2 * (order + 1))) + (ii + (order + 1))] = 1;
	}

	// Move the Identity matrix portion of the redux matrix
	// to the left side (find the inverse of the left side
	// of the redux matrix
	for (ii = 0; ii < (order + 1); ii++) {
		x = A[(ii * (2 * (order + 1))) + ii];
		if (x != 0) {
			for (kk = 0; kk < (2 * (order + 1)); kk++) {
				A[(ii * (2 * (order + 1))) + kk] = A[(ii * (2 * (order + 1))) + kk] / x;
			}

			for (jj = 0; jj < (order + 1); jj++) {
				if ((jj - ii) != 0) {
					y = A[(jj * (2 * (order + 1))) + ii];
					for (kk = 0; kk < (2 * (order + 1)); kk++) {
						A[(jj * (2 * (order + 1))) + kk] = A[(jj * (2 * (order + 1))) + kk] - y * A[(ii * (2 * (order + 1))) + kk];
					}
				}
			}
		} else {
			// Cannot work with singular matrices
			return -1;
		}
	}

	// Calculate and Identify the polynomial_coefficients
	for (ii = 0; ii < (order + 1); ii++) {
		for (jj = 0; jj < (order + 1); jj++) {
			x = 0;
			for (kk = 0; kk < (order + 1); kk++) {
				x = x + (A[(ii * (2 * (order + 1))) + (kk + (order + 1))] * B[kk]);
			}
			polynomial_coefficients[ii] = x;
		}
	}

	return 0;
}

/**
 Calculate polynomial value for a given x
*/
double indigo_polynomial_value(double x, int coefficient_count, double *polynomial_coefficients) {
	double t = 1;
	double value = 0;
	for (int i = 0; i < coefficient_count; i++) {
		value += polynomial_coefficients[i] * t;
		t *= x;
	}
	return value;
}

/**
 Calculate polynomial derivative (returns a polynomial of order - 1)
*/
void indigo_polynomial_derivative(int coefficient_count, double *polynomial_coefficients, double *derivative_polynomial_coefficients) {
	for (int i = 1; i < coefficient_count; i++) {
		derivative_polynomial_coefficients[i - 1] = i * polynomial_coefficients[i];
	}
}

/**
 Calculate polynomial extremums (minimums and maximums)
 NOTE: Works for polynomials of order 2 and 3 only
*/
int indigo_polynomial_extremums(int coefficient_count, double *polynomial_coefficients, double *extremums) {
	double *derivative = indigo_safe_malloc(coefficient_count - 1);
	indigo_polynomial_derivative(coefficient_count, polynomial_coefficients, derivative);
	if (coefficient_count == 3) {  // order = 2
		extremums[0] = -derivative[0] / derivative[1];
		indigo_safe_free(derivative);
		return 0;
	}
	if (coefficient_count == 4) {  // order = 3
		double det = sqrt(derivative[1] * derivative[1] - 4 * derivative[0] * derivative[2]);
		extremums[0] = (-derivative[1] + det)/(2*derivative[2]);
		extremums[1] = (-derivative[1] - det)/(2*derivative[2]);
		indigo_safe_free(derivative);
		return 0;
	}
	indigo_safe_free(derivative);
	return 1;
}

/**
 Calculate polynomial minimum at x in [min_x, max_x]
*/
double indigo_polynomial_min_x(int coefficient_count, double* polynomial_coefficients, double low, double high, double tolerance) {
	while (high - low > tolerance) {
		double mid1 = low + (high - low) / 3;
		double mid2 = high - (high - low) / 3;

		double f_mid1 = indigo_polynomial_value(mid1, coefficient_count, polynomial_coefficients);
		double f_mid2 = indigo_polynomial_value(mid2, coefficient_count, polynomial_coefficients);

		if (f_mid1 < f_mid2) {
			high = mid2;
		} else {
			low = mid1;
		}
	}
	return low + (high - low) / 2;
}

/**
 Reprent polynomial as string
*/
void indigo_polynomial_string(int coefficient_count, double *polynomial_coefficients, char *polynomial_string) {
	char *p = polynomial_string;
	if (coefficient_count > 0) {
		p += sprintf(p, "y =");
	}
	for (int i = coefficient_count-1; i >= 0; i--) {
		if (i > 1) {
			p += sprintf(p, " %+.15e*x^%d", polynomial_coefficients[i], i);
		} else if (i == 1) {
			p += sprintf(p, " %+.15e*x", polynomial_coefficients[i]);
		} else {
			p += sprintf(p, " %+.15e", polynomial_coefficients[i]);
		}
	}
}
