// Copyright (c) 2016-2026 CloudMakers, s. r. o.
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
// 3.0 generated-driver migration by Peter Polakovic <peter.polakovic@cloudmakers.eu>

#ifndef indigo_ccd_simulator_data_h
#define indigo_ccd_simulator_data_h

#define IMAGER_WIDTH 1600
#define IMAGER_HEIGHT 1200
#define GUIDER_WIDTH 1600
#define GUIDER_HEIGHT 1200
#define BAHTINOV_WIDTH 500
#define BAHTINOV_HEIGHT 500
#define BAHTINOV_MAX_STEPS 15
#define DSLR_WIDTH 1600
#define DSLR_HEIGHT 1200

extern unsigned short indigo_ccd_simulator_raw_image[];
extern unsigned char indigo_ccd_simulator_rgb_image[];
extern unsigned char indigo_ccd_simulator_bahtinov_image[][BAHTINOV_WIDTH * BAHTINOV_HEIGHT];

#endif
