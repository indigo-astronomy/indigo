// Copyright (c) 2026 Rumen G. Bogdanovski
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

// Predictive PEC (Periodic Error Correction) guiding algorithm.
//
// This is a dependency-free pure-C implementation of a Gaussian Process based
// guiding algorithm, originally developed at the Max Planck Institute for
// Intelligent Systems (Edgar D. Klenske, Stephan Wenninger, Raffi Enficiaud)
// and distributed under a BSD license. The original implementation relies on
// the Eigen linear algebra library and its FFT module; here the minimal subset
// of dense linear algebra (matrix products, LDLT/Cholesky factorisation and
// solve) and a radix-2 FFT needed by the algorithm are re-implemented from
// scratch so that no external dependency is introduced.
//
// The algorithm learns the mount's periodic gear (worm) error online with a
// Gaussian process and predicts the error one exposure into the future, so the
// correction is applied before the error is actually observed. This removes the
// reaction lag inherent in purely reactive controllers and is therefore only
// meaningful on the RA axis.
//
// version history
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

#ifndef indigo_gp_guider_h
#define indigo_gp_guider_h

#include <stdbool.h>

#if defined(INDIGO_WINDOWS)
#if defined(INDIGO_WINDOWS_DLL)
#define INDIGO_EXTERN __declspec(dllexport)
#else
#define INDIGO_EXTERN __declspec(dllimport)
#endif
#else
#define INDIGO_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque Predictive PEC guider state. One instance per guiding session. */
typedef struct indigo_gp_guider indigo_gp_guider;

/* Create a guider initialised with default parameters. Returns NULL on
   allocation failure. */
INDIGO_EXTERN indigo_gp_guider *indigo_gp_guider_create(void);

/* Destroy a guider created by indigo_gp_guider_create(). NULL is ignored. */
INDIGO_EXTERN void indigo_gp_guider_destroy(indigo_gp_guider *guider);

/* Clear all collected data, keeping the learned worm period estimate (the
   period is a mechanical constant, so it stays valid across a meridian flip or
   re-slew). Used internally when a guiding session restarts. */
INDIGO_EXTERN void indigo_gp_guider_reset(indigo_gp_guider *guider);

/* Full reset to the prior, including the learned worm period and the
   cross-session retain state. Use this for an explicit user-requested "forget
   everything and relearn from scratch" (e.g. when the period locked onto a
   wrong value). User tuning set via indigo_gp_guider_set_parameters() is not
   affected in practice because callers re-apply it each frame. */
INDIGO_EXTERN void indigo_gp_guider_reset_model(indigo_gp_guider *guider);

/* Begin a guiding session, deciding whether to retain the learned model or
   reset it. The model is retained only if guiding resumes on the
   same side of pier and the worm has rotated less than retain_pct percent of a
   period since guiding stopped (accounting for both the elapsed time and any RA
   slew between targets). Otherwise the model is reset and re-learned.
     current_ra    - mount RA in hours, or NaN if unknown
     side_of_pier  - -1 (east) / +1 (west), or 0 if unknown
     retain_pct    - max worm rotation to still retain, e.g. 40.0
   Call indigo_gp_guider_session_stop() when guiding stops so the elapsed time
   can be measured. Returns true if the model was retained, false if reset. */
INDIGO_EXTERN bool indigo_gp_guider_session_start(indigo_gp_guider *guider, double current_ra, int side_of_pier, double retain_pct);

/* Record that guiding has stopped (timestamp used by the next session_start to
   decide whether the model can be retained). */
INDIGO_EXTERN void indigo_gp_guider_session_stop(indigo_gp_guider *guider);

/* Set the user-facing tuning parameters.
     control_gain    - reactive (proportional) gain, 0..1 (default 0.6)
     prediction_gain - amount of GP prediction blended in, 0..1 (default 0.5)
     min_move        - minimum drift magnitude (pixels) to act on (default 0.2)
     compute_period  - if true the worm period is estimated online via FFT
     period_length   - worm period length in seconds; <= 0 keeps the current
                       value (default 200). */
INDIGO_EXTERN void indigo_gp_guider_set_parameters(indigo_gp_guider *guider, double control_gain, double prediction_gain, double min_move, bool compute_period, double period_length);

/* Current worm period length estimate in seconds. */
INDIGO_EXTERN double indigo_gp_guider_get_period_length(const indigo_gp_guider *guider);

/* Learning progress of the predictive model, 0.0 .. 1.0. This is a data
   sufficiency measure (not a goodness-of-fit): it ramps from 0 to 1 as the
   model collects up to min_periods_for_inference worm periods of guiding data,
   which is the same ramp that blends the GP prediction into the correction. At
   1.0 the correction is driven fully by the learned periodic-error curve. */
INDIGO_EXTERN double indigo_gp_guider_get_learning_progress(const indigo_gp_guider *guider);

/* Compute the RA correction for one guiding frame.
     drift     - measured RA star displacement in pixels for this frame
     snr       - guide star signal to noise ratio (used for the measurement
                 variance); pass <= 0 if unknown
     time_step - guide cycle time in seconds (exposure + delay)
   Returns the correction in pixels following the same sign convention as the
   other indigo_guider_*_response() functions (i.e. a value that, applied to the
   mount, counteracts the drift). */
INDIGO_EXTERN double indigo_gp_guider_response(indigo_gp_guider *guider, double drift, double snr, double time_step);

/* Predictive control for a frame where no measurement could be made (lost star
   / dark frame). Returns the correction in pixels, same sign convention as
   indigo_gp_guider_response(). */
INDIGO_EXTERN double indigo_gp_guider_deduce(indigo_gp_guider *guider, double time_step);

/* Notify the guider that a dither command was issued. While dithering the
   guider stops trusting measurements and relies on predictions to keep its FFT
   and GP consistent.
     amount_px - dither amount applied to the RA axis in pixels
     ra_rate   - RA guide rate in pixels per second (amount_px / ra_rate gives
                 the equivalent offset in gear time) */
INDIGO_EXTERN void indigo_gp_guider_dithered(indigo_gp_guider *guider, double amount_px, double ra_rate);

/* Notify the guider that dithering has settled. */
INDIGO_EXTERN void indigo_gp_guider_dither_settle_done(indigo_gp_guider *guider, bool success);

#ifdef __cplusplus
}
#endif

#endif /* indigo_gp_guider_h */
