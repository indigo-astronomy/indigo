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

/** INDIGO Scheduler agent
 \file indigo_agent_scheduler.c
 */

#define DRIVER_VERSION 0x03000004
#define DRIVER_NAME	"indigo_agent_scheduler"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

#include <indigo/indigo_driver_xml.h>
#include <indigo/indigo_uni_io.h>

#include "indigo_agent_scheduler.h"
#include "indigo_agent_scheduler_header.h"
#include "indigo_agent_scheduler_sky.h"
#include "indigo_agent_scheduler_plan.h"

#define MAX_TASKS									128
#define SCHEDULER_GROUP						"Scheduler"
#define TASKS_GROUP								"Tasks"
#define SCRIPTING_AGENT						"Scripting Agent"
#define IMAGER_AGENT							"Imager Agent"
#define MOUNT_AGENT								"Mount Agent"
#define GUIDER_AGENT							"Guider Agent"
#define SCHEDULER_RESULT_PROPERTY	"SCHEDULER_RESULT"
#define SCHEDULER_ESTIMATE_PROPERTY	"SCHEDULER_ESTIMATE"
#define MAX_HISTORY								200
#define ESTIMATE_TIMEOUT					10
#define MAX_DURATION							86400
#define MAX_PLAN_RUNS							256
#define MAX_PLAN_ITEMS						(MAX_PLAN_RUNS + MAX_TASKS)
#define LIBRARY_RETRY_INTERVAL		30
#define SAVE_DELAY								2
#define SKY_REFRESH_INTERVAL			3600
#define TICK_INTERVAL							0.5

#define AGENT_IMAGER_BUSY					0x01
#define AGENT_MOUNT_BUSY					0x02
#define AGENT_GUIDER_BUSY					0x04
#define AGENT_SOLVER_BUSY					0x08

#define PRIVATE_DATA							((scheduler_private_data *)device->private_data)

#define ADD_TASK_PROPERTY					(PRIVATE_DATA->add_task_property)
#define ADD_TASK_TASK_ITEM				(ADD_TASK_PROPERTY->items + 0)
#define ADD_TASK_STATUS_ITEM			(ADD_TASK_PROPERTY->items + 1)
#define DELETE_TASK_PROPERTY			(PRIVATE_DATA->delete_task_property)
#define DELETE_TASK_NAME_ITEM			(DELETE_TASK_PROPERTY->items + 0)
#define URGENT_TASK_PROPERTY			(PRIVATE_DATA->urgent_task_property)
#define URGENT_TASK_TASK_ITEM			(URGENT_TASK_PROPERTY->items + 0)
#define TASK_STATE_PROPERTY				(PRIVATE_DATA->task_state_property)
#define TASK_NAME_PROPERTY				(PRIVATE_DATA->task_name_property)
#define TASK_NAME_NAME_ITEM				(TASK_NAME_PROPERTY->items + 0)
#define TASK_COMMAND_PROPERTY			(PRIVATE_DATA->task_command_property)
#define TASK_COMMAND_RESET_ITEM		(TASK_COMMAND_PROPERTY->items + 0)
#define TASK_COMMAND_ENABLE_ITEM	(TASK_COMMAND_PROPERTY->items + 1)
#define TASK_COMMAND_DISABLE_ITEM	(TASK_COMMAND_PROPERTY->items + 2)
#define TASK_COMMAND_UP_ITEM			(TASK_COMMAND_PROPERTY->items + 3)
#define TASK_COMMAND_DOWN_ITEM		(TASK_COMMAND_PROPERTY->items + 4)
#define CONTROL_PROPERTY					(PRIVATE_DATA->control_property)
#define CONTROL_START_ITEM				(CONTROL_PROPERTY->items + 0)
#define CONTROL_STOP_ITEM					(CONTROL_PROPERTY->items + 1)
#define CONTROL_ABORT_ITEM				(CONTROL_PROPERTY->items + 2)
#define STATE_PROPERTY						(PRIVATE_DATA->state_property)
#define STATE_DONE_ITEM						(STATE_PROPERTY->items + 0)
#define STATE_FAILED_ITEM					(STATE_PROPERTY->items + 1)
#define STATE_TOTAL_ITEM					(STATE_PROPERTY->items + 2)
#define CURRENT_PROPERTY					(PRIVATE_DATA->current_property)
#define CURRENT_CURRENT_ITEM			(CURRENT_PROPERTY->items + 0)
#define CURRENT_NEXT_ITEM					(CURRENT_PROPERTY->items + 1)
#define CURRENT_NEXT_START_ITEM		(CURRENT_PROPERTY->items + 2)
#define CURRENT_REASON_ITEM				(CURRENT_PROPERTY->items + 3)
#define SETTINGS_PROPERTY					(PRIVATE_DATA->settings_property)
#define SETTINGS_IDLE_CHECK_ITEM	(SETTINGS_PROPERTY->items + 0)
#define SETTINGS_STOP_TIMEOUT_ITEM	(SETTINGS_PROPERTY->items + 1)
#define SETTINGS_START_TIMEOUT_ITEM	(SETTINGS_PROPERTY->items + 2)
#define SETTINGS_SETTING_WEIGHT_ITEM	(SETTINGS_PROPERTY->items + 3)
#define SETTINGS_ALTITUDE_WEIGHT_ITEM	(SETTINGS_PROPERTY->items + 4)
#define SETTINGS_IDLE_PARK_AFTER_ITEM	(SETTINGS_PROPERTY->items + 5)
#define SETTINGS_FRAME_OVERHEAD_ITEM	(SETTINGS_PROPERTY->items + 6)
#define SETTINGS_OPERATION_OVERHEAD_ITEM	(SETTINGS_PROPERTY->items + 7)
#define HOOKS_PROPERTY						(PRIVATE_DATA->hooks_property)
#define HOOKS_STARTUP_ITEM				(HOOKS_PROPERTY->items + 0)
#define HOOKS_SHUTDOWN_ITEM				(HOOKS_PROPERTY->items + 1)
#define HOOKS_IDLE_ITEM						(HOOKS_PROPERTY->items + 2)
#define HISTORY_PROPERTY					(PRIVATE_DATA->history_property)
#define CLEAR_HISTORY_PROPERTY		(PRIVATE_DATA->clear_history_property)
#define CLEAR_HISTORY_ITEM				(CLEAR_HISTORY_PROPERTY->items + 0)
#define RESTORE_HISTORY_PROPERTY	(PRIVATE_DATA->restore_history_property)
#define PLAN_PROPERTY							(PRIVATE_DATA->plan_property)
#define RESTORE_HISTORY_ITEM			(RESTORE_HISTORY_PROPERTY->items + 0)

typedef enum {
	TASK_QUEUED = 0,
	TASK_WAITING,
	TASK_RUNNING,
	TASK_RETRY,
	TASK_REPEAT,
	TASK_DONE,
	TASK_FAILED,
	TASK_EXPIRED
} task_state;

static const char *task_state_names[] = { "QUEUED", "WAITING", "RUNNING", "RETRY", "REPEAT", "DONE", "FAILED", "EXPIRED" };

typedef enum {
	RUN_IDLE = 0,
	RUN_STARTING,
	RUN_RUNNING,
	RUN_STOPPING
} run_phase;

typedef enum {
	RUN_TASK = 0,
	RUN_STARTUP_HOOK,
	RUN_SHUTDOWN_HOOK,
	RUN_IDLE_HOOK
} run_kind;

typedef enum {
	RESULT_SUCCEEDED = 0,
	RESULT_FAILED,
	RESULT_PREEMPTED,
	RESULT_ABORTED
} run_result;

static const char *run_result_names[] = { "succeeded", "failed", "preempted", "aborted" };

typedef struct {
	char task[SCHEDULER_TASK_NAME_SIZE];
	int run;
	time_t start, end;
	run_result result;
	char mark;
	char reason[INDIGO_VALUE_SIZE];
} history_entry;

typedef enum {
	STOP_PREEMPT = 0,
	STOP_ABORT,
	STOP_DELETE,
	STOP_FAILURE
} stop_reason;

typedef struct {
	bool used;
	bool stored;
	char *text;
	scheduler_header header;
	char *pending_text;
	scheduler_header pending_header;
	task_state state;
	int runs;
	int retries_used;
	bool disabled;
	time_t delay_until;
	unsigned long urgent_order;
	int measured;
	bool estimate_needed;
	bool estimated;
	int estimate;
	bool script_error;
	scheduler_target_grid target;
	scheduler_task_points points;
	indigo_property *property;
} scheduler_task;

// bus state observed by the client callbacks, guarded by its own mutex
typedef struct {
	pthread_mutex_t mutex;
	bool scripting_present;
	bool sequencer_present;
	indigo_property_state sequence_state;
	unsigned busy_events;
	unsigned finish_events;
	indigo_property_state finish_state;
	char finish_message[INDIGO_VALUE_SIZE];
	char sequence_name[INDIGO_VALUE_SIZE];
	// results of the run and of the estimate are kept apart by their tokens
	int run_token;
	bool run_received;
	bool run_ok;
	char run_message[INDIGO_VALUE_SIZE];
	int estimate_token;
	bool estimate_received;
	bool estimate_ok;
	char estimate_message[INDIGO_VALUE_SIZE];
	bool estimate_counts;
	double estimate_values[6];
	bool site_known;
	double latitude, longitude, elevation;
	unsigned agents_busy;
} scheduler_observed;

typedef struct {
	indigo_property *add_task_property;
	indigo_property *delete_task_property;
	indigo_property *urgent_task_property;
	indigo_property *task_state_property;
	indigo_property *task_name_property;
	indigo_property *task_command_property;
	indigo_property *control_property;
	indigo_property *state_property;
	indigo_property *current_property;
	indigo_property *settings_property;
	indigo_property *hooks_property;
	indigo_property *history_property;
	indigo_property *clear_history_property;
	indigo_property *restore_history_property;
	indigo_property *plan_property;
	char plan_labels[MAX_PLAN_ITEMS][INDIGO_VALUE_SIZE];
	indigo_property_state plan_states[MAX_PLAN_ITEMS];
	int plan_count;
	bool plan_dirty;
	double plan_due;
	scheduler_observed observed;
	history_entry history[MAX_HISTORY];
	int history_count;
	unsigned history_serial;
	scheduler_task tasks[MAX_TASKS];
	int order[MAX_TASKS];
	int order_count;
	// guards the task texts and the task list against client enumerations, never held while calling the bus
	pthread_mutex_t property_mutex;
	char task_state_labels[MAX_TASKS][INDIGO_VALUE_SIZE];
	char label_buffer[MAX_TASKS][INDIGO_VALUE_SIZE];
	int task_state_slots[MAX_TASKS];
	int task_state_count;
	unsigned long urgent_counter;
	bool shutting_down;
	bool started;
	bool stop_requested;
	bool automatic_stop;
	indigo_property_state stop_state;
	char stop_text[INDIGO_VALUE_SIZE];
	bool startup_pending;
	bool shutdown_ran;
	bool idle_hook_done;
	time_t idle_since;
	run_phase phase;
	run_kind kind;
	time_t run_start;
	int run_number;
	int current;
	stop_reason stop;
	char stop_message[INDIGO_VALUE_SIZE];
	bool stop_waits_for_start;
	int token;
	double deadline;
	unsigned busy_baseline;
	unsigned finish_baseline;
	bool result_received;
	int estimate_slot;
	double estimate_deadline;
	double library_sent;
	bool dirty;
	double next_check;
	double save_due;
	scheduler_sky_grid sky;
	char reason[INDIGO_VALUE_SIZE];
	time_t next_start;
	int next_task;
} scheduler_private_data;

typedef struct {
	indigo_property *property;
	indigo_property *target;
	bool config_reader;
} scheduler_command;

static char sequencer_js[] = {
#include "Sequencer.js.dat"
	0
};

static indigo_device *agent_device = NULL;
static indigo_client *agent_client = NULL;

static void process(indigo_device *device);

// -------------------------------------------------------------------------------- Helpers

static bool is_urgent(scheduler_task *task) {
	return task->header.priority_class == SCHEDULER_PRIORITY_URGENT;
}

static bool is_selectable_state(task_state state) {
	return state == TASK_QUEUED || state == TASK_WAITING;
}

static bool is_terminal_state(task_state state) {
	return state == TASK_DONE || state == TASK_FAILED || state == TASK_EXPIRED;
}

static bool is_enabled(scheduler_task *task) {
	return task->header.enabled && !task->disabled;
}

// the hook a task serves, RUN_TASK for an ordinary task
static run_kind hook_kind(indigo_device *device, scheduler_task *task) {
	if (*indigo_get_text_item_value(HOOKS_STARTUP_ITEM) && !strcasecmp(indigo_get_text_item_value(HOOKS_STARTUP_ITEM), task->header.name)) {
		return RUN_STARTUP_HOOK;
	}
	if (*indigo_get_text_item_value(HOOKS_SHUTDOWN_ITEM) && !strcasecmp(indigo_get_text_item_value(HOOKS_SHUTDOWN_ITEM), task->header.name)) {
		return RUN_SHUTDOWN_HOOK;
	}
	if (*indigo_get_text_item_value(HOOKS_IDLE_ITEM) && !strcasecmp(indigo_get_text_item_value(HOOKS_IDLE_ITEM), task->header.name)) {
		return RUN_IDLE_HOOK;
	}
	return RUN_TASK;
}

static bool is_hook(indigo_device *device, scheduler_task *task) {
	return hook_kind(device, task) != RUN_TASK;
}

// effective duration in seconds: @duration, the measured duration, the estimate, or the default
static int task_duration(scheduler_task *task, const char **source) {
	int duration;
	const char *from;
	if (task->header.has_duration) {
		duration = task->header.duration * 60;
		from = "set";
	} else if (task->measured > 0) {
		duration = task->measured;
		from = "measured";
	} else if (task->estimated) {
		duration = task->estimate;
		from = "estimated";
	} else {
		duration = SCHEDULER_DEFAULT_DURATION * 60;
		from = "default";
	}
	if (source) {
		*source = from;
	}
	if (duration < 60) {
		duration = 60;
	} else if (duration > MAX_DURATION) {
		duration = MAX_DURATION;
	}
	return duration;
}

static int find_task(indigo_device *device, const char *name) {
	for (int i = 0; i < MAX_TASKS; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + i;
		if (task->used && !strcasecmp(task->header.name, name)) {
			return i;
		}
	}
	return -1;
}

// a name is taken by a task or by the pending rename of a running task
static bool name_taken(indigo_device *device, const char *name, int except) {
	for (int i = 0; i < MAX_TASKS; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + i;
		if (i == except || !task->used) {
			continue;
		}
		if (!strcasecmp(task->header.name, name) || (task->pending_text && !strcasecmp(task->pending_header.name, name))) {
			return true;
		}
	}
	return false;
}

static int order_index(indigo_device *device, int slot) {
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		if (PRIVATE_DATA->order[i] == slot) {
			return i;
		}
	}
	return -1;
}

static void cut_label(const char *text, char *label, size_t size) {
	// labels are limited to INDIGO_VALUE_SIZE - 1 bytes, do not cut a UTF-8 sequence
	size_t length = strlen(text);
	if (length >= size) {
		length = size - 1;
		while (length > 0 && ((unsigned char)text[length] & 0xC0) == 0x80) {
			length--;
		}
	}
	memcpy(label, text, length);
	label[length] = 0;
}

// JSON string literal, the result is allocated
static char *json_string(const char *text) {
	size_t length = strlen(text);
	char *result = indigo_safe_malloc(length * 6 + 3);
	char *out = result;
	*out++ = '"';
	for (const unsigned char *in = (const unsigned char *)text; *in; in++) {
		if (*in == '"' || *in == '\\') {
			*out++ = '\\';
			*out++ = *in;
		} else if (*in == '\n') {
			*out++ = '\\';
			*out++ = 'n';
		} else if (*in == '\r') {
			*out++ = '\\';
			*out++ = 'r';
		} else if (*in == '\t') {
			*out++ = '\\';
			*out++ = 't';
		} else if (*in < 0x20 || *in == 0x7f) {
			out += sprintf(out, "\\u%04x", *in);
		} else if (in[0] == 0xE2 && in[1] == 0x80 && (in[2] == 0xA8 || in[2] == 0xA9)) {
			// line and paragraph separators are not allowed in ES5 string literals
			out += sprintf(out, "\\u%04x", in[2] == 0xA8 ? 0x2028 : 0x2029);
			in += 2;
		} else {
			*out++ = *in;
		}
	}
	*out++ = '"';
	*out = 0;
	return result;
}

// the script that runs a task or, in estimation mode, only builds its sequence and reports the counts of estimate()
static char *wrapper_script(indigo_device *device, scheduler_task *task, int token, bool estimate) {
	char *body = json_string(task->text);
	char *name = json_string(task->header.name);
	char target[128] = "";
	if (task->header.has_target) {
		snprintf(target, sizeof(target), " ra: %.8f, dec: %.8f,", task->header.ra, task->header.dec);
	}
	char report[1024];
	snprintf(report, sizeof(report), "var scheduler_report = function(result, message) { indigo_define_text_property(\"%s\", \"%s\", \"%s\", \"Scheduler result\", { TOKEN: \"%d\", RESULT: result, MESSAGE: message }, { TOKEN: { label: \"Token\" }, RESULT: { label: \"Result\" }, MESSAGE: { label: \"Message\" }}, result == \"ok\" ? \"Ok\" : \"Alert\", \"RO\"); };\n", SCRIPTING_AGENT, SCHEDULER_RESULT_PROPERTY, SCHEDULER_GROUP, token);
	char counts[2048] = "";
	char restore[128] = "";
	if (estimate) {
		snprintf(counts, sizeof(counts), "var scheduler_start = Sequence.prototype.start;\nSequence.prototype.start = function() { var e = typeof this.estimate == \"function\" ? this.estimate() : null; indigo_define_number_property(\"%s\", \"%s\", \"%s\", \"Scheduler estimate\", { TOKEN: %d, SUPPORTED: e ? 1 : 0, EXPOSURE: e ? e.exposure : 0, FRAMES: e ? e.frames : 0, OPERATIONS: e ? e.operations : 0, WAITS: e ? e.waits : 0, UNKNOWN: e && e.unknown ? 1 : 0 }, { TOKEN: { label: \"Token\", format: \"%%g\", min: 0, max: 1e12, step: 1 }, SUPPORTED: { label: \"Supported\", format: \"%%g\", min: 0, max: 1, step: 1 }, EXPOSURE: { label: \"Exposure (s)\", format: \"%%g\", min: 0, max: 1e9, step: 1 }, FRAMES: { label: \"Frames\", format: \"%%g\", min: 0, max: 1e9, step: 1 }, OPERATIONS: { label: \"Operations\", format: \"%%g\", min: 0, max: 1e9, step: 1 }, WAITS: { label: \"Waits (s)\", format: \"%%g\", min: 0, max: 1e9, step: 1 }, UNKNOWN: { label: \"Unknown\", format: \"%%g\", min: 0, max: 1, step: 1 }}, \"Ok\", \"RO\"); };\n", SCRIPTING_AGENT, SCHEDULER_ESTIMATE_PROPERTY, SCHEDULER_GROUP, token);
		strcpy(restore, "Sequence.prototype.start = scheduler_start;\n");
	}
	const char *format =
		"(function() {\n"
		"var scheduler_task = { name: %s,%s run: %d, token: %d%s };\n"
		"%s"
		"try {\n"
		"%s"
		"try {\n"
		"(function(scheduler_task) { eval(%s); })(scheduler_task);\n"
		"} finally {\n"
		"%s"
		"}\n"
		"scheduler_report(\"ok\", \"\");\n"
		"} catch (error) {\n"
		"scheduler_report(\"error\", String(error));\n"
		"}\n"
		"})();\n";
	size_t size = strlen(format) + strlen(body) + strlen(name) + strlen(target) + strlen(report) + strlen(counts) + strlen(restore) + 256;
	char *script = indigo_safe_malloc(size);
	snprintf(script, size, format, name, target, task->runs + 1, token, estimate ? ", estimate: true" : "", report, counts, body, restore);
	free(body);
	free(name);
	return script;
}

static void status_string(scheduler_task *task, char *status, size_t size) {
	task_state state = task->state;
	if (state == TASK_RUNNING) {
		state = TASK_RETRY;
	} else if (state == TASK_WAITING) {
		state = TASK_QUEUED;
	}
	snprintf(status, size, "state=%s runs=%d retries=%d disabled=%d measured=%d", task_state_names[state], task->runs, task->retries_used, task->disabled ? 1 : 0, task->measured);
}

static bool status_value(const char *status, const char *key, char *value, size_t size) {
	const char *s = status;
	size_t length = strlen(key);
	while ((s = strstr(s, key)) != NULL) {
		if ((s == status || s[-1] == ' ') && s[length] == '=') {
			s += length + 1;
			size_t n = strcspn(s, " ");
			if (n >= size) {
				n = size - 1;
			}
			memcpy(value, s, n);
			value[n] = 0;
			return true;
		}
		s += length;
	}
	return false;
}

static void apply_status(scheduler_task *task, const char *status) {
	char value[32];
	if (status_value(status, "state", value, sizeof(value))) {
		for (int i = 0; i < (int)(sizeof(task_state_names) / sizeof(task_state_names[0])); i++) {
			if (!strcmp(value, task_state_names[i])) {
				task->state = (task_state)i;
			}
		}
	}
	if (status_value(status, "runs", value, sizeof(value))) {
		task->runs = atoi(value);
	}
	if (status_value(status, "retries", value, sizeof(value))) {
		task->retries_used = atoi(value);
	}
	if (status_value(status, "disabled", value, sizeof(value))) {
		task->disabled = atoi(value) != 0;
	}
	if (status_value(status, "measured", value, sizeof(value))) {
		task->measured = atoi(value);
	}
	if (task->state == TASK_RUNNING) {
		task->state = TASK_RETRY;
	} else if (task->state == TASK_WAITING) {
		task->state = TASK_QUEUED;
	}
	task->delay_until = 0;
}

// -------------------------------------------------------------------------------- Run history

static indigo_property_state history_light(run_result result) {
	switch (result) {
		case RESULT_SUCCEEDED:
			return INDIGO_OK_STATE;
		case RESULT_FAILED:
			return INDIGO_ALERT_STATE;
		default:
			return INDIGO_BUSY_STATE;
	}
}

static void history_label(history_entry *entry, char *label, size_t size) {
	struct tm start, end;
	indigo_gmtime(&entry->start, &start);
	indigo_gmtime(&entry->end, &end);
	char text[INDIGO_VALUE_SIZE * 2];
	snprintf(text, sizeof(text), "%04d-%02d-%02d %02d:%02d\xe2\x80\x93%02d:%02d  %s%s #%d  %s%s%s", start.tm_year + 1900, start.tm_mon + 1, start.tm_mday, start.tm_hour, start.tm_min, end.tm_hour, end.tm_min, entry->task, entry->mark == 'H' ? " (hook)" : entry->mark == 'U' ? " (urgent)" : "", entry->run, run_result_names[entry->result], *entry->reason ? ": " : "", entry->reason);
	cut_label(text, label, size);
}

// the history is a list of finished runs, newest first; its length changes, so it is defined again
static void update_history_property(indigo_device *device) {
	indigo_delete_property(device, HISTORY_PROPERTY, NULL);
	pthread_mutex_lock(&PRIVATE_DATA->property_mutex);
	for (int i = 0; i < PRIVATE_DATA->history_count; i++) {
		char name[INDIGO_NAME_SIZE];
		char label[INDIGO_VALUE_SIZE];
		snprintf(name, sizeof(name), AGENT_SCHEDULER_HISTORY_ITEM_NAME, PRIVATE_DATA->history_serial - i);
		history_label(PRIVATE_DATA->history + i, label, sizeof(label));
		indigo_init_light_item(HISTORY_PROPERTY->items + i, name, label, history_light(PRIVATE_DATA->history[i].result));
	}
	HISTORY_PROPERTY->count = PRIVATE_DATA->history_count;
	pthread_mutex_unlock(&PRIVATE_DATA->property_mutex);
	indigo_define_property(device, HISTORY_PROPERTY, NULL);
}

static void insert_history(indigo_device *device, const char *task, int run, time_t start, time_t end, run_result result, char mark, const char *reason) {
	if (PRIVATE_DATA->history_count == MAX_HISTORY) {
		PRIVATE_DATA->history_count--;
	}
	memmove(PRIVATE_DATA->history + 1, PRIVATE_DATA->history, PRIVATE_DATA->history_count * sizeof(history_entry));
	history_entry *entry = PRIVATE_DATA->history;
	memset(entry, 0, sizeof(*entry));
	// the name field is shorter than INDIGO_NAME_SIZE, and a restored name may be longer than a valid one
	snprintf(entry->task, sizeof(entry->task), "%s", task);
	entry->run = run;
	entry->start = start;
	entry->end = end;
	entry->result = result;
	entry->mark = mark;
	// tabs and line breaks would break the saved form, one line per entry
	INDIGO_COPY_VALUE(entry->reason, reason ? reason : "");
	for (char *c = entry->reason; *c; c++) {
		if (*c == '\t' || *c == '\n' || *c == '\r') {
			*c = ' ';
		}
	}
	PRIVATE_DATA->history_count++;
	PRIVATE_DATA->history_serial++;
}

// records the end of the current run
static void add_history(indigo_device *device, scheduler_task *task, run_result result, const char *reason) {
	char mark = PRIVATE_DATA->kind != RUN_TASK ? 'H' : is_urgent(task) ? 'U' : ' ';
	insert_history(device, task->header.name, PRIVATE_DATA->run_number, PRIVATE_DATA->run_start, time(NULL), result, mark, reason);
	update_history_property(device);
}

static char *history_text(indigo_device *device) {
	size_t size = PRIVATE_DATA->history_count * (INDIGO_VALUE_SIZE + 128) + 1;
	char *text = indigo_safe_malloc(size);
	char *out = text;
	*out = 0;
	for (int i = PRIVATE_DATA->history_count - 1; i >= 0; i--) {
		history_entry *entry = PRIVATE_DATA->history + i;
		// the configuration reader trims trailing whitespace, so no field is saved as a space
		out += snprintf(out, size - (out - text), "%s\t%d\t%ld\t%ld\t%s\t%c\t%s\n", entry->task, entry->run, (long)entry->start, (long)entry->end, run_result_names[entry->result], entry->mark == ' ' ? '-' : entry->mark, entry->reason);
	}
	return text;
}

static void restore_history(indigo_device *device, const char *text) {
	PRIVATE_DATA->history_count = 0;
	const char *line = text;
	while (line && *line) {
		const char *end = strchr(line, '\n');
		size_t length = end ? (size_t)(end - line) : strlen(line);
		char buffer[INDIGO_VALUE_SIZE + 256];
		if (length < sizeof(buffer)) {
			memcpy(buffer, line, length);
			buffer[length] = 0;
			char *fields[7];
			int count = 0;
			char *field = buffer;
			while (count < 7) {
				fields[count++] = field;
				char *tab = count < 7 ? strchr(field, '\t') : NULL;
				if (tab == NULL) {
					break;
				}
				*tab = 0;
				field = tab + 1;
			}
			// trailing empty fields may have been trimmed
			if (count >= 5) {
				run_result result = RESULT_FAILED;
				for (int i = 0; i < 4; i++) {
					if (!strcmp(fields[4], run_result_names[i])) {
						result = (run_result)i;
					}
				}
				char mark = count > 5 && (*fields[5] == 'H' || *fields[5] == 'U') ? *fields[5] : ' ';
				insert_history(device, fields[0], atoi(fields[1]), (time_t)atol(fields[2]), (time_t)atol(fields[3]), result, mark, count > 6 ? fields[6] : "");
			}
		}
		line = end ? end + 1 : NULL;
	}
	update_history_property(device);
}

// -------------------------------------------------------------------------------- Persistence

static void save_config(indigo_device *device) {
	if (CONFIG_PROPERTY->state == INDIGO_BUSY_STATE) {
		PRIVATE_DATA->save_due = indigo_monotonic_time() + SAVE_DELAY;
		return;
	}
	PRIVATE_DATA->save_due = 0;
	if (pthread_mutex_trylock(&DEVICE_CONTEXT->config_mutex) != 0) {
		PRIVATE_DATA->save_due = indigo_monotonic_time() + SAVE_DELAY;
		return;
	}
	pthread_mutex_unlock(&DEVICE_CONTEXT->config_mutex);
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + PRIVATE_DATA->order[i];
		if (!task->stored) {
			continue;
		}
		char status[INDIGO_VALUE_SIZE];
		status_string(task, status, sizeof(status));
		indigo_property *property = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_ADD_TASK_PROPERTY_NAME, SCHEDULER_GROUP, "Add task", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		indigo_init_text_item_raw(property->items + 0, AGENT_SCHEDULER_ADD_TASK_TASK_ITEM_NAME, "Task", task->text);
		indigo_init_text_item(property->items + 1, AGENT_SCHEDULER_ADD_TASK_STATUS_ITEM_NAME, "Status", status);
		indigo_save_property(device, NULL, property);
		indigo_release_property(property);
	}
	indigo_save_property(device, NULL, SETTINGS_PROPERTY);
	indigo_save_property(device, NULL, HOOKS_PROPERTY);
	char *entries = history_text(device);
	indigo_property *history = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_RESTORE_HISTORY_PROPERTY_NAME, SCHEDULER_GROUP, "Restore history", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
	indigo_init_text_item_raw(history->items, AGENT_SCHEDULER_RESTORE_HISTORY_ENTRIES_ITEM_NAME, "Entries", entries);
	indigo_save_property(device, NULL, history);
	indigo_release_property(history);
	free(entries);
	if (DEVICE_CONTEXT->property_save_file_handle != NULL) {
		indigo_uni_close(&DEVICE_CONTEXT->property_save_file_handle);
	} else {
		INDIGO_DRIVER_ERROR(DRIVER_NAME, "Failed to save the configuration");
	}
}

static void schedule_save(indigo_device *device) {
	PRIVATE_DATA->save_due = indigo_monotonic_time() + SAVE_DELAY;
}

// -------------------------------------------------------------------------------- Task properties

static void define_task_property(indigo_device *device, int slot) {
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	indigo_property *property = task->property;
	pthread_mutex_lock(&PRIVATE_DATA->property_mutex);
	INDIGO_COPY_VALUE(property->label, task->header.name);
	indigo_set_text_item_value(property->items, task->text);
	property->state = INDIGO_OK_STATE;
	pthread_mutex_unlock(&PRIVATE_DATA->property_mutex);
	indigo_define_property(device, property, NULL);
}

static void build_task_state_label(indigo_device *device, scheduler_task *task, char *label, size_t size) {
	static const char *hook_names[] = { "", "STARTUP HOOK", "SHUTDOWN HOOK", "IDLE HOOK" };
	run_kind kind = hook_kind(device, task);
	const char *state = kind != RUN_TASK ? hook_names[kind] : task->script_error ? "SCRIPT ERROR" : is_enabled(task) ? task_state_names[task->state] : "DISABLED";
	char runs[64];
	if (task->header.repeat > 1) {
		snprintf(runs, sizeof(runs), ", run %d/%d", task->runs, task->header.repeat);
	} else {
		runs[0] = 0;
	}
	char retries[64];
	if (task->header.retry > 0) {
		snprintf(retries, sizeof(retries), ", retries left %d", task->header.retry - task->retries_used);
	} else {
		retries[0] = 0;
	}
	char duration[64] = "";
	if (kind == RUN_TASK) {
		const char *source;
		int minutes = (task_duration(task, &source) + 59) / 60;
		snprintf(duration, sizeof(duration), ", %dh%02d %s", minutes / 60, minutes % 60, source);
	}
	char text[INDIGO_VALUE_SIZE + 128];
	snprintf(text, sizeof(text), "%s%s - %s%s%s%s", task->header.name, is_urgent(task) ? " (urgent)" : "", state, runs, retries, duration);
	cut_label(text, label, size);
}

static indigo_property_state task_light_state(indigo_device *device, scheduler_task *task) {
	if (task->script_error) {
		return INDIGO_ALERT_STATE;
	}
	if (!is_enabled(task) || is_hook(device, task)) {
		return INDIGO_IDLE_STATE;
	}
	switch (task->state) {
		case TASK_RUNNING:
			return INDIGO_BUSY_STATE;
		case TASK_DONE:
			return INDIGO_OK_STATE;
		case TASK_FAILED:
		case TASK_EXPIRED:
			return INDIGO_ALERT_STATE;
		default:
			return INDIGO_IDLE_STATE;
	}
}

// redefines the list only when its items or labels changed
static void update_task_state_property(indigo_device *device) {
	bool redefine = PRIVATE_DATA->task_state_count != PRIVATE_DATA->order_count;
	char (*labels)[INDIGO_VALUE_SIZE] = PRIVATE_DATA->label_buffer;
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		int slot = PRIVATE_DATA->order[i];
		build_task_state_label(device, PRIVATE_DATA->tasks + slot, labels[i], INDIGO_VALUE_SIZE);
		if (!redefine && (PRIVATE_DATA->task_state_slots[i] != slot || strcmp(PRIVATE_DATA->task_state_labels[i], labels[i]))) {
			redefine = true;
		}
	}
	if (redefine) {
		indigo_delete_property(device, TASK_STATE_PROPERTY, NULL);
		pthread_mutex_lock(&PRIVATE_DATA->property_mutex);
		for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
			char name[INDIGO_NAME_SIZE];
			snprintf(name, sizeof(name), AGENT_SCHEDULER_TASK_STATE_ITEM_NAME, PRIVATE_DATA->order[i]);
			indigo_init_light_item(TASK_STATE_PROPERTY->items + i, name, labels[i], task_light_state(device, PRIVATE_DATA->tasks + PRIVATE_DATA->order[i]));
			strcpy(PRIVATE_DATA->task_state_labels[i], labels[i]);
			PRIVATE_DATA->task_state_slots[i] = PRIVATE_DATA->order[i];
		}
		TASK_STATE_PROPERTY->count = PRIVATE_DATA->order_count;
		PRIVATE_DATA->task_state_count = PRIVATE_DATA->order_count;
		pthread_mutex_unlock(&PRIVATE_DATA->property_mutex);
		indigo_define_property(device, TASK_STATE_PROPERTY, NULL);
	} else {
		bool changed = false;
		for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
			indigo_property_state state = task_light_state(device, PRIVATE_DATA->tasks + PRIVATE_DATA->order[i]);
			if (TASK_STATE_PROPERTY->items[i].light.value != state) {
				pthread_mutex_lock(&PRIVATE_DATA->property_mutex);
				TASK_STATE_PROPERTY->items[i].light.value = state;
				pthread_mutex_unlock(&PRIVATE_DATA->property_mutex);
				changed = true;
			}
		}
		if (changed) {
			indigo_update_property(device, TASK_STATE_PROPERTY, NULL);
		}
	}
}

static void update_counters(indigo_device *device) {
	int done = 0, failed = 0;
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + PRIVATE_DATA->order[i];
		if (task->state == TASK_DONE) {
			done++;
		} else if (task->state == TASK_FAILED || task->state == TASK_EXPIRED) {
			failed++;
		}
	}
	if (STATE_DONE_ITEM->number.value != done || STATE_FAILED_ITEM->number.value != failed || STATE_TOTAL_ITEM->number.value != PRIVATE_DATA->order_count) {
		STATE_DONE_ITEM->number.value = done;
		STATE_FAILED_ITEM->number.value = failed;
		STATE_TOTAL_ITEM->number.value = PRIVATE_DATA->order_count;
		indigo_update_property(device, STATE_PROPERTY, NULL);
	}
}

static void set_scheduler_state(indigo_device *device, indigo_property_state state, const char *message) {
	STATE_PROPERTY->state = state;
	indigo_update_property(device, STATE_PROPERTY, message);
}

static void update_current(indigo_device *device) {
	const char *current = PRIVATE_DATA->current >= 0 && PRIVATE_DATA->phase != RUN_IDLE ? PRIVATE_DATA->tasks[PRIVATE_DATA->current].header.name : "";
	const char *next = PRIVATE_DATA->next_task >= 0 && PRIVATE_DATA->tasks[PRIVATE_DATA->next_task].used ? PRIVATE_DATA->tasks[PRIVATE_DATA->next_task].header.name : "";
	char next_start[64] = "";
	if (PRIVATE_DATA->next_start > 0) {
		indigo_timetoisogm(PRIVATE_DATA->next_start, next_start, sizeof(next_start));
	}
	if (strcmp(indigo_get_text_item_value(CURRENT_CURRENT_ITEM), current) || strcmp(indigo_get_text_item_value(CURRENT_NEXT_ITEM), next) || strcmp(indigo_get_text_item_value(CURRENT_NEXT_START_ITEM), next_start) || strcmp(indigo_get_text_item_value(CURRENT_REASON_ITEM), PRIVATE_DATA->reason)) {
		indigo_set_text_item_value(CURRENT_CURRENT_ITEM, current);
		indigo_set_text_item_value(CURRENT_NEXT_ITEM, next);
		indigo_set_text_item_value(CURRENT_NEXT_START_ITEM, next_start);
		indigo_set_text_item_value(CURRENT_REASON_ITEM, PRIVATE_DATA->reason);
		indigo_update_property(device, CURRENT_PROPERTY, NULL);
	}
}

static void refresh(indigo_device *device) {
	update_task_state_property(device);
	update_counters(device);
	update_current(device);
}

// -------------------------------------------------------------------------------- Task store

// the target grid and the table of failing points of a task
static void build_target(indigo_device *device, scheduler_task *task) {
	if (task->header.has_target && PRIVATE_DATA->sky.valid) {
		indigo_scheduler_target_build(&PRIVATE_DATA->sky, &task->header, &task->target);
	} else {
		task->target.valid = false;
	}
	indigo_scheduler_points_build(&PRIVATE_DATA->sky, &task->target, &task->header, &task->points);
	PRIVATE_DATA->plan_dirty = true;
}

static int create_task(indigo_device *device, const char *text, scheduler_header *header, bool stored) {
	int slot = -1;
	for (int i = 0; i < MAX_TASKS; i++) {
		if (!PRIVATE_DATA->tasks[i].used) {
			slot = i;
			break;
		}
	}
	if (slot < 0) {
		return -1;
	}
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	indigo_property *property = task->property;
	memset(task, 0, sizeof(*task));
	task->property = property;
	task->used = true;
	task->stored = stored;
	task->text = indigo_safe_malloc_copy(strlen(text) + 1, (void *)text);
	task->header = *header;
	task->state = TASK_QUEUED;
	task->urgent_order = ++PRIVATE_DATA->urgent_counter;
	task->estimate_needed = true;
	build_target(device, task);
	PRIVATE_DATA->order[PRIVATE_DATA->order_count++] = slot;
	if (stored) {
		define_task_property(device, slot);
	}
	return slot;
}

static void remove_task(indigo_device *device, int slot) {
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	if (task->stored) {
		indigo_delete_property(device, task->property, NULL);
	}
	free(task->text);
	free(task->pending_text);
	indigo_property *property = task->property;
	memset(task, 0, sizeof(*task));
	task->property = property;
	int index = order_index(device, slot);
	if (index >= 0) {
		memmove(PRIVATE_DATA->order + index, PRIVATE_DATA->order + index + 1, (PRIVATE_DATA->order_count - index - 1) * sizeof(int));
		PRIVATE_DATA->order_count--;
	}
	if (PRIVATE_DATA->next_task == slot) {
		PRIVATE_DATA->next_task = -1;
	}
	if (PRIVATE_DATA->estimate_slot == slot) {
		PRIVATE_DATA->estimate_slot = -1;
	}
}

static void replace_text(indigo_device *device, int slot, const char *text, scheduler_header *header) {
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	bool renamed = strcmp(task->header.name, header->name) != 0;
	free(task->text);
	task->text = indigo_safe_malloc_copy(strlen(text) + 1, (void *)text);
	task->header = *header;
	// the measured and estimated durations belong to the old text
	task->measured = 0;
	task->estimated = false;
	task->estimate_needed = true;
	task->script_error = false;
	// a result of the old text is ignored
	if (PRIVATE_DATA->estimate_slot == slot) {
		PRIVATE_DATA->estimate_slot = -1;
	}
	build_target(device, task);
	if (renamed) {
		indigo_delete_property(device, task->property, NULL);
		define_task_property(device, slot);
	} else {
		pthread_mutex_lock(&PRIVATE_DATA->property_mutex);
		indigo_set_text_item_value(task->property->items, task->text);
		task->property->state = INDIGO_OK_STATE;
		pthread_mutex_unlock(&PRIVATE_DATA->property_mutex);
		indigo_update_property(device, task->property, NULL);
	}
}

// -------------------------------------------------------------------------------- Running tasks

static void set_reason(indigo_device *device, const char *reason) {
	INDIGO_COPY_VALUE(PRIVATE_DATA->reason, reason ? reason : "");
}

static void apply_pending_edit(indigo_device *device, scheduler_task *task, int slot) {
	if (task->pending_text) {
		char *text = task->pending_text;
		task->pending_text = NULL;
		replace_text(device, slot, text, &task->pending_header);
		free(text);
	}
}

static void request_stop(indigo_device *device, indigo_property_state state, const char *text) {
	PRIVATE_DATA->stop_requested = true;
	PRIVATE_DATA->automatic_stop = false;
	PRIVATE_DATA->stop_state = state;
	INDIGO_COPY_VALUE(PRIVATE_DATA->stop_text, text);
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
}

static void stop_scheduler(indigo_device *device, indigo_property_state state, const char *message) {
	PRIVATE_DATA->started = false;
	PRIVATE_DATA->stop_requested = false;
	PRIVATE_DATA->startup_pending = false;
	PRIVATE_DATA->shutdown_ran = false;
	set_scheduler_state(device, state, message);
}

// a hook runs outside the state machine of its task
static void finish_hook(indigo_device *device, run_result result, const char *message) {
	int slot = PRIVATE_DATA->current;
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	run_kind kind = PRIVATE_DATA->kind;
	add_history(device, task, result, message);
	PRIVATE_DATA->phase = RUN_IDLE;
	PRIVATE_DATA->current = -1;
	PRIVATE_DATA->kind = RUN_TASK;
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
	if (result != RESULT_SUCCEEDED) {
		indigo_send_message(device, NULL, "Hook '%s' %s%s%s", task->header.name, run_result_names[result], message && *message ? ": " : "", message ? message : "");
	} else {
		indigo_send_message(device, NULL, "Hook '%s' completed", task->header.name);
	}
	switch (kind) {
		case RUN_STARTUP_HOOK:
			// an ABORT during the startup hook already requested the stop, which runs the shutdown hook
			if (result != RESULT_SUCCEEDED && !PRIVATE_DATA->stop_requested) {
				stop_scheduler(device, INDIGO_ALERT_STATE, "The startup hook failed, the scheduler stopped");
			}
			break;
		case RUN_SHUTDOWN_HOOK:
			if (PRIVATE_DATA->stop_requested) {
				stop_scheduler(device, result == RESULT_SUCCEEDED ? PRIVATE_DATA->stop_state : INDIGO_ALERT_STATE, PRIVATE_DATA->stop_text);
			} else {
				// START during the shutdown hook continues scheduling, after the startup hook prepares again
				PRIVATE_DATA->shutdown_ran = false;
				PRIVATE_DATA->startup_pending = *indigo_get_text_item_value(HOOKS_STARTUP_ITEM) != 0;
			}
			break;
		case RUN_IDLE_HOOK:
			PRIVATE_DATA->idle_hook_done = true;
			break;
		case RUN_TASK:
			break;
	}
	apply_pending_edit(device, task, slot);
	schedule_save(device);
}

static void finish_task(indigo_device *device, bool success, const char *message) {
	if (PRIVATE_DATA->kind != RUN_TASK) {
		finish_hook(device, success ? RESULT_SUCCEEDED : RESULT_FAILED, message);
		return;
	}
	int slot = PRIVATE_DATA->current;
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	time_t now = time(NULL);
	add_history(device, task, success ? RESULT_SUCCEEDED : RESULT_FAILED, message);
	if (success) {
		task->runs++;
		// the real duration includes what the estimate cannot know: flips, focusing retries, downloads
		task->measured = (int)(now - PRIVATE_DATA->run_start);
		if (task->runs >= task->header.repeat) {
			task->state = TASK_DONE;
			indigo_send_message(device, NULL, "Task '%s' completed", task->header.name);
		} else {
			task->state = TASK_REPEAT;
			task->delay_until = now + task->header.repeat_every * 60;
			indigo_send_message(device, NULL, "Task '%s' completed run %d of %d", task->header.name, task->runs, task->header.repeat);
		}
	} else if (task->retries_used < task->header.retry) {
		task->retries_used++;
		task->state = TASK_RETRY;
		task->delay_until = now + task->header.retry_after * 60;
		indigo_send_message(device, NULL, "Task '%s' failed%s%s, it will be retried", task->header.name, message && *message ? ": " : "", message ? message : "");
	} else {
		task->state = TASK_FAILED;
		indigo_send_message(device, NULL, "Task '%s' failed%s%s", task->header.name, message && *message ? ": " : "", message ? message : "");
	}
	PRIVATE_DATA->phase = RUN_IDLE;
	PRIVATE_DATA->current = -1;
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
	if (!task->stored) {
		remove_task(device, slot);
	} else {
		apply_pending_edit(device, task, slot);
	}
	schedule_save(device);
}

static void finish_stop(indigo_device *device) {
	int slot = PRIVATE_DATA->current;
	if (slot < 0) {
		PRIVATE_DATA->phase = RUN_IDLE;
		PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
		return;
	}
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	if (PRIVATE_DATA->stop == STOP_FAILURE) {
		// the sequence the failed body had started was aborted, the failure follows the retry rules
		finish_task(device, false, PRIVATE_DATA->stop_message);
		return;
	}
	if (PRIVATE_DATA->kind != RUN_TASK) {
		bool deleted = PRIVATE_DATA->stop == STOP_DELETE;
		finish_hook(device, RESULT_ABORTED, deleted ? "deleted" : "");
		if (deleted) {
			remove_task(device, slot);
		}
		return;
	}
	PRIVATE_DATA->phase = RUN_IDLE;
	PRIVATE_DATA->current = -1;
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
	switch (PRIVATE_DATA->stop) {
		case STOP_DELETE:
			add_history(device, task, RESULT_ABORTED, "deleted");
			indigo_send_message(device, NULL, "Task '%s' aborted and deleted", task->header.name);
			remove_task(device, slot);
			break;
		case STOP_PREEMPT:
			add_history(device, task, RESULT_PREEMPTED, PRIVATE_DATA->stop_message);
			if (task->header.preempted_fail) {
				task->state = TASK_FAILED;
				indigo_send_message(device, NULL, "Task '%s' preempted by an urgent task and failed", task->header.name);
			} else {
				task->state = TASK_RETRY;
				task->delay_until = time(NULL) + task->header.retry_after * 60;
				indigo_send_message(device, NULL, "Task '%s' preempted by an urgent task, it will be retried", task->header.name);
			}
			break;
		case STOP_ABORT:
			add_history(device, task, RESULT_ABORTED, PRIVATE_DATA->stop_message);
			task->state = TASK_RETRY;
			task->delay_until = 0;
			indigo_send_message(device, NULL, "Task '%s' aborted", task->header.name);
			break;
		case STOP_FAILURE:
			break;
	}
	if (task->used) {
		if (!task->stored) {
			remove_task(device, slot);
		} else {
			apply_pending_edit(device, task, slot);
		}
	}
	schedule_save(device);
}

// the abort requests are sent from the handler without holding any lock of the agent
static void send_abort(indigo_device *device) {
	indigo_change_switch_property_1(agent_client, SCRIPTING_AGENT, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
	indigo_change_switch_property_1(agent_client, GUIDER_AGENT, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
}

static void begin_stop(indigo_device *device, stop_reason reason) {
	if (PRIVATE_DATA->phase == RUN_STARTING) {
		// the Busy update of the sequence may still be queued in the Scripting Agent
		pthread_mutex_lock(&PRIVATE_DATA->observed.mutex);
		PRIVATE_DATA->stop_waits_for_start = PRIVATE_DATA->observed.busy_events == PRIVATE_DATA->busy_baseline;
		pthread_mutex_unlock(&PRIVATE_DATA->observed.mutex);
	} else {
		PRIVATE_DATA->stop_waits_for_start = false;
	}
	PRIVATE_DATA->stop = reason;
	PRIVATE_DATA->phase = RUN_STOPPING;
	PRIVATE_DATA->deadline = indigo_monotonic_time() + SETTINGS_STOP_TIMEOUT_ITEM->number.value;
	set_reason(device, "stopping the running sequence");
	send_abort(device);
}

static void start_task(indigo_device *device, int slot, run_kind kind) {
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	int token = ++PRIVATE_DATA->token;
	char *script = wrapper_script(device, task, token, false);
	pthread_mutex_lock(&PRIVATE_DATA->observed.mutex);
	PRIVATE_DATA->busy_baseline = PRIVATE_DATA->observed.busy_events;
	PRIVATE_DATA->finish_baseline = PRIVATE_DATA->observed.finish_events;
	PRIVATE_DATA->observed.run_token = token;
	PRIVATE_DATA->observed.run_received = false;
	pthread_mutex_unlock(&PRIVATE_DATA->observed.mutex);
	PRIVATE_DATA->result_received = false;
	PRIVATE_DATA->current = slot;
	PRIVATE_DATA->kind = kind;
	PRIVATE_DATA->run_start = time(NULL);
	PRIVATE_DATA->run_number = kind == RUN_TASK ? task->runs + 1 : 1;
	PRIVATE_DATA->phase = RUN_STARTING;
	PRIVATE_DATA->deadline = indigo_monotonic_time() + SETTINGS_START_TIMEOUT_ITEM->number.value;
	PRIVATE_DATA->plan_dirty = true;
	set_reason(device, "");
	if (kind == RUN_TASK) {
		task->state = TASK_RUNNING;
		PRIVATE_DATA->idle_since = 0;
		PRIVATE_DATA->idle_hook_done = false;
		indigo_send_message(device, NULL, "Starting task '%s'", task->header.name);
	} else {
		indigo_send_message(device, NULL, "Starting hook '%s'", task->header.name);
	}
	refresh(device);
	const char *items[] = { AGENT_SCRIPTING_RUN_SCRIPT_ITEM_NAME };
	const char *values[] = { script };
	indigo_change_text_property(agent_client, SCRIPTING_AGENT, AGENT_SCRIPTING_RUN_SCRIPT_PROPERTY_NAME, 1, items, values);
	free(script);
}

static void load_library(indigo_device *device) {
	PRIVATE_DATA->library_sent = indigo_monotonic_time();
	INDIGO_DRIVER_LOG(DRIVER_NAME, "Loading Sequencer.js into %s", SCRIPTING_AGENT);
	const char *items[] = { AGENT_SCRIPTING_RUN_SCRIPT_ITEM_NAME };
	const char *values[] = { sequencer_js };
	indigo_change_text_property(agent_client, SCRIPTING_AGENT, AGENT_SCRIPTING_RUN_SCRIPT_PROPERTY_NAME, 1, items, values);
}

// -------------------------------------------------------------------------------- Selection

static void refresh_sky(indigo_device *device, time_t now) {
	scheduler_observed *observed = &PRIVATE_DATA->observed;
	pthread_mutex_lock(&observed->mutex);
	bool known = observed->site_known;
	double latitude = observed->latitude, longitude = observed->longitude, elevation = observed->elevation;
	pthread_mutex_unlock(&observed->mutex);
	scheduler_sky_grid *sky = &PRIVATE_DATA->sky;
	if (!known) {
		if (sky->valid) {
			sky->valid = false;
			for (int i = 0; i < MAX_TASKS; i++) {
				PRIVATE_DATA->tasks[i].target.valid = false;
				PRIVATE_DATA->tasks[i].points.valid = false;
			}
			PRIVATE_DATA->plan_dirty = true;
		}
		return;
	}
	if (sky->valid && sky->latitude == latitude && sky->longitude == longitude && sky->elevation == elevation && now - sky->start < SKY_REFRESH_INTERVAL) {
		return;
	}
	indigo_scheduler_sky_build(sky, now, latitude, longitude, elevation);
	for (int i = 0; i < MAX_TASKS; i++) {
		if (PRIVATE_DATA->tasks[i].used) {
			build_target(device, PRIVATE_DATA->tasks + i);
		}
	}
}

static bool eligible_now(indigo_device *device, scheduler_task *task, time_t now, const char **reason) {
	int duration = task_duration(task, NULL);
	return indigo_scheduler_points_ok(&PRIVATE_DATA->sky, &task->points, &task->header, now, now + duration, reason);
}

static double score(indigo_device *device, scheduler_task *task, time_t now) {
	return indigo_scheduler_score(&PRIVATE_DATA->sky, &task->target, &task->points, &task->header, now, SETTINGS_SETTING_WEIGHT_ITEM->number.value, SETTINGS_ALTITUDE_WEIGHT_ITEM->number.value);
}

// a task waits for its estimate, which also finds script errors before the first run
static bool estimate_pending(indigo_device *device, int slot) {
	return PRIVATE_DATA->tasks[slot].estimate_needed || PRIVATE_DATA->estimate_slot == slot;
}

// updates delays and expiry of all tasks
static void update_task_states(indigo_device *device, time_t now) {
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		int slot = PRIVATE_DATA->order[i];
		scheduler_task *task = PRIVATE_DATA->tasks + slot;
		if (is_hook(device, task)) {
			continue;
		}
		if ((task->state == TASK_RETRY || task->state == TASK_REPEAT) && now >= task->delay_until) {
			task->state = TASK_QUEUED;
			task->urgent_order = ++PRIVATE_DATA->urgent_counter;
			PRIVATE_DATA->plan_dirty = true;
		}
		if (task->state != TASK_RUNNING && !is_terminal_state(task->state) && indigo_scheduler_window_expired(&task->header, now)) {
			task->state = TASK_EXPIRED;
			PRIVATE_DATA->plan_dirty = true;
			indigo_send_message(device, NULL, "Task '%s' expired", task->header.name);
			schedule_save(device);
			if (!task->stored) {
				remove_task(device, slot);
				i--;
			}
		}
	}
}

// selects the eligible urgent task submitted first, -1 if none
static int select_urgent(indigo_device *device, time_t now) {
	int best = -1;
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		int slot = PRIVATE_DATA->order[i];
		scheduler_task *task = PRIVATE_DATA->tasks + slot;
		const char *reason;
		if (is_urgent(task) && !is_hook(device, task) && !task->script_error && !estimate_pending(device, slot) && is_selectable_state(task->state) && is_enabled(task) && eligible_now(device, task, now, &reason)) {
			if (best < 0 || task->urgent_order < PRIVATE_DATA->tasks[best].urgent_order) {
				best = slot;
			}
		}
	}
	return best;
}

// selects the task to run now, -1 if none; publishes why nothing can run and when the next task can
static int select_task(indigo_device *device, time_t now) {
	int best = select_urgent(device, now);
	double best_score = 0;
	const char *first_reason = NULL;
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		int slot = PRIVATE_DATA->order[i];
		scheduler_task *task = PRIVATE_DATA->tasks + slot;
		if (!is_selectable_state(task->state) || !is_enabled(task) || task->script_error || is_hook(device, task)) {
			continue;
		}
		const char *reason = NULL;
		if (estimate_pending(device, slot)) {
			if (first_reason == NULL) {
				first_reason = "estimating the task duration";
			}
			PRIVATE_DATA->dirty = true;
			continue;
		}
		if (!eligible_now(device, task, now, &reason)) {
			task->state = TASK_WAITING;
			if (first_reason == NULL) {
				first_reason = reason;
			}
			continue;
		}
		task->state = TASK_QUEUED;
		if (best >= 0 && is_urgent(PRIVATE_DATA->tasks + best)) {
			continue;
		}
		double value = score(device, task, now);
		if (best < 0 || value > best_score) {
			best = slot;
			best_score = value;
		}
	}
	PRIVATE_DATA->next_task = best;
	PRIVATE_DATA->next_start = best >= 0 ? now : 0;
	if (best >= 0) {
		set_reason(device, "");
		return best;
	}
	// nothing eligible now, find the task that becomes eligible first
	time_t next_start = 0;
	int next_task = -1;
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		int slot = PRIVATE_DATA->order[i];
		scheduler_task *task = PRIVATE_DATA->tasks + slot;
		if (!is_enabled(task) || is_terminal_state(task->state) || task->state == TASK_RUNNING || task->script_error || is_hook(device, task)) {
			continue;
		}
		time_t from = now;
		if ((task->state == TASK_RETRY || task->state == TASK_REPEAT) && task->delay_until > now) {
			from = task->delay_until;
		}
		const char *reason;
		time_t start = indigo_scheduler_points_next(&PRIVATE_DATA->sky, &task->points, &task->header, from, task_duration(task, NULL), 0, &reason);
		if (start > 0 && (next_start == 0 || start < next_start)) {
			next_start = start;
			next_task = slot;
		}
	}
	PRIVATE_DATA->next_task = next_task;
	PRIVATE_DATA->next_start = next_start;
	if (PRIVATE_DATA->order_count == 0) {
		set_reason(device, "no tasks");
	} else if (first_reason) {
		set_reason(device, first_reason);
	} else {
		set_reason(device, "no task is ready");
	}
	return -1;
}

// an empty list keeps the scheduler running, ready for urgent tasks
static bool all_tasks_finished(indigo_device *device) {
	if (PRIVATE_DATA->order_count == 0) {
		return false;
	}
	bool any = false;
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + PRIVATE_DATA->order[i];
		if (is_hook(device, task)) {
			continue;
		}
		any = true;
		if (is_enabled(task) && !is_terminal_state(task->state) && !task->script_error) {
			return false;
		}
	}
	return any;
}

// -------------------------------------------------------------------------------- State machine

static void process_starting(indigo_device *device) {
	scheduler_observed *observed = &PRIVATE_DATA->observed;
	pthread_mutex_lock(&observed->mutex);
	bool result = observed->run_received;
	bool result_ok = observed->run_ok;
	char message[INDIGO_VALUE_SIZE];
	INDIGO_COPY_VALUE(message, observed->run_message);
	bool started = observed->busy_events != PRIVATE_DATA->busy_baseline;
	pthread_mutex_unlock(&observed->mutex);
	if (result) {
		PRIVATE_DATA->result_received = true;
		if (!result_ok) {
			if (started) {
				// the body threw after starting its sequence, which must not keep running unowned
				INDIGO_COPY_VALUE(PRIVATE_DATA->stop_message, message);
				begin_stop(device, STOP_FAILURE);
			} else {
				finish_task(device, false, message);
			}
			return;
		}
	}
	if (PRIVATE_DATA->result_received && started) {
		PRIVATE_DATA->phase = RUN_RUNNING;
		return;
	}
	if (indigo_monotonic_time() > PRIVATE_DATA->deadline) {
		if (started) {
			INDIGO_COPY_VALUE(PRIVATE_DATA->stop_message, "no result from the Scripting Agent");
			begin_stop(device, STOP_FAILURE);
		} else {
			finish_task(device, false, PRIVATE_DATA->result_received ? "the sequence was not started" : "no result from the Scripting Agent");
		}
	}
}

static void process_running(indigo_device *device, time_t now) {
	scheduler_observed *observed = &PRIVATE_DATA->observed;
	pthread_mutex_lock(&observed->mutex);
	bool finished = observed->finish_events != PRIVATE_DATA->finish_baseline;
	bool success = observed->finish_state == INDIGO_OK_STATE;
	bool present = observed->scripting_present && observed->sequencer_present;
	char message[INDIGO_VALUE_SIZE];
	INDIGO_COPY_VALUE(message, observed->finish_message);
	pthread_mutex_unlock(&observed->mutex);
	if (finished) {
		finish_task(device, success, message);
		return;
	}
	if (!present) {
		finish_task(device, false, "the Scripting Agent or Sequencer.js disappeared");
		return;
	}
	scheduler_task *task = PRIVATE_DATA->tasks + PRIVATE_DATA->current;
	if (PRIVATE_DATA->kind == RUN_TASK && !is_urgent(task) && (PRIVATE_DATA->dirty || indigo_monotonic_time() >= PRIVATE_DATA->next_check)) {
		PRIVATE_DATA->dirty = false;
		PRIVATE_DATA->next_check = indigo_monotonic_time() + SETTINGS_IDLE_CHECK_ITEM->number.value;
		int urgent = PRIVATE_DATA->started && !PRIVATE_DATA->stop_requested ? select_urgent(device, now) : -1;
		if (urgent >= 0) {
			snprintf(PRIVATE_DATA->stop_message, sizeof(PRIVATE_DATA->stop_message), "by %s", PRIVATE_DATA->tasks[urgent].header.name);
			begin_stop(device, STOP_PREEMPT);
		}
	}
}

static void process_stopping(indigo_device *device) {
	scheduler_observed *observed = &PRIVATE_DATA->observed;
	pthread_mutex_lock(&observed->mutex);
	bool started = observed->busy_events != PRIVATE_DATA->busy_baseline;
	bool finished = observed->finish_events != PRIVATE_DATA->finish_baseline;
	bool result = observed->run_received;
	bool sequence_idle = observed->sequence_state != INDIGO_BUSY_STATE;
	bool agents_idle = observed->agents_busy == 0;
	pthread_mutex_unlock(&observed->mutex);
	if (PRIVATE_DATA->current >= 0) {
		if (started) {
			// the state of the sequence may be stale, its end is counted
			sequence_idle = finished;
		} else if (PRIVATE_DATA->stop_waits_for_start && !result) {
			// the body has not reported yet, its sequence may still start
			sequence_idle = false;
		}
	}
	if (sequence_idle && agents_idle) {
		finish_stop(device);
		return;
	}
	if (indigo_monotonic_time() > PRIVATE_DATA->deadline) {
		if (PRIVATE_DATA->stop != STOP_FAILURE) {
			INDIGO_COPY_VALUE(PRIVATE_DATA->stop_message, "did not stop in time");
		}
		finish_stop(device);
		// another task or hook cannot start while the old sequence runs
		stop_scheduler(device, INDIGO_ALERT_STATE, "The sequence or an agent did not stop in time, the scheduler stopped");
	}
}

// the slot of a configured hook, -1 if it is not set or names no task
static int hook_slot(indigo_device *device, indigo_item *item) {
	const char *name = indigo_get_text_item_value(item);
	return *name ? find_task(device, name) : -1;
}

static void process_idle(indigo_device *device, time_t now) {
	if (!PRIVATE_DATA->started) {
		return;
	}
	scheduler_observed *observed = &PRIVATE_DATA->observed;
	pthread_mutex_lock(&observed->mutex);
	bool scripting = observed->scripting_present;
	bool sequencer = observed->sequencer_present;
	bool occupied = observed->sequence_state == INDIGO_BUSY_STATE;
	pthread_mutex_unlock(&observed->mutex);
	bool ready = scripting && sequencer && !occupied;
	// a task added before the automatic stop completed cancels it
	if (PRIVATE_DATA->stop_requested && PRIVATE_DATA->automatic_stop && !PRIVATE_DATA->shutdown_ran && !all_tasks_finished(device)) {
		PRIVATE_DATA->stop_requested = false;
		PRIVATE_DATA->automatic_stop = false;
	}
	if (PRIVATE_DATA->stop_requested) {
		// every stop runs the shutdown hook once, when it can run
		int slot = hook_slot(device, HOOKS_SHUTDOWN_ITEM);
		if (slot >= 0 && !PRIVATE_DATA->shutdown_ran && ready) {
			PRIVATE_DATA->shutdown_ran = true;
			start_task(device, slot, RUN_SHUTDOWN_HOOK);
		} else {
			if (slot < 0 && *indigo_get_text_item_value(HOOKS_SHUTDOWN_ITEM)) {
				indigo_send_message(device, NULL, "The shutdown hook '%s' names no task", indigo_get_text_item_value(HOOKS_SHUTDOWN_ITEM));
			} else if (slot >= 0 && !PRIVATE_DATA->shutdown_ran) {
				indigo_send_message(device, NULL, "The shutdown hook '%s' could not run, %s", PRIVATE_DATA->tasks[slot].header.name, occupied ? "another sequence is running" : "the Scripting Agent or Sequencer.js is missing");
			}
			stop_scheduler(device, PRIVATE_DATA->stop_state, PRIVATE_DATA->stop_text);
		}
		return;
	}
	if (!PRIVATE_DATA->dirty && indigo_monotonic_time() < PRIVATE_DATA->next_check) {
		return;
	}
	PRIVATE_DATA->dirty = false;
	PRIVATE_DATA->next_check = indigo_monotonic_time() + SETTINGS_IDLE_CHECK_ITEM->number.value;
	if (!scripting) {
		set_reason(device, "waiting for the " SCRIPTING_AGENT);
		return;
	}
	if (!sequencer) {
		set_reason(device, "loading Sequencer.js");
		PRIVATE_DATA->dirty = true;
		return;
	}
	if (PRIVATE_DATA->startup_pending) {
		if (occupied) {
			set_reason(device, "another sequence is running");
			return;
		}
		PRIVATE_DATA->startup_pending = false;
		int slot = hook_slot(device, HOOKS_STARTUP_ITEM);
		if (slot >= 0) {
			start_task(device, slot, RUN_STARTUP_HOOK);
			return;
		}
		indigo_send_message(device, NULL, "The startup hook '%s' names no task", indigo_get_text_item_value(HOOKS_STARTUP_ITEM));
	}
	if (all_tasks_finished(device)) {
		PRIVATE_DATA->next_task = -1;
		PRIVATE_DATA->next_start = 0;
		set_reason(device, "all tasks finished");
		request_stop(device, INDIGO_OK_STATE, "All tasks finished, the scheduler stopped");
		PRIVATE_DATA->automatic_stop = true;
		return;
	}
	int slot = select_task(device, now);
	if (occupied) {
		// a sequence started by hand occupies the Scripting Agent, only an urgent task may abort it
		if (slot >= 0 && is_urgent(PRIVATE_DATA->tasks + slot)) {
			PRIVATE_DATA->current = -1;
			PRIVATE_DATA->kind = RUN_TASK;
			INDIGO_COPY_VALUE(PRIVATE_DATA->stop_message, "");
			begin_stop(device, STOP_PREEMPT);
		} else {
			set_reason(device, "another sequence is running");
		}
		return;
	}
	if (slot >= 0) {
		start_task(device, slot, RUN_TASK);
		return;
	}
	// the idle hook runs once per idle period, for example to park and close; waiting for estimates is not idle
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		if (estimate_pending(device, PRIVATE_DATA->order[i])) {
			PRIVATE_DATA->idle_since = 0;
			return;
		}
	}
	if (PRIVATE_DATA->idle_since == 0) {
		PRIVATE_DATA->idle_since = now;
	}
	int idle = hook_slot(device, HOOKS_IDLE_ITEM);
	if (idle >= 0 && !PRIVATE_DATA->idle_hook_done && now - PRIVATE_DATA->idle_since >= SETTINGS_IDLE_PARK_AFTER_ITEM->number.value * 60) {
		start_task(device, idle, RUN_IDLE_HOOK);
	}
}

// estimates run one at a time, also while a task runs, because they never start a sequence
static void process_estimation(indigo_device *device) {
	scheduler_observed *observed = &PRIVATE_DATA->observed;
	pthread_mutex_lock(&observed->mutex);
	bool ready = observed->scripting_present && observed->sequencer_present;
	bool result = observed->estimate_received;
	bool ok = observed->estimate_ok;
	bool counted = observed->estimate_counts;
	double values[6];
	memcpy(values, observed->estimate_values, sizeof(values));
	char message[INDIGO_VALUE_SIZE];
	INDIGO_COPY_VALUE(message, observed->estimate_message);
	pthread_mutex_unlock(&observed->mutex);
	int slot = PRIVATE_DATA->estimate_slot;
	if (slot >= 0) {
		scheduler_task *task = PRIVATE_DATA->tasks + slot;
		if (!result && indigo_monotonic_time() < PRIVATE_DATA->estimate_deadline) {
			return;
		}
		PRIVATE_DATA->estimate_slot = -1;
		if (result && !ok) {
			// the body cannot run, the task stays out of the schedule until it is edited or reset
			task->script_error = true;
			task->estimated = false;
			indigo_send_message(device, NULL, "Task '%s' has a script error: %s", task->header.name, message);
		} else if (result && counted && values[0] > 0 && values[5] == 0 && isfinite(values[1] + values[2] * SETTINGS_FRAME_OVERHEAD_ITEM->number.value + values[3] * SETTINGS_OPERATION_OVERHEAD_ITEM->number.value + values[4])) {
			double estimate = values[1] + values[2] * SETTINGS_FRAME_OVERHEAD_ITEM->number.value + values[3] * SETTINGS_OPERATION_OVERHEAD_ITEM->number.value + values[4];
			task->script_error = false;
			task->estimated = true;
			task->estimate = (int)(estimate < 0 ? 0 : estimate > MAX_DURATION ? MAX_DURATION : estimate);
		} else {
			task->script_error = false;
			task->estimated = false;
			if (!task->header.has_duration && !is_hook(device, task)) {
				indigo_send_message(device, NULL, "The duration of task '%s' cannot be estimated, %d minutes are assumed until it has run; set @duration", task->header.name, SCHEDULER_DEFAULT_DURATION);
			}
		}
		PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
		return;
	}
	if (!ready) {
		return;
	}
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + PRIVATE_DATA->order[i];
		if (task->estimate_needed && task->state != TASK_RUNNING) {
			task->estimate_needed = false;
			int token = ++PRIVATE_DATA->token;
			char *script = wrapper_script(device, task, token, true);
			pthread_mutex_lock(&observed->mutex);
			observed->estimate_token = token;
			observed->estimate_received = false;
			observed->estimate_counts = false;
			pthread_mutex_unlock(&observed->mutex);
			PRIVATE_DATA->estimate_slot = PRIVATE_DATA->order[i];
			PRIVATE_DATA->estimate_deadline = indigo_monotonic_time() + ESTIMATE_TIMEOUT;
			const char *items[] = { AGENT_SCRIPTING_RUN_SCRIPT_ITEM_NAME };
			const char *values[] = { script };
			indigo_change_text_property(agent_client, SCRIPTING_AGENT, AGENT_SCRIPTING_RUN_SCRIPT_PROPERTY_NAME, 1, items, values);
			free(script);
			return;
		}
	}
}

// the library is loaded when the scheduler runs or a task needs an estimate
static void process_library(indigo_device *device) {
	scheduler_observed *observed = &PRIVATE_DATA->observed;
	pthread_mutex_lock(&observed->mutex);
	bool scripting = observed->scripting_present;
	bool sequencer = observed->sequencer_present;
	pthread_mutex_unlock(&observed->mutex);
	if (!scripting || sequencer) {
		return;
	}
	bool needed = PRIVATE_DATA->started;
	for (int i = 0; i < PRIVATE_DATA->order_count && !needed; i++) {
		needed = PRIVATE_DATA->tasks[PRIVATE_DATA->order[i]].estimate_needed;
	}
	if (needed && (PRIVATE_DATA->library_sent == 0 || indigo_monotonic_time() - PRIVATE_DATA->library_sent > LIBRARY_RETRY_INTERVAL)) {
		load_library(device);
	}
}

// -------------------------------------------------------------------------------- Night plan

static scheduler_plan_task plan_tasks[MAX_TASKS];
static int plan_slots[MAX_TASKS];
static scheduler_plan_run plan_runs[MAX_PLAN_RUNS];

static void plan_run_label(indigo_device *device, scheduler_plan_run *run, char *label, size_t size) {
	scheduler_task *task = PRIVATE_DATA->tasks + plan_slots[run->task];
	struct tm start, end;
	indigo_gmtime(&run->start, &start);
	indigo_gmtime(&run->end, &end);
	char priority[32];
	if (is_urgent(task)) {
		strcpy(priority, "urgent");
	} else {
		snprintf(priority, sizeof(priority), "%s %d", indigo_scheduler_priority_name(task->header.priority_class), task->header.level);
	}
	char text[INDIGO_VALUE_SIZE * 2];
	if (task->header.repeat > 1) {
		snprintf(text, sizeof(text), "%02d:%02d\xe2\x80\x93%02d:%02d  %s #%d (%s)", start.tm_hour, start.tm_min, end.tm_hour, end.tm_min, task->header.name, run->run, priority);
	} else {
		snprintf(text, sizeof(text), "%02d:%02d\xe2\x80\x93%02d:%02d  %s (%s)", start.tm_hour, start.tm_min, end.tm_hour, end.tm_min, task->header.name, priority);
	}
	cut_label(text, label, size);
}

static void publish_plan(indigo_device *device, char (*labels)[INDIGO_VALUE_SIZE], indigo_property_state *states, int count) {
	bool redefine = count != PRIVATE_DATA->plan_count;
	bool changed = false;
	for (int i = 0; i < count && !redefine; i++) {
		if (strcmp(labels[i], PRIVATE_DATA->plan_labels[i])) {
			redefine = true;
		} else if (states[i] != PRIVATE_DATA->plan_states[i]) {
			changed = true;
		}
	}
	if (redefine) {
		indigo_delete_property(device, PLAN_PROPERTY, NULL);
	}
	if (redefine || changed) {
		pthread_mutex_lock(&PRIVATE_DATA->property_mutex);
		for (int i = 0; i < count; i++) {
			if (redefine) {
				char name[INDIGO_NAME_SIZE];
				snprintf(name, sizeof(name), AGENT_SCHEDULER_PLAN_ITEM_NAME, i);
				indigo_init_light_item(PLAN_PROPERTY->items + i, name, labels[i], states[i]);
			} else {
				// the bus sends an update only for a value that differs from the previous one, which init would overwrite
				PLAN_PROPERTY->items[i].light.value = states[i];
			}
			strcpy(PRIVATE_DATA->plan_labels[i], labels[i]);
			PRIVATE_DATA->plan_states[i] = states[i];
		}
		PLAN_PROPERTY->count = count;
		PRIVATE_DATA->plan_count = count;
		pthread_mutex_unlock(&PRIVATE_DATA->property_mutex);
	}
	if (redefine) {
		indigo_define_property(device, PLAN_PROPERTY, NULL);
	} else if (changed) {
		indigo_update_property(device, PLAN_PROPERTY, NULL);
	}
}

// the forecast of the rest of the night: the runs the scheduler would make if every run took its duration and succeeded
static void update_plan(indigo_device *device, time_t now) {
	static char labels[MAX_PLAN_ITEMS][INDIGO_VALUE_SIZE];
	static indigo_property_state states[MAX_PLAN_ITEMS];
	int count = 0;
	for (int i = 0; i < PRIVATE_DATA->order_count; i++) {
		int slot = PRIVATE_DATA->order[i];
		scheduler_task *task = PRIVATE_DATA->tasks + slot;
		if (is_hook(device, task) || is_terminal_state(task->state)) {
			continue;
		}
		bool running = PRIVATE_DATA->phase != RUN_IDLE && PRIVATE_DATA->current == slot && PRIVATE_DATA->kind == RUN_TASK;
		scheduler_plan_task *plan_task = plan_tasks + count;
		plan_slots[count] = slot;
		plan_task->header = &task->header;
		plan_task->target = &task->target;
		plan_task->points = &task->points;
		plan_task->selectable = is_enabled(task) && !task->script_error;
		plan_task->duration = task_duration(task, NULL);
		plan_task->runs_left = task->header.repeat - task->runs;
		plan_task->available_from = (task->state == TASK_RETRY || task->state == TASK_REPEAT) ? task->delay_until : 0;
		plan_task->urgent_order = task->urgent_order;
		plan_task->running = running;
		plan_task->running_start = PRIVATE_DATA->run_start;
		plan_task->running_end = PRIVATE_DATA->run_start + plan_task->duration;
		if (running && plan_task->running_end < now + 60) {
			plan_task->running_end = now + 60;
		}
		count++;
	}
	time_t end = indigo_scheduler_night_end(&PRIVATE_DATA->sky, now);
	int runs = indigo_scheduler_plan(&PRIVATE_DATA->sky, plan_tasks, count, now, end, SETTINGS_SETTING_WEIGHT_ITEM->number.value, SETTINGS_ALTITUDE_WEIGHT_ITEM->number.value, plan_runs, MAX_PLAN_RUNS);
	bool planned[MAX_TASKS] = { false };
	int items = 0;
	for (int i = 0; i < runs && items < MAX_PLAN_ITEMS; i++) {
		plan_run_label(device, plan_runs + i, labels[items], INDIGO_VALUE_SIZE);
		states[items++] = plan_runs[i].running ? INDIGO_BUSY_STATE : INDIGO_IDLE_STATE;
		planned[plan_runs[i].task] = true;
	}
	// tasks without a run before the end of the night follow with the reason
	for (int i = 0; i < count && items < MAX_PLAN_ITEMS; i++) {
		if (planned[i]) {
			continue;
		}
		scheduler_task *task = PRIVATE_DATA->tasks + plan_slots[i];
		const char *reason = NULL;
		if (task->script_error) {
			reason = "script error";
		} else if (!is_enabled(task)) {
			reason = "disabled";
		} else {
			time_t from = plan_tasks[i].available_from > now ? plan_tasks[i].available_from : now;
			time_t start = indigo_scheduler_points_next(&PRIVATE_DATA->sky, &task->points, &task->header, from, plan_tasks[i].duration, end, &reason);
			if (start > 0 || reason == NULL) {
				reason = "no time left tonight";
			}
		}
		char text[INDIGO_VALUE_SIZE * 2];
		snprintf(text, sizeof(text), "%s \xe2\x80\x94 %s", task->header.name, reason);
		cut_label(text, labels[items], INDIGO_VALUE_SIZE);
		states[items++] = INDIGO_ALERT_STATE;
	}
	publish_plan(device, labels, states, items);
}

static void process(indigo_device *device) {
	time_t now = time(NULL);
	bool plan = PRIVATE_DATA->plan_dirty || indigo_monotonic_time() >= PRIVATE_DATA->plan_due;
	refresh_sky(device, now);
	process_library(device);
	process_estimation(device);
	if (PRIVATE_DATA->phase == RUN_IDLE || PRIVATE_DATA->dirty || indigo_monotonic_time() >= PRIVATE_DATA->next_check) {
		update_task_states(device, now);
	}
	// a stop finished in this pass lets late updates of the aborted sequence arrive before the next start
	bool stopped_now = false;
	switch (PRIVATE_DATA->phase) {
		case RUN_STARTING:
			process_starting(device);
			break;
		case RUN_RUNNING:
			process_running(device, now);
			break;
		case RUN_STOPPING:
			process_stopping(device);
			stopped_now = PRIVATE_DATA->phase == RUN_IDLE;
			break;
		case RUN_IDLE:
			break;
	}
	if (PRIVATE_DATA->phase == RUN_IDLE && !stopped_now) {
		process_idle(device, now);
	}
	refresh(device);
	if (plan || PRIVATE_DATA->plan_dirty) {
		update_plan(device, now);
		PRIVATE_DATA->plan_dirty = false;
		PRIVATE_DATA->plan_due = indigo_monotonic_time() + SETTINGS_IDLE_CHECK_ITEM->number.value;
	}
	if (PRIVATE_DATA->save_due > 0 && indigo_monotonic_time() >= PRIVATE_DATA->save_due) {
		save_config(device);
	}
}

static void tick_handler(indigo_device *device) {
	if (PRIVATE_DATA->shutting_down) {
		return;
	}
	process(device);
	indigo_execute_handler_in(device, TICK_INTERVAL, tick_handler);
}

// -------------------------------------------------------------------------------- Commands

static void reject(indigo_device *device, indigo_property *property, const char *format, ...) {
	char message[INDIGO_VALUE_SIZE];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	property->state = INDIGO_ALERT_STATE;
	indigo_update_property(device, property, "%s", message);
}

static void add_task(indigo_device *device, indigo_property *request, bool config_reader) {
	indigo_item *text_item = indigo_get_item(request, AGENT_SCHEDULER_ADD_TASK_TASK_ITEM_NAME);
	indigo_item *status_item = indigo_get_item(request, AGENT_SCHEDULER_ADD_TASK_STATUS_ITEM_NAME);
	const char *text = text_item ? indigo_get_text_item_value(text_item) : "";
	indigo_set_text_item_value(ADD_TASK_TASK_ITEM, "");
	indigo_set_text_item_value(ADD_TASK_STATUS_ITEM, "");
	scheduler_header header;
	char error[INDIGO_VALUE_SIZE];
	if (!indigo_scheduler_parse_header(text, &header, NULL, error, sizeof(error))) {
		if (config_reader) {
			indigo_send_message(device, NULL, "A saved task was skipped: %s", error);
		}
		reject(device, ADD_TASK_PROPERTY, "Task rejected: %s", error);
		return;
	}
	if (name_taken(device, header.name, -1)) {
		reject(device, ADD_TASK_PROPERTY, "Task rejected: a task named '%s' already exists", header.name);
		return;
	}
	int slot = create_task(device, text, &header, true);
	if (slot < 0) {
		reject(device, ADD_TASK_PROPERTY, "Task rejected: at most %d tasks can be stored", MAX_TASKS);
		return;
	}
	if (config_reader && status_item && *indigo_get_text_item_value(status_item)) {
		apply_status(PRIVATE_DATA->tasks + slot, indigo_get_text_item_value(status_item));
	} else {
		schedule_save(device);
	}
	ADD_TASK_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, ADD_TASK_PROPERTY, NULL);
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
}

static void edit_task(indigo_device *device, int slot, indigo_property *request) {
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	indigo_item *text_item = indigo_get_item(request, AGENT_SCHEDULER_TASK_TASK_ITEM_NAME);
	const char *text = text_item ? indigo_get_text_item_value(text_item) : "";
	scheduler_header header;
	char error[INDIGO_VALUE_SIZE];
	if (!indigo_scheduler_parse_header(text, &header, NULL, error, sizeof(error))) {
		reject(device, task->property, "Edit rejected: %s", error);
		return;
	}
	if (name_taken(device, header.name, slot)) {
		reject(device, task->property, "Edit rejected: a task named '%s' already exists", header.name);
		return;
	}
	if (task->state == TASK_RUNNING || (PRIVATE_DATA->phase != RUN_IDLE && PRIVATE_DATA->current == slot)) {
		free(task->pending_text);
		task->pending_text = indigo_safe_malloc_copy(strlen(text) + 1, (void *)text);
		task->pending_header = header;
		task->property->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, task->property, "The edit of '%s' takes effect after the task ends", task->header.name);
		return;
	}
	replace_text(device, slot, text, &header);
	schedule_save(device);
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
}

static void delete_task(indigo_device *device, indigo_property *request) {
	indigo_item *item = indigo_get_item(request, AGENT_SCHEDULER_DELETE_TASK_NAME_ITEM_NAME);
	const char *name = item ? indigo_get_text_item_value(item) : "";
	indigo_set_text_item_value(DELETE_TASK_NAME_ITEM, "");
	int slot = find_task(device, name);
	if (slot < 0) {
		reject(device, DELETE_TASK_PROPERTY, "Task '%s' not found", name);
		return;
	}
	if (PRIVATE_DATA->phase != RUN_IDLE && PRIVATE_DATA->current == slot) {
		if (PRIVATE_DATA->phase == RUN_STOPPING) {
			PRIVATE_DATA->stop = STOP_DELETE;
		} else {
			begin_stop(device, STOP_DELETE);
		}
	} else {
		remove_task(device, slot);
		schedule_save(device);
	}
	DELETE_TASK_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, DELETE_TASK_PROPERTY, NULL);
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
}

static void urgent_task(indigo_device *device, indigo_property *request) {
	indigo_item *text_item = indigo_get_item(request, AGENT_SCHEDULER_URGENT_TASK_TASK_ITEM_NAME);
	const char *text = text_item ? indigo_get_text_item_value(text_item) : "";
	indigo_set_text_item_value(URGENT_TASK_TASK_ITEM, "");
	scheduler_header header;
	char error[INDIGO_VALUE_SIZE];
	if (!indigo_scheduler_parse_header(text, &header, NULL, error, sizeof(error))) {
		reject(device, URGENT_TASK_PROPERTY, "Urgent task rejected: %s", error);
		return;
	}
	if (name_taken(device, header.name, -1)) {
		reject(device, URGENT_TASK_PROPERTY, "Urgent task rejected: a task named '%s' already exists", header.name);
		return;
	}
	header.priority_class = SCHEDULER_PRIORITY_URGENT;
	int slot = create_task(device, text, &header, false);
	if (slot < 0) {
		reject(device, URGENT_TASK_PROPERTY, "Urgent task rejected: too many tasks");
		return;
	}
	URGENT_TASK_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, URGENT_TASK_PROPERTY, PRIVATE_DATA->started ? NULL : "Urgent task queued, but the scheduler is not running");
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
}

static void task_command(indigo_device *device, indigo_property *request) {
	indigo_property_copy_values(TASK_COMMAND_PROPERTY, request, false);
	const char *name = indigo_get_text_item_value(TASK_NAME_NAME_ITEM);
	int slot = find_task(device, name);
	bool reset = TASK_COMMAND_RESET_ITEM->sw.value, enable = TASK_COMMAND_ENABLE_ITEM->sw.value, disable = TASK_COMMAND_DISABLE_ITEM->sw.value, up = TASK_COMMAND_UP_ITEM->sw.value, down = TASK_COMMAND_DOWN_ITEM->sw.value;
	for (int i = 0; i < TASK_COMMAND_PROPERTY->count; i++) {
		TASK_COMMAND_PROPERTY->items[i].sw.value = false;
	}
	if (slot < 0) {
		reject(device, TASK_COMMAND_PROPERTY, "Task '%s' not found", name);
		return;
	}
	scheduler_task *task = PRIVATE_DATA->tasks + slot;
	int index = order_index(device, slot);
	if (reset) {
		if (task->state == TASK_RUNNING || (PRIVATE_DATA->phase != RUN_IDLE && PRIVATE_DATA->current == slot)) {
			reject(device, TASK_COMMAND_PROPERTY, "Task '%s' is running and cannot be reset", name);
			return;
		}
		task->state = TASK_QUEUED;
		task->runs = 0;
		task->retries_used = 0;
		task->delay_until = 0;
		task->urgent_order = ++PRIVATE_DATA->urgent_counter;
		if (task->script_error) {
			task->script_error = false;
			task->estimate_needed = true;
		}
	} else if (enable) {
		task->disabled = false;
	} else if (disable) {
		task->disabled = true;
	} else if (up && index > 0) {
		PRIVATE_DATA->order[index] = PRIVATE_DATA->order[index - 1];
		PRIVATE_DATA->order[index - 1] = slot;
	} else if (down && index >= 0 && index < PRIVATE_DATA->order_count - 1) {
		PRIVATE_DATA->order[index] = PRIVATE_DATA->order[index + 1];
		PRIVATE_DATA->order[index + 1] = slot;
	}
	TASK_COMMAND_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, TASK_COMMAND_PROPERTY, NULL);
	schedule_save(device);
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
}

static void control(indigo_device *device, indigo_property *request) {
	indigo_property_copy_values(CONTROL_PROPERTY, request, false);
	bool start = CONTROL_START_ITEM->sw.value, stop = CONTROL_STOP_ITEM->sw.value, abort = CONTROL_ABORT_ITEM->sw.value;
	CONTROL_START_ITEM->sw.value = CONTROL_STOP_ITEM->sw.value = CONTROL_ABORT_ITEM->sw.value = false;
	CONTROL_PROPERTY->state = INDIGO_OK_STATE;
	indigo_update_property(device, CONTROL_PROPERTY, NULL);
	if (start) {
		if (!PRIVATE_DATA->started) {
			// the startup hook runs once per start, before the first task
			PRIVATE_DATA->startup_pending = *indigo_get_text_item_value(HOOKS_STARTUP_ITEM) != 0;
			PRIVATE_DATA->shutdown_ran = false;
			PRIVATE_DATA->idle_since = 0;
			PRIVATE_DATA->idle_hook_done = false;
		}
		PRIVATE_DATA->started = true;
		PRIVATE_DATA->stop_requested = false;
		set_scheduler_state(device, INDIGO_BUSY_STATE, "Scheduler started");
	} else if (stop) {
		if (PRIVATE_DATA->started) {
			request_stop(device, INDIGO_OK_STATE, "Scheduler stopped");
			if (PRIVATE_DATA->phase != RUN_IDLE) {
				indigo_send_message(device, NULL, "The scheduler stops after the running task");
			}
		}
	} else if (abort) {
		if (PRIVATE_DATA->phase != RUN_STOPPING || PRIVATE_DATA->stop != STOP_FAILURE) {
			INDIGO_COPY_VALUE(PRIVATE_DATA->stop_message, "");
		}
		if (PRIVATE_DATA->phase == RUN_STARTING || PRIVATE_DATA->phase == RUN_RUNNING) {
			begin_stop(device, STOP_ABORT);
		} else if (PRIVATE_DATA->phase == RUN_STOPPING && PRIVATE_DATA->stop == STOP_PREEMPT) {
			PRIVATE_DATA->stop = STOP_ABORT;
		}
		if (PRIVATE_DATA->started) {
			request_stop(device, INDIGO_ALERT_STATE, "Scheduler aborted");
		} else {
			stop_scheduler(device, INDIGO_ALERT_STATE, "Scheduler aborted");
		}
	}
	PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
}

static void command_handler(indigo_device *device, void *data) {
	scheduler_command *command = data;
	indigo_property *request = command->property;
	// INDIGO publishes only updates that change something, so the request is acknowledged as Busy first
	if (command->target->defined) {
		command->target->state = INDIGO_BUSY_STATE;
		indigo_update_property(device, command->target, NULL);
	}
	if (indigo_property_match_changeable(ADD_TASK_PROPERTY, request)) {
		add_task(device, request, command->config_reader);
	} else if (indigo_property_match_changeable(DELETE_TASK_PROPERTY, request)) {
		delete_task(device, request);
	} else if (indigo_property_match_changeable(URGENT_TASK_PROPERTY, request)) {
		urgent_task(device, request);
	} else if (indigo_property_match_changeable(TASK_NAME_PROPERTY, request)) {
		indigo_property_copy_values(TASK_NAME_PROPERTY, request, false);
		TASK_NAME_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, TASK_NAME_PROPERTY, NULL);
	} else if (indigo_property_match_changeable(TASK_COMMAND_PROPERTY, request)) {
		task_command(device, request);
	} else if (indigo_property_match_changeable(CONTROL_PROPERTY, request)) {
		control(device, request);
	} else if (indigo_property_match_changeable(HOOKS_PROPERTY, request)) {
		indigo_property_copy_values(HOOKS_PROPERTY, request, false);
		HOOKS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, HOOKS_PROPERTY, NULL);
		if (!command->config_reader) {
			schedule_save(device);
		}
		PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
	} else if (indigo_property_match_changeable(CLEAR_HISTORY_PROPERTY, request)) {
		PRIVATE_DATA->history_count = 0;
		update_history_property(device);
		CLEAR_HISTORY_ITEM->sw.value = false;
		CLEAR_HISTORY_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, CLEAR_HISTORY_PROPERTY, NULL);
		schedule_save(device);
	} else if (indigo_property_match_changeable(RESTORE_HISTORY_PROPERTY, request)) {
		if (command->config_reader) {
			indigo_item *item = indigo_get_item(request, AGENT_SCHEDULER_RESTORE_HISTORY_ENTRIES_ITEM_NAME);
			restore_history(device, item ? indigo_get_text_item_value(item) : "");
			RESTORE_HISTORY_PROPERTY->state = INDIGO_OK_STATE;
			indigo_update_property(device, RESTORE_HISTORY_PROPERTY, NULL);
		} else {
			reject(device, RESTORE_HISTORY_PROPERTY, "The history is restored only from the configuration");
		}
	} else if (indigo_property_match_changeable(SETTINGS_PROPERTY, request)) {
		indigo_property_copy_values(SETTINGS_PROPERTY, request, false);
		SETTINGS_PROPERTY->state = INDIGO_OK_STATE;
		indigo_update_property(device, SETTINGS_PROPERTY, NULL);
		if (!command->config_reader) {
			schedule_save(device);
		}
		PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
	} else {
		for (int i = 0; i < MAX_TASKS; i++) {
			scheduler_task *task = PRIVATE_DATA->tasks + i;
			if (task->used && task->stored && indigo_property_match_changeable(task->property, request)) {
				edit_task(device, i, request);
				break;
			}
		}
	}
	indigo_release_property(request);
	free(command);
	process(device);
}

// -------------------------------------------------------------------------------- INDIGO agent device implementation

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property);

static indigo_result agent_device_attach(indigo_device *device) {
	assert(device != NULL);
	assert(PRIVATE_DATA != NULL);
	if (indigo_device_attach(device, DRIVER_NAME, DRIVER_VERSION, INDIGO_INTERFACE_AGENT) == INDIGO_OK) {
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_ADD_TASK
		ADD_TASK_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_ADD_TASK_PROPERTY_NAME, SCHEDULER_GROUP, "Add task", INDIGO_OK_STATE, INDIGO_RW_PERM, 2);
		if (ADD_TASK_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item_raw(ADD_TASK_TASK_ITEM, AGENT_SCHEDULER_ADD_TASK_TASK_ITEM_NAME, "Task", "");
		indigo_init_text_item(ADD_TASK_STATUS_ITEM, AGENT_SCHEDULER_ADD_TASK_STATUS_ITEM_NAME, "Status (configuration only)", "");
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_DELETE_TASK
		DELETE_TASK_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_DELETE_TASK_PROPERTY_NAME, SCHEDULER_GROUP, "Delete task", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (DELETE_TASK_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(DELETE_TASK_NAME_ITEM, AGENT_SCHEDULER_DELETE_TASK_NAME_ITEM_NAME, "Name", "");
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_URGENT_TASK
		URGENT_TASK_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_URGENT_TASK_PROPERTY_NAME, SCHEDULER_GROUP, "Submit urgent task", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (URGENT_TASK_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item_raw(URGENT_TASK_TASK_ITEM, AGENT_SCHEDULER_URGENT_TASK_TASK_ITEM_NAME, "Task", "");
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_TASK_STATE
		TASK_STATE_PROPERTY = indigo_init_light_property(NULL, device->name, AGENT_SCHEDULER_TASK_STATE_PROPERTY_NAME, SCHEDULER_GROUP, "Tasks", INDIGO_OK_STATE, MAX_TASKS);
		if (TASK_STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		TASK_STATE_PROPERTY->count = 0;
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_TASK_NAME
		TASK_NAME_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_TASK_NAME_PROPERTY_NAME, SCHEDULER_GROUP, "Selected task", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (TASK_NAME_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(TASK_NAME_NAME_ITEM, AGENT_SCHEDULER_TASK_NAME_NAME_ITEM_NAME, "Name", "");
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_TASK_COMMAND
		TASK_COMMAND_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SCHEDULER_TASK_COMMAND_PROPERTY_NAME, SCHEDULER_GROUP, "Task command", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 5);
		if (TASK_COMMAND_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(TASK_COMMAND_RESET_ITEM, AGENT_SCHEDULER_TASK_COMMAND_RESET_ITEM_NAME, "Reset", false);
		indigo_init_switch_item(TASK_COMMAND_ENABLE_ITEM, AGENT_SCHEDULER_TASK_COMMAND_ENABLE_ITEM_NAME, "Enable", false);
		indigo_init_switch_item(TASK_COMMAND_DISABLE_ITEM, AGENT_SCHEDULER_TASK_COMMAND_DISABLE_ITEM_NAME, "Disable", false);
		indigo_init_switch_item(TASK_COMMAND_UP_ITEM, AGENT_SCHEDULER_TASK_COMMAND_MOVE_UP_ITEM_NAME, "Move up", false);
		indigo_init_switch_item(TASK_COMMAND_DOWN_ITEM, AGENT_SCHEDULER_TASK_COMMAND_MOVE_DOWN_ITEM_NAME, "Move down", false);
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_CONTROL
		CONTROL_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SCHEDULER_CONTROL_PROPERTY_NAME, SCHEDULER_GROUP, "Scheduler control", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 3);
		if (CONTROL_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(CONTROL_START_ITEM, AGENT_SCHEDULER_CONTROL_START_ITEM_NAME, "Start", false);
		indigo_init_switch_item(CONTROL_STOP_ITEM, AGENT_SCHEDULER_CONTROL_STOP_ITEM_NAME, "Stop after the running task", false);
		indigo_init_switch_item(CONTROL_ABORT_ITEM, AGENT_SCHEDULER_CONTROL_ABORT_ITEM_NAME, "Abort", false);
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_STATE
		STATE_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_SCHEDULER_STATE_PROPERTY_NAME, SCHEDULER_GROUP, "Scheduler state", INDIGO_IDLE_STATE, INDIGO_RO_PERM, 3);
		if (STATE_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(STATE_DONE_ITEM, AGENT_SCHEDULER_STATE_DONE_ITEM_NAME, "Done", 0, MAX_TASKS, 1, 0);
		indigo_init_number_item(STATE_FAILED_ITEM, AGENT_SCHEDULER_STATE_FAILED_ITEM_NAME, "Failed or expired", 0, MAX_TASKS, 1, 0);
		indigo_init_number_item(STATE_TOTAL_ITEM, AGENT_SCHEDULER_STATE_TOTAL_ITEM_NAME, "Total", 0, MAX_TASKS, 1, 0);
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_CURRENT
		CURRENT_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_CURRENT_PROPERTY_NAME, SCHEDULER_GROUP, "Current task", INDIGO_OK_STATE, INDIGO_RO_PERM, 4);
		if (CURRENT_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(CURRENT_CURRENT_ITEM, AGENT_SCHEDULER_CURRENT_CURRENT_ITEM_NAME, "Running task", "");
		indigo_init_text_item(CURRENT_NEXT_ITEM, AGENT_SCHEDULER_CURRENT_NEXT_ITEM_NAME, "Next task", "");
		indigo_init_text_item(CURRENT_NEXT_START_ITEM, AGENT_SCHEDULER_CURRENT_NEXT_START_ITEM_NAME, "Next start (UTC)", "");
		indigo_init_text_item(CURRENT_REASON_ITEM, AGENT_SCHEDULER_CURRENT_REASON_ITEM_NAME, "Waiting because", "");
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_SETTINGS
		SETTINGS_PROPERTY = indigo_init_number_property(NULL, device->name, AGENT_SCHEDULER_SETTINGS_PROPERTY_NAME, SCHEDULER_GROUP, "Scheduler settings", INDIGO_OK_STATE, INDIGO_RW_PERM, 8);
		if (SETTINGS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_number_item(SETTINGS_IDLE_CHECK_ITEM, AGENT_SCHEDULER_SETTINGS_IDLE_CHECK_INTERVAL_ITEM_NAME, "Check interval (s)", 1, 3600, 1, 30);
		indigo_init_number_item(SETTINGS_STOP_TIMEOUT_ITEM, AGENT_SCHEDULER_SETTINGS_STOP_TIMEOUT_ITEM_NAME, "Stop timeout (s)", 1, 3600, 1, 60);
		indigo_init_number_item(SETTINGS_START_TIMEOUT_ITEM, AGENT_SCHEDULER_SETTINGS_START_TIMEOUT_ITEM_NAME, "Start timeout (s)", 1, 600, 1, 10);
		indigo_init_number_item(SETTINGS_SETTING_WEIGHT_ITEM, AGENT_SCHEDULER_SETTINGS_SETTING_WEIGHT_ITEM_NAME, "Setting soonest weight", 0, 4, 0.1, 1);
		indigo_init_number_item(SETTINGS_ALTITUDE_WEIGHT_ITEM, AGENT_SCHEDULER_SETTINGS_ALTITUDE_WEIGHT_ITEM_NAME, "Altitude weight", 0, 4, 0.1, 1);
		indigo_init_number_item(SETTINGS_IDLE_PARK_AFTER_ITEM, AGENT_SCHEDULER_SETTINGS_IDLE_PARK_AFTER_ITEM_NAME, "Idle hook after (min)", 0, 1440, 1, 30);
		indigo_init_number_item(SETTINGS_FRAME_OVERHEAD_ITEM, AGENT_SCHEDULER_SETTINGS_FRAME_OVERHEAD_ITEM_NAME, "Estimate overhead per frame (s)", 0, 600, 1, 10);
		indigo_init_number_item(SETTINGS_OPERATION_OVERHEAD_ITEM, AGENT_SCHEDULER_SETTINGS_OPERATION_OVERHEAD_ITEM_NAME, "Estimate overhead per operation (s)", 0, 3600, 1, 120);
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_HOOKS
		HOOKS_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_HOOKS_PROPERTY_NAME, SCHEDULER_GROUP, "Hooks", INDIGO_OK_STATE, INDIGO_RW_PERM, 3);
		if (HOOKS_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item(HOOKS_STARTUP_ITEM, AGENT_SCHEDULER_HOOKS_STARTUP_ITEM_NAME, "Startup task", "");
		indigo_init_text_item(HOOKS_SHUTDOWN_ITEM, AGENT_SCHEDULER_HOOKS_SHUTDOWN_ITEM_NAME, "Shutdown task", "");
		indigo_init_text_item(HOOKS_IDLE_ITEM, AGENT_SCHEDULER_HOOKS_IDLE_ITEM_NAME, "Idle task", "");
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_HISTORY
		HISTORY_PROPERTY = indigo_init_light_property(NULL, device->name, AGENT_SCHEDULER_HISTORY_PROPERTY_NAME, SCHEDULER_GROUP, "Run history", INDIGO_OK_STATE, MAX_HISTORY);
		if (HISTORY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		HISTORY_PROPERTY->count = 0;
		CLEAR_HISTORY_PROPERTY = indigo_init_switch_property(NULL, device->name, AGENT_SCHEDULER_CLEAR_HISTORY_PROPERTY_NAME, SCHEDULER_GROUP, "Clear history", INDIGO_OK_STATE, INDIGO_RW_PERM, INDIGO_AT_MOST_ONE_RULE, 1);
		if (CLEAR_HISTORY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_switch_item(CLEAR_HISTORY_ITEM, AGENT_SCHEDULER_CLEAR_HISTORY_CLEAR_ITEM_NAME, "Clear", false);
		// used only by the configuration restore, so it is published empty
		RESTORE_HISTORY_PROPERTY = indigo_init_text_property(NULL, device->name, AGENT_SCHEDULER_RESTORE_HISTORY_PROPERTY_NAME, SCHEDULER_GROUP, "Restore history", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
		if (RESTORE_HISTORY_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		indigo_init_text_item_raw(RESTORE_HISTORY_ITEM, AGENT_SCHEDULER_RESTORE_HISTORY_ENTRIES_ITEM_NAME, "Entries", "");
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_PLAN
		PLAN_PROPERTY = indigo_init_light_property(NULL, device->name, AGENT_SCHEDULER_PLAN_PROPERTY_NAME, SCHEDULER_GROUP, "Night plan", INDIGO_OK_STATE, MAX_PLAN_ITEMS);
		if (PLAN_PROPERTY == NULL) {
			return INDIGO_FAILED;
		}
		PLAN_PROPERTY->count = 0;
		// -------------------------------------------------------------------------------- AGENT_SCHEDULER_TASK_<n>
		for (int i = 0; i < MAX_TASKS; i++) {
			char name[INDIGO_NAME_SIZE];
			snprintf(name, sizeof(name), AGENT_SCHEDULER_TASK_PROPERTY_NAME, i);
			indigo_property *property = indigo_init_text_property(NULL, device->name, name, TASKS_GROUP, "", INDIGO_OK_STATE, INDIGO_RW_PERM, 1);
			if (property == NULL) {
				return INDIGO_FAILED;
			}
			indigo_init_text_item_raw(property->items, AGENT_SCHEDULER_TASK_TASK_ITEM_NAME, "Task", "");
			PRIVATE_DATA->tasks[i].property = property;
		}
		// --------------------------------------------------------------------------------
		CONNECTION_PROPERTY->hidden = true;
		CONFIG_PROPERTY->hidden = true;
		PROFILE_PROPERTY->hidden = true;
		pthread_mutex_init(&PRIVATE_DATA->observed.mutex, NULL);
		pthread_mutex_init(&PRIVATE_DATA->property_mutex, NULL);
		PRIVATE_DATA->observed.sequence_state = INDIGO_IDLE_STATE;
		// a result left on the Scripting Agent by an earlier instance never matches a new token
		PRIVATE_DATA->token = (int)(time(NULL) % 100000) * 1000;
		PRIVATE_DATA->current = -1;
		PRIVATE_DATA->estimate_slot = -1;
		PRIVATE_DATA->next_task = -1;
		PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true;
		// the live properties are defined here, before any handler can change them
		indigo_define_property(device, ADD_TASK_PROPERTY, NULL);
		indigo_define_property(device, DELETE_TASK_PROPERTY, NULL);
		indigo_define_property(device, URGENT_TASK_PROPERTY, NULL);
		indigo_define_property(device, TASK_STATE_PROPERTY, NULL);
		indigo_define_property(device, TASK_NAME_PROPERTY, NULL);
		indigo_define_property(device, TASK_COMMAND_PROPERTY, NULL);
		indigo_define_property(device, CONTROL_PROPERTY, NULL);
		indigo_define_property(device, STATE_PROPERTY, NULL);
		indigo_define_property(device, CURRENT_PROPERTY, NULL);
		indigo_define_property(device, SETTINGS_PROPERTY, NULL);
		indigo_define_property(device, HOOKS_PROPERTY, NULL);
		indigo_define_property(device, HISTORY_PROPERTY, NULL);
		indigo_define_property(device, CLEAR_HISTORY_PROPERTY, NULL);
		indigo_define_property(device, RESTORE_HISTORY_PROPERTY, NULL);
		indigo_define_property(device, PLAN_PROPERTY, NULL);
		indigo_execute_handler_in(device, TICK_INTERVAL, tick_handler);
		INDIGO_DEVICE_ATTACH_LOG(DRIVER_NAME, device->name);
		return indigo_device_enumerate_properties(device, NULL, NULL);
	}
	return INDIGO_FAILED;
}

// clients get copies, the handler may change a task text or the task list at the same time
static void define_copy(indigo_device *device, indigo_client *client, indigo_property *property, indigo_property *selector) {
	if (!indigo_property_match(property, selector)) {
		return;
	}
	pthread_mutex_lock(&PRIVATE_DATA->property_mutex);
	indigo_property *copy = indigo_copy_property(NULL, property);
	pthread_mutex_unlock(&PRIVATE_DATA->property_mutex);
	if (client != NULL) {
		indigo_define_property_to_client(device, client, copy, NULL);
	} else {
		indigo_define_property(device, copy, NULL);
	}
	indigo_release_property(copy);
}

static indigo_result agent_enumerate_properties(indigo_device *device, indigo_client *client, indigo_property *property) {
	define_copy(device, client, ADD_TASK_PROPERTY, property);
	define_copy(device, client, DELETE_TASK_PROPERTY, property);
	define_copy(device, client, URGENT_TASK_PROPERTY, property);
	define_copy(device, client, TASK_STATE_PROPERTY, property);
	define_copy(device, client, TASK_NAME_PROPERTY, property);
	define_copy(device, client, TASK_COMMAND_PROPERTY, property);
	define_copy(device, client, CONTROL_PROPERTY, property);
	define_copy(device, client, STATE_PROPERTY, property);
	define_copy(device, client, CURRENT_PROPERTY, property);
	define_copy(device, client, SETTINGS_PROPERTY, property);
	define_copy(device, client, HOOKS_PROPERTY, property);
	define_copy(device, client, HISTORY_PROPERTY, property);
	define_copy(device, client, CLEAR_HISTORY_PROPERTY, property);
	define_copy(device, client, RESTORE_HISTORY_PROPERTY, property);
	define_copy(device, client, PLAN_PROPERTY, property);
	for (int i = 0; i < MAX_TASKS; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + i;
		if (task->used && task->stored) {
			define_copy(device, client, task->property, property);
		}
	}
	return indigo_device_enumerate_properties(device, client, property);
}

// returns the property of the agent the request changes, NULL for other properties
static indigo_property *own_property(indigo_device *device, indigo_property *property) {
	indigo_property *properties[] = { ADD_TASK_PROPERTY, DELETE_TASK_PROPERTY, URGENT_TASK_PROPERTY, TASK_NAME_PROPERTY, TASK_COMMAND_PROPERTY, CONTROL_PROPERTY, SETTINGS_PROPERTY, HOOKS_PROPERTY, CLEAR_HISTORY_PROPERTY, RESTORE_HISTORY_PROPERTY };
	for (int i = 0; i < (int)(sizeof(properties) / sizeof(properties[0])); i++) {
		if (indigo_property_match_changeable(properties[i], property)) {
			return properties[i];
		}
	}
	for (int i = 0; i < MAX_TASKS; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + i;
		if (task->used && task->stored && indigo_property_match_changeable(task->property, property)) {
			return task->property;
		}
	}
	return NULL;
}

static indigo_result agent_change_property(indigo_device *device, indigo_client *client, indigo_property *property) {
	assert(device != NULL);
	assert(DEVICE_CONTEXT != NULL);
	assert(property != NULL);
	indigo_property *target = own_property(device, property);
	if (target != NULL) {
		// the request is queued and acknowledged by the handler, which alone changes the properties,
		// so a caller holding another agent's lock never waits for the scheduler
		scheduler_command *command = indigo_safe_malloc(sizeof(scheduler_command));
		command->property = indigo_copy_property(NULL, property);
		command->target = target;
		command->config_reader = client != NULL && !strcmp(client->name, CONFIG_READER);
		indigo_execute_handler_with_data(device, command_handler, command);
		return INDIGO_OK;
	}
	return indigo_device_change_property(device, client, property);
}

static indigo_result agent_device_detach(indigo_device *device) {
	assert(device != NULL);
	PRIVATE_DATA->shutting_down = true;
	indigo_cancel_pending_handlers(device);
	// a change made less than SAVE_DELAY before the shutdown is still written
	if (PRIVATE_DATA->save_due > 0) {
		save_config(device);
	}
	indigo_release_property(ADD_TASK_PROPERTY);
	indigo_release_property(DELETE_TASK_PROPERTY);
	indigo_release_property(URGENT_TASK_PROPERTY);
	indigo_release_property(TASK_STATE_PROPERTY);
	indigo_release_property(TASK_NAME_PROPERTY);
	indigo_release_property(TASK_COMMAND_PROPERTY);
	indigo_release_property(CONTROL_PROPERTY);
	indigo_release_property(STATE_PROPERTY);
	indigo_release_property(CURRENT_PROPERTY);
	indigo_release_property(SETTINGS_PROPERTY);
	indigo_release_property(HOOKS_PROPERTY);
	indigo_release_property(HISTORY_PROPERTY);
	indigo_release_property(CLEAR_HISTORY_PROPERTY);
	indigo_release_property(RESTORE_HISTORY_PROPERTY);
	indigo_release_property(PLAN_PROPERTY);
	for (int i = 0; i < MAX_TASKS; i++) {
		scheduler_task *task = PRIVATE_DATA->tasks + i;
		if (task->used && task->stored) {
			indigo_delete_property(device, task->property, NULL);
		}
		free(task->text);
		free(task->pending_text);
		indigo_release_property(task->property);
	}
	pthread_mutex_destroy(&PRIVATE_DATA->observed.mutex);
	pthread_mutex_destroy(&PRIVATE_DATA->property_mutex);
	INDIGO_DEVICE_DETACH_LOG(DRIVER_NAME, device->name);
	return indigo_device_detach(device);
}

// -------------------------------------------------------------------------------- INDIGO agent client implementation

static unsigned agent_busy_bit(const char *device_name) {
	if (!strcmp(device_name, IMAGER_AGENT)) {
		return AGENT_IMAGER_BUSY;
	}
	if (!strcmp(device_name, MOUNT_AGENT)) {
		return AGENT_MOUNT_BUSY;
	}
	if (!strcmp(device_name, GUIDER_AGENT)) {
		return AGENT_GUIDER_BUSY;
	}
	if (!strcmp(device_name, "Astrometry Agent") || !strcmp(device_name, "Platesolver Agent") || !strcmp(device_name, "ASTAP Agent")) {
		return AGENT_SOLVER_BUSY;
	}
	return 0;
}

static void observe_property(indigo_property *property, const char *message) {
	if (agent_device == NULL || !strcmp(property->device, agent_device->name)) {
		return;
	}
	indigo_device *device = agent_device;
	scheduler_observed *observed = &PRIVATE_DATA->observed;
	pthread_mutex_lock(&observed->mutex);
	if (!strcmp(property->device, SCRIPTING_AGENT)) {
		if (!strcmp(property->name, AGENT_SCRIPTING_RUN_SCRIPT_PROPERTY_NAME)) {
			observed->scripting_present = true;
		} else if (!strcmp(property->name, "SEQUENCE_STATE")) {
			observed->sequencer_present = true;
			if (property->state == INDIGO_BUSY_STATE && observed->sequence_state != INDIGO_BUSY_STATE) {
				observed->busy_events++;
			} else if (property->state != INDIGO_BUSY_STATE && observed->sequence_state == INDIGO_BUSY_STATE) {
				observed->finish_events++;
				observed->finish_state = property->state;
				INDIGO_COPY_VALUE(observed->finish_message, message ? message : "");
			}
			observed->sequence_state = property->state;
		} else if (!strcmp(property->name, "SEQUENCE_NAME") && property->count > 0) {
			INDIGO_COPY_VALUE(observed->sequence_name, indigo_get_text_item_value(property->items));
		} else if (!strcmp(property->name, SCHEDULER_RESULT_PROPERTY)) {
			indigo_item *token = indigo_get_item(property, "TOKEN");
			indigo_item *result = indigo_get_item(property, "RESULT");
			indigo_item *result_message = indigo_get_item(property, "MESSAGE");
			if (token && result) {
				int value = atoi(indigo_get_text_item_value(token));
				bool ok = !strcmp(indigo_get_text_item_value(result), "ok");
				const char *text = result_message ? indigo_get_text_item_value(result_message) : "";
				if (value == observed->run_token) {
					observed->run_received = true;
					observed->run_ok = ok;
					INDIGO_COPY_VALUE(observed->run_message, text);
				} else if (value == observed->estimate_token) {
					observed->estimate_received = true;
					observed->estimate_ok = ok;
					INDIGO_COPY_VALUE(observed->estimate_message, text);
				}
			}
		} else if (!strcmp(property->name, SCHEDULER_ESTIMATE_PROPERTY)) {
			static const char *names[] = { "SUPPORTED", "EXPOSURE", "FRAMES", "OPERATIONS", "WAITS", "UNKNOWN" };
			indigo_item *token = indigo_get_item(property, "TOKEN");
			if (token && (int)token->number.value == observed->estimate_token) {
				for (int i = 0; i < 6; i++) {
					indigo_item *item = indigo_get_item(property, names[i]);
					observed->estimate_values[i] = item ? item->number.value : 0;
				}
				observed->estimate_counts = true;
			}
		}
	} else if (!strcmp(property->device, MOUNT_AGENT) && !strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
		indigo_item *latitude = indigo_get_item(property, GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME);
		indigo_item *longitude = indigo_get_item(property, GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME);
		indigo_item *elevation = indigo_get_item(property, GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME);
		if (latitude && longitude) {
			observed->latitude = latitude->number.value;
			observed->longitude = longitude->number.value;
			observed->elevation = elevation ? elevation->number.value : 0;
			observed->site_known = true;
		}
	}
	if (!strcmp(property->name, AGENT_START_PROCESS_PROPERTY_NAME)) {
		unsigned bit = agent_busy_bit(property->device);
		if (property->state == INDIGO_BUSY_STATE) {
			observed->agents_busy |= bit;
		} else {
			observed->agents_busy &= ~bit;
		}
	}
	pthread_mutex_unlock(&observed->mutex);
}

static indigo_result agent_client_attach(indigo_client *client) {
	assert(client != NULL);
	indigo_property all_properties;
	memset(&all_properties, 0, sizeof(all_properties));
	indigo_enumerate_properties(client, &all_properties);
	return INDIGO_OK;
}

static indigo_result agent_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	observe_property(property, message);
	return INDIGO_OK;
}

static indigo_result agent_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	observe_property(property, message);
	return INDIGO_OK;
}

static indigo_result agent_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	if (agent_device == NULL || !strcmp(property->device, agent_device->name)) {
		return INDIGO_OK;
	}
	indigo_device *scheduler = agent_device;
	scheduler_observed *observed = &((scheduler_private_data *)scheduler->private_data)->observed;
	pthread_mutex_lock(&observed->mutex);
	if (!strcmp(property->device, SCRIPTING_AGENT)) {
		if (*property->name == 0 || !strcmp(property->name, AGENT_SCRIPTING_RUN_SCRIPT_PROPERTY_NAME)) {
			observed->scripting_present = false;
			observed->sequencer_present = false;
		} else if (!strcmp(property->name, "SEQUENCE_STATE")) {
			observed->sequencer_present = false;
		}
	} else if (!strcmp(property->device, MOUNT_AGENT) && (*property->name == 0 || !strcmp(property->name, GEOGRAPHIC_COORDINATES_PROPERTY_NAME))) {
		observed->site_known = false;
	}
	if (*property->name == 0 || !strcmp(property->name, AGENT_START_PROCESS_PROPERTY_NAME)) {
		observed->agents_busy &= ~agent_busy_bit(property->device);
	}
	pthread_mutex_unlock(&observed->mutex);
	return INDIGO_OK;
}

static indigo_result agent_client_detach(indigo_client *client) {
	assert(client != NULL);
	return INDIGO_OK;
}

// -------------------------------------------------------------------------------- Initialization

static scheduler_private_data *private_data = NULL;

indigo_result indigo_agent_scheduler(indigo_driver_action action, indigo_driver_info *info) {
	static indigo_device agent_device_template = INDIGO_DEVICE_INITIALIZER(SCHEDULER_AGENT_NAME, agent_device_attach, agent_enumerate_properties, agent_change_property, NULL, agent_device_detach);
	static indigo_client agent_client_template = { SCHEDULER_AGENT_NAME, false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL, agent_client_attach, agent_define_property, agent_update_property, agent_delete_property, NULL, agent_client_detach };
	static indigo_driver_action last_action = INDIGO_DRIVER_SHUTDOWN;
	SET_DRIVER_INFO(info, SCHEDULER_AGENT_NAME, __FUNCTION__, DRIVER_VERSION, false, last_action);
	if (action == last_action) {
		return INDIGO_OK;
	}
	switch (action) {
		case INDIGO_DRIVER_INIT:
			last_action = action;
			private_data = indigo_safe_malloc(sizeof(scheduler_private_data));
			agent_device = indigo_safe_malloc_copy(sizeof(indigo_device), &agent_device_template);
			agent_device->private_data = private_data;
			indigo_attach_device(agent_device);
			agent_client = indigo_safe_malloc_copy(sizeof(indigo_client), &agent_client_template);
			agent_client->client_context = agent_device->device_context;
			indigo_attach_client(agent_client);
			break;
		case INDIGO_DRIVER_SHUTDOWN:
			last_action = action;
			// the handlers use the client, so they stop before it is detached
			if (agent_device != NULL) {
				private_data->shutting_down = true;
				indigo_cancel_pending_handlers(agent_device);
			}
			if (agent_client != NULL) {
				indigo_detach_client(agent_client);
				free(agent_client);
				agent_client = NULL;
			}
			if (agent_device != NULL) {
				indigo_device *device = agent_device;
				agent_device = NULL;
				indigo_detach_device(device);
				free(device);
			}
			if (private_data != NULL) {
				free(private_data);
				private_data = NULL;
			}
			break;
		case INDIGO_DRIVER_INFO:
			break;
	}
	return INDIGO_OK;
}
