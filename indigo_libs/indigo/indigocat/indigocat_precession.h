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

#ifndef __PRECESSION_H
#define __PRECESSION_H

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

#include <indigo/indigocat/indigocat_transform.h>

#ifdef __cplusplus
extern "C" {
#endif

INDIGO_EXTERN equatorial_coords_s indigocat_precess(const equatorial_coords_s *c0, const double eq0, const double eq1);
INDIGO_EXTERN equatorial_coords_s indigocat_apply_proper_motion(const equatorial_coords_s *c0, double pmra, double pmdec, double eq0, double eq1);

INDIGO_EXTERN void indigocat_j2k_to_jnow_pm(double *ra, double *dec, double pmra, double pmdec);
INDIGO_EXTERN void indigocat_jnow_to_j2k(double *ra, double *dec);

#ifdef __cplusplus
};
#endif

#endif
