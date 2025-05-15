// Copyright (c) 2017-2025 Rumen G. Bogdanovski
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
// 2.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>
// 3.0 refactoring by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO QHY CCD driver
 \file indigo_ccd_qhy.h
 */

#ifndef ccd_qhy_h
#define ccd_qhy_h

#include <indigo/indigo_driver.h>
#include <indigo/indigo_client.h>
#include <indigo/indigo_ccd_driver.h>
#include <indigo/indigo_guider_driver.h>
#include <indigo/indigo_wheel_driver.h>


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

/** Register QHY CCD hot-plug callback
 */

#define INDIGO_CCD_QHY indigo_ccd_qhy
#define DRIVER_NAME "indigo_ccd_qhy"
#define CONFLICTING_DRIVER "indigo_ccd_qhy2"
#define DRIVER_DESCRIPTION "QHY CCD (legacy) Camera"

#if defined(INDIGO_LINUX) || defined(INDIGO_MACOS)
#define USE_LOG4Z
#endif

extern indigo_result INDIGO_CCD_QHY(indigo_driver_action action, indigo_driver_info *info);

#ifdef __cplusplus
}
#endif

#endif /* ccd_qhy_h */
