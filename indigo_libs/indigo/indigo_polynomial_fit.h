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
 \file indigo_polynomial_fit.h
 */

#ifndef indigo_polynomial_fit_h
#define indigo_polynomial_fit_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 Computes polynomial coefficients that best fit a set
 of input points using Least Squares.

 Returns 0 if success.
*/
int indigo_polynomial_fit(int point_count, double *x_values, double *y_values, int coefficient_count, double *polynomial_coefficients);

/**
 Calculate polynomial value for a given x
*/
double indigo_polynomial_value(double x, int coefficient_count, double *polynomial_coefficients);

/**
 Calculate polynomial derivative (returns a polinomial of degree - 1)
*/
void indigo_polynomial_derivative(int coefficient_count, double *polynomial_coefficients, double *derivative_coefficients);

/**
 Calculate polynomial extremums (minimums and maximums)
 NOTE: Works for polynomials of order 2 and 3 only
*/
int indigo_polynomial_extremums(int coefficient_count, double *polynomial_coefficients, double *extremums);

#ifdef __cplusplus
}
#endif

#endif /* indigo_polynomial_fit_h */
