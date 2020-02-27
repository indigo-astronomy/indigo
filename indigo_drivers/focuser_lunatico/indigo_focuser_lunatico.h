// Copyright (C) 2020 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski

/** INDIGO Lunatico Armadillo, Platypus etc. focuser driver
 \file indigo_focuser_lunatico.h
 */

#ifndef focuser_lunatico_h
#define focuser_lunatico_h

#include <indigo/indigo_driver.h>
#include <indigo/indigo_focuser_driver.h>
#include <indigo/indigo_rotator_driver.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LUNATICO_ROTATOR_DRIVER
#define DRIVER_ENTRY_POINT       indigo_rotator_lunatico
#define DRIVER_NAME              "indigo_rotator_lunatico"
#define CONFLICTING_DRIVER       "indigo_focuser_lunatico"
#define DRIVER_INFO              "Lunatico Astronomia Rotator"
#define DEFAULT_DEVICE           TYPE_ROTATOR
#else /* FOCUSER */
#define DRIVER_ENTRY_POINT       indigo_focuser_lunatico
#define DRIVER_NAME              "indigo_focuser_lunatico"
#define CONFLICTING_DRIVER       "indigo_rotator_lunatico"
#define DRIVER_INFO              "Lunatico Astronomia Focuser"
#define DEFAULT_DEVICE           TYPE_FOCUSER
#endif

#define FOCUSER_LUNATICO_NAME    "Focuser Lunatico"
#define ROTATOR_LUNATICO_NAME    "Rotator Lunatico"

extern indigo_result DRIVER_ENTRY_POINT(indigo_driver_action action, indigo_driver_info *info);

#ifdef __cplusplus
}
#endif

#endif /* focuser_lunatico_h */
