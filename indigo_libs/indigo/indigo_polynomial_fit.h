// Copyright (c) 2022 Rumen G.Bogdanvski
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
 \file indigo_polynomial_fit.h
 */

#ifndef indigo_polynomial_fit_h
#define indigo_polynomial_fit_h

#ifdef __cplusplus
extern "C" {
#endif

/**
 Perofrm polynomial fit of the given data with polynomial of a given order
*/
void indigo_polynomial_fit(const int count, double data[count][2], int order, double polynomial[order + 1]);

/**
 Calculate polynomial value for a given x
*/
double indigo_polynomial_value(double x, int order, double polynomial[order + 1]);

/**
 Calculate polynomial derivative (returns a polinomial of order - 1)
*/
void indigo_polynomial_derivative(int order, double polynomial[order + 1], double derivative[order]);

/**
 Calculate polynomial extremums (minimums and maximums)
 NOTE: Works for polynomials of order 2 and 3 only
*/
int indigo_polynomial_extremums(int order, double polynomial[order + 1], double extremum[order-1]);

#ifdef __cplusplus
}
#endif

#endif /* indigo_polynomial_fit_h */
