// Copyright (c) 2018 Rumen G. Bogdanovski
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

/** INDIGO ASCOL system driver
 \file indigo_system_ascol.h
 */

#ifndef system_ascol_h
#define system_ascol_h

#include "indigo_driver.h"
#include "indigo_mount_driver.h"
#include "indigo_guider_driver.h"
#include "indigo_dome_driver.h"
#include "indigo_focuser_driver.h"
#include "indigo_aux_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SYSTEM_ASCOL_PANEL_NAME        "ASCOL Panel"
#define SYSTEM_ASCOL_NAME              "ASCOL Telescope"
#define SYSTEM_ASCOL_GUIDER_NAME       "ASCOL Guider"
#define SYSTEM_ASCOL_FOCUSER_NAME      "ASCOL Focuser"
#define SYSTEM_ASCOL_DOME_NAME         "ASCOL Dome"


/** Create ASCOL system device instance
 */

extern indigo_result indigo_system_ascol(indigo_driver_action action, indigo_driver_info *info);

#ifdef __cplusplus
}
#endif

#endif /* system_ascol_h */
