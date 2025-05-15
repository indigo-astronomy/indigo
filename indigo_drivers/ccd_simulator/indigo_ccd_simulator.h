// Copyright (c) 2016-2025 CloudMakers, s. r. o.
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
// 2.0 by Peter Polakovic <peter.polakovic@cloudmakers.eu>
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO CCD Simulator driver
 \file indigo_ccd_simulator.h
 */

#ifndef ccd_simulator_h
#define ccd_simulator_h

#include <indigo/indigo_driver.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_ao_driver.h>
#include <indigo/indigo_wheel_driver.h>
#include <indigo/indigo_focuser_driver.h>

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

#define CCD_SIMULATOR_IMAGER_CAMERA_NAME	"CCD Imager Simulator"
#define CCD_SIMULATOR_WHEEL_NAME					"CCD Imager Simulator (wheel)"
#define CCD_SIMULATOR_FOCUSER_NAME				"CCD Imager Simulator (focuser)"

#define CCD_SIMULATOR_GUIDER_CAMERA_NAME	"CCD Guider Simulator"
#define CCD_SIMULATOR_GUIDER_NAME					"CCD Guider Simulator (guider)"
#define CCD_SIMULATOR_AO_NAME							"CCD Guider Simulator (AO)"

#define CCD_SIMULATOR_BAHTINOV_CAMERA_NAME	"CCD Bahtinov Mask Simulator"

#define CCD_SIMULATOR_DSLR_NAME						"DSLR Simulator"

#define CCD_SIMULATOR_FILE_NAME						"CCD File Simulator"

//#define CCD_SIMULATOR_BAHTINOV_IMAGE

//#define ENABLE_BACKLASH_PROPERTY

// USE_DISK_BLUR is used to simulate disk blur effect
// if not defined then gaussian blur is used
//#define USE_DISK_BLUR

// related to embedded image size, don't touch!
#define IMAGER_WIDTH        		1600
#define IMAGER_HEIGHT       		1200
#define BAHTINOV_WIDTH        	500
#define BAHTINOV_HEIGHT       	500
#define BAHTINOV_MAX_STEPS      15
//#define BAHTINOV_ASYMETRIC
#define DSLR_WIDTH        			1600
#define DSLR_HEIGHT       			1200

extern unsigned short indigo_ccd_simulator_raw_image[];
extern unsigned char indigo_ccd_simulator_rgb_image[];
extern unsigned char indigo_ccd_simulator_bahtinov_image[][BAHTINOV_WIDTH * BAHTINOV_HEIGHT];

/** Create CCD Simulator device instance
 */

INDIGO_EXTERN indigo_result indigo_ccd_simulator(indigo_driver_action action, indigo_driver_info *info);

#ifdef __cplusplus
}
#endif

#endif /* ccd_simulator_h */

