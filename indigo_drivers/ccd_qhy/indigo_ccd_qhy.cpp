// Copyright (c) 2017 Rumen G. Bogdanovski
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
// 2.0 Build 0 - PoC by Rumen G. Bogdanovski

/** INDIGO QHY CCD driver
 \file indigo_ccd_qhy.cpp
 \NOTE: This file should be .cpp as qhy headers are in C++
 */

#define DRIVER_VERSION 0x0001
#define DRIVER_NAME "indigo_ccd_qhy"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#if defined(INDIGO_MACOS)
#include <libusb-1.0/libusb.h>
#elif defined(INDIGO_FREEBSD)
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif

#include "qhyccd.h"
#include "log4z.h"
#include "indigo_ccd_qhy.h"
#include "indigo_driver_xml.h"

indigo_result indigo_ccd_qhy(indigo_driver_action action, indigo_driver_info *info) {
	SetQHYCCDLogLevel(LOG_LEVEL_FATAL);
	uint32_t res = InitQHYCCDResource();
	if (res != QHYCCD_SUCCESS) INDIGO_DRIVER_ERROR(DRIVER_NAME, "InitQHYCCDResource() ERROR = %d", res);
	else INDIGO_DRIVER_ERROR(DRIVER_NAME, "InitQHYCCDResource() OK = %d", res);
	ReleaseQHYCCDResource();
	return INDIGO_OK;
}
