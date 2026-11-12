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

// version history
// 3.0 by Rumen G. Bogdanovski <rumenastro@gmail.com>

/** INDIGO Scheduler agent task header parser
 \file indigo_agent_scheduler_header.h
 */

#ifndef agent_scheduler_header_h
#define agent_scheduler_header_h

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHEDULER_TASK_NAME_SIZE	41
#define SCHEDULER_TASK_TEXT_LIMIT	16384
#define SCHEDULER_DEFAULT_DURATION	60

typedef enum {
	SCHEDULER_PRIORITY_LOW = 0,
	SCHEDULER_PRIORITY_NORMAL,
	SCHEDULER_PRIORITY_HIGH,
	SCHEDULER_PRIORITY_URGENT
} scheduler_priority_class;

typedef enum {
	SCHEDULER_TWILIGHT_NONE = 0,
	SCHEDULER_TWILIGHT_CIVIL,
	SCHEDULER_TWILIGHT_NAUTICAL,
	SCHEDULER_TWILIGHT_ASTRONOMICAL
} scheduler_twilight;

typedef enum {
	SCHEDULER_WINDOW_NONE = 0,
	SCHEDULER_WINDOW_ABSOLUTE,
	SCHEDULER_WINDOW_NIGHTLY
} scheduler_window_kind;

typedef struct {
	char name[SCHEDULER_TASK_NAME_SIZE];
	scheduler_priority_class priority_class;
	int level;
	bool has_target;
	double ra, dec;
	bool has_min_altitude;
	double min_altitude;
	bool has_max_airmass;
	double max_airmass;
	bool has_ha;
	double ha_from, ha_to;
	bool has_moon;
	double moon_separation;
	double moon_width;
	bool has_illumination;
	double max_illumination;
	scheduler_twilight twilight;
	scheduler_window_kind window_kind;
	bool has_window_start, has_window_end;
	time_t window_start, window_end;
	int window_start_minute, window_end_minute;
	bool has_duration;
	int duration;
	int repeat;
	int repeat_every;
	int retry;
	int retry_after;
	bool preempted_fail;
	bool enabled;
} scheduler_header;

/** Parse and validate the header of a task text. On success fills header, stores the offset of the body and returns true. On failure writes a message naming the line to error and returns false.
 */
extern bool indigo_scheduler_parse_header(const char *text, scheduler_header *header, size_t *body_offset, char *error, size_t error_size);

/** Return the effective duration of the task in minutes.
 */
extern int indigo_scheduler_header_duration(const scheduler_header *header);

/** Return the name of the priority class.
 */
extern const char *indigo_scheduler_priority_name(scheduler_priority_class priority_class);

#ifdef __cplusplus
}
#endif

#endif /* agent_scheduler_header_h */
