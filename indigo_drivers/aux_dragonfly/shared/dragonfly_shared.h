// Copyright (C) 2020-2026 Rumen G. Bogdanovski
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
// 3.0 refactoring to indigo_generator by Peter Polakovic <peter.polakovic@cloudmakers.eu>

/** INDIGO Lunatico Dragonfly shared definitions
 \file dragonfly_shared.h
 */

// Constants shared by indigo_aux_dragonfly and indigo_dome_dragonfly. Included
// from the define { } block of both .driver definitions, so they are available
// to the private data declaration and to shared/dragonfly_shared.c.

#ifndef dragonfly_shared_h
#define dragonfly_shared_h

// The Dragonfly has eight relays and eight analog sensor inputs on port 0.
#define DRAGONFLY_CHANNELS          8

// The SLP request and reply of this protocol always fit one datagram; this is
// the historical buffer size of the driver.
#define DRAGONFLY_CMD_LEN           100

// Default UDP port of the controller when DEVICE_PORT carries a bare host name.
#define DRAGONFLY_UDP_PORT          10000

// A reply is expected within this time, matching the original 3.1 s select().
#define DRAGONFLY_REPLY_TIMEOUT     INDIGO_DELAY(3)

// Extra time added to a relay pulse before the driver clears the outlet item,
// so the controller has released the relay first.
#define DRAGONFLY_PULSE_MARGIN_MS   20

// An analog sensor input is considered active above this value.
#define DRAGONFLY_SENSOR_THRESHOLD  512

#define AUX_RELAYS_GROUP            "Relay control"
#define AUX_SENSORS_GROUP           "Sensors"

#endif /* dragonfly_shared_h */
