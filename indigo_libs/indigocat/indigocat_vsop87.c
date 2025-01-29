// Created by Rumen Bogdanovski, 2022
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
//
// Thanks to Messrs. Bretagnon and Francou for publishing planetary solution VSOP87.

#include <math.h>

#include <indigo/indigocat/indigocat_vsop87.h>

double indigocat_vsop87_calc_series(const struct vsop * data, int terms, double t) {
	double value = 0;
	int i;

	for (i=0; i<terms; i++) {
		value += data->A * cos(data->B + data->C * t);
		data++;
	}

	return value;
}

void indigocat_vsop87_to_fk5(heliocentric_coords_s * position, double JD) {
	double LL, cos_LL, sin_LL, T, delta_L, delta_B, B;

	/* get julian centuries from 2000 */
	T = (JD - 2451545.0)/ 36525.0;

	LL = position->L + (- 1.397 - 0.00031 * T) * T;
	LL = DEG2RAD * LL;
	cos_LL = cos(LL);
	sin_LL = sin(LL);
	B = DEG2RAD * position->B;

	delta_L = (-0.09033 / 3600.0) + (0.03916 / 3600.0) * (cos_LL + sin_LL) * tan (B);
	delta_B = (0.03916 / 3600.0) * (cos_LL - sin_LL);

	position->L += delta_L;
	position->B += delta_B;
}
