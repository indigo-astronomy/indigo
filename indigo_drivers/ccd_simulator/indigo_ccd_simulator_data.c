// Copyright (c) 2018-2025 CloudMakers, s. r. o.
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

/** INDIGO CCD Simulator driver
 \file indigo_ccd_simulator_data.c
 */

#include <stdio.h>
#include "indigo_ccd_simulator.h"

unsigned short indigo_ccd_simulator_raw_image[] = {
#include "indigo_ccd_simulator_mono.h"
};

unsigned char indigo_ccd_simulator_rgb_image[] = {
#include "indigo_ccd_simulator_rgb.h"
};

unsigned char indigo_ccd_simulator_bahtinov_image[][500 * 500] = { {
#ifdef BAHTINOV_ASYMETRIC
#include "indigo_ccd_simulator_bahtinov_-15.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-14.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-13.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-12.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-11.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-10.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-9.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-8.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-7.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-6.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-5.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-4.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-3.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-2.h"
}, {
#include "indigo_ccd_simulator_bahtinov_-1.h"
}, {
#endif
#include "indigo_ccd_simulator_bahtinov_0.h"
}, {
#include "indigo_ccd_simulator_bahtinov_1.h"
}, {
#include "indigo_ccd_simulator_bahtinov_2.h"
}, {
#include "indigo_ccd_simulator_bahtinov_3.h"
}, {
#include "indigo_ccd_simulator_bahtinov_4.h"
}, {
#include "indigo_ccd_simulator_bahtinov_5.h"
}, {
#include "indigo_ccd_simulator_bahtinov_6.h"
}, {
#include "indigo_ccd_simulator_bahtinov_7.h"
}, {
#include "indigo_ccd_simulator_bahtinov_8.h"
}, {
#include "indigo_ccd_simulator_bahtinov_9.h"
}, {
#include "indigo_ccd_simulator_bahtinov_10.h"
}, {
#include "indigo_ccd_simulator_bahtinov_11.h"
}, {
#include "indigo_ccd_simulator_bahtinov_12.h"
}, {
#include "indigo_ccd_simulator_bahtinov_13.h"
}, {
#include "indigo_ccd_simulator_bahtinov_14.h"
}, {
#include "indigo_ccd_simulator_bahtinov_15.h"
}
};

