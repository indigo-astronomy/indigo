// Copyright (c) 2026 CloudMakers, s. r. o.
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
// 3.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** C interface of the libcamera bridge
 \file indigo_ccd_libcamera_bridge.h

 libcamera is a C++ library, the generated driver is C. This interface is the
 whole boundary between them: indigo_ccd_libcamera_bridge.cpp implements it on
 top of libcamera, the hardware-free tests implement it with a fake camera.

 Every camera delivers RAW sensor frames only, converted to 16-bit little
 endian samples that keep the native sensor values (0 to 2^bit_depth - 1).
 Calls for one camera are serialized by the caller; lcam_read() may be called
 while libcamera completes requests on its own thread.
 */

#ifndef indigo_ccd_libcamera_bridge_h
#define indigo_ccd_libcamera_bridge_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCAM_ID_SIZE					256
#define LCAM_MODEL_SIZE				64
#define LCAM_MAX_CAMERAS			8
#define LCAM_MAX_MODES				16
#define LCAM_ERROR_SIZE				256

/** Camera found by lcam_list().
 */
typedef struct {
	char id[LCAM_ID_SIZE];						///< libcamera camera id, stable for one boot
	char model[LCAM_MODEL_SIZE];			///< sensor model as reported by libcamera, e.g. "ov5647"
} lcam_camera_info;

/** RAW sensor mode.
 */
typedef struct {
	unsigned width, height;						///< frame size in pixels
	unsigned bit_depth;								///< significant bits per sample
	unsigned binning;									///< binning factor relative to the full pixel array, 1 for full resolution
	double exposure_min, exposure_max;	///< seconds, the limits differ between modes
} lcam_mode;

/** Properties of an opened camera.
 */
typedef struct {
	unsigned pixel_array_width, pixel_array_height;
	double pixel_width, pixel_height;	///< micrometers, 0 when unknown
	char bayer[8];										///< "RGGB", "BGGR", "GRBG" or "GBRG", empty for a mono sensor
	double exposure_min, exposure_max;	///< seconds, limits of the first mode
	double gain_min, gain_max, gain_default;
	int mode_count;
	lcam_mode modes[LCAM_MAX_MODES];		///< ordered from the largest frame down
} lcam_properties;

/** Frame returned by lcam_read().
 */
typedef struct {
	unsigned width, height;						///< size of the copied rectangle
	unsigned bit_depth;
	double exposure;									///< seconds, from the frame metadata
	double gain;											///< analogue gain, from the frame metadata
} lcam_frame;

typedef struct lcam_camera lcam_camera;

/** Start the camera manager. Returns false when libcamera is not available.
 */
extern bool lcam_initialize(void);

/** Stop the camera manager. All cameras must be closed.
 */
extern void lcam_terminate(void);

/** Copy up to size cameras into list and return their number.
 */
extern int lcam_list(lcam_camera_info *list, int size);

/** Acquire and inspect a camera. Returns NULL and an error message on failure.
 */
extern lcam_camera *lcam_open(const char *id, lcam_properties *properties, char *error, size_t error_size);

/** Stop any capture and release the camera.
 */
extern void lcam_close(lcam_camera *camera);

/** Start capturing RAW frames of the given mode with the given exposure (seconds) and analogue gain.
		A single capture delivers one frame, a streaming capture delivers frames until lcam_stop().
 */
extern bool lcam_start(lcam_camera *camera, int mode, double exposure, double gain, bool streaming, char *error, size_t error_size);

/** Copy the rectangle (left, top, width, height) of the next completed frame into buffer as 16-bit samples.
		Returns 1 when a frame was copied, 0 when no frame is ready yet and -1 on failure (error is filled).
 */
extern int lcam_read(lcam_camera *camera, uint16_t *buffer, size_t buffer_size, unsigned left, unsigned top, unsigned width, unsigned height, lcam_frame *frame, char *error, size_t error_size);

/** Stop capturing and drop pending frames. Safe to call when not capturing.
 */
extern void lcam_stop(lcam_camera *camera);

#ifdef __cplusplus
}
#endif

#endif /* indigo_ccd_libcamera_bridge_h */
