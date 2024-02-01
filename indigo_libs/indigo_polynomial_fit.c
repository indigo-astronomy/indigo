// Copyright (c) 2022 Rumen G.Bogdanovski
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

/** Chi-square Polynomial Fitting
 \file indigo_polynomial_fit.c
 */

#include<stdio.h>
#include<math.h>

/**
Function that prints the elements of a matrix row-wise
Parameters: rows(m),columns(n),matrix[m][n]
*/
static void print_matrix(int m, int n, double matrix[m][n]){
	int i,j;
	for(i=0;i<m;i++){
		for(j=0;j<n;j++){
			printf("%lf\t",matrix[i][j]);
		}
		printf("\n");
	}
}


/**
 Performs Gauss-Elimination and returns the upper triangular matrix and solution of equations
 Pass the augmented matrix (a) as the parameter, and calculate and store the upperTriangular(Gauss-Eliminated Matrix) in it.
*/
static void gauss_elimination(int m, int n, double a[m][n], double x[n-1]) {
	int i, j, k;
	for(i = 0; i < m - 1; i++) {
		for(k = i + 1; k < m; k++) {
			if(fabs(a[i][i]) < fabs(a[k][i])) {
				for(j = 0; j < n; j++) {
					double temp;
					temp = a[i][j];
					a[i][j] = a[k][j];
					a[k][j] = temp;
				}
			}
		}
		for(k = i + 1; k < m; k++) {
			double  term = a[k][i]/ a[i][i];
			for(j = 0; j < n; j++) {
				a[k][j] = a[k][j] - term * a[i][j];
			}
		}
	}
	for (i = m - 1; i >= 0; i--){
		x[i] = a[i][n - 1];
		for(j = i + 1; j < n - 1; j++) {
			x[i] = x[i] - a[i][j] * x[j];
		}
		x[i] = x[i] / a[i][i];
	}
}

/**
 calculate augmented matrix (am) for the data for a given order
*/
static void augmented_matrix(int count, double data[count][2], int order, double am[order+1][order+2]) {
	int i,j;
	double x[ 2 * order + 1];

	for(i = 0; i <= 2 * order; i++) {
		x[i] = 0;
		for(j = 0; j < count; j++) {
			x[i] = x[i] + pow(data[j][0], i);
		}
	}
	double y[order+1];
	for(i = 0; i <= order; i++) {
		y[i] = 0;
		for(j = 0; j < count; j++) {
			y[i] = y[i] + pow(data[j][0], i) * data[j][1];
		}
	}
	for(i = 0; i <= order; i++) {
		for(j = 0; j <= order; j++) {
			am[i][j] = x[i+j];
		}
	}
	for(i = 0; i <= order; i++) {
		am[i][order + 1] = y[i];
	}
}

/**
 Perofrm polynomial fit of the given data with polynomial of a given order
*/
void indigo_polynomial_fit(const int count, double data[count][2], int order, double polynomial[order + 1]) {
	double am[order + 1][order + 2];
	augmented_matrix(count, data, order, am);
	gauss_elimination(order + 1, order + 2, am, polynomial);
}

/**
 Calculate polynomial value for a given x
*/
double indigo_polynomial_value(double x, int order, double polynomial[order + 1]) {
	double value = 0;
	for(int i = 0; i <= order; i++) {
		value += pow(x,i) * polynomial[i];
	}
	return value;
}

/**
 Calculate polynomial derivative (returns a polynomial of order - 1)
*/
void indigo_polynomial_derivative(int order, double polynomial[order + 1], double derivative[order]) {
	for (int i = 1; i < order; i++) {
		derivative[i] = (i + 1) * polynomial[i + 1];
	}
	derivative[0] = polynomial[1];
}

/**
 Calculate polynomial extremums (minimums and maximums)
 NOTE: Works for polynomials of order 2 and 3 only
*/
int indigo_polynomial_extremums(int order, double polynomial[order + 1], double extremum[order-1]) {
	double derivative[order];
	indigo_polynomial_derivative(order, polynomial, derivative);
	if (order == 2) {
		extremum[0] = -derivative[0]/derivative[1];
		return 0;
	}
	if (order == 3) {
		double det = sqrt(derivative[1] * derivative[1] - 4 * derivative[2] * derivative[0]);
		extremum[0] = (-derivative[1] + det)/(2*derivative[2]);
		extremum[1] = (-derivative[1] - det)/(2*derivative[2]);
		return 0;
	}
	return 1;
}
