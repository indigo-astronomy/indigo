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
 \file indigo_agent_scheduler_header.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <indigo/indigo_uni_io.h>

#include "indigo_agent_scheduler_header.h"

#define LINE_SIZE 256

#define KEY_TASK					0x0001
#define KEY_PRIORITY			0x0002
#define KEY_TARGET				0x0004
#define KEY_ALTITUDE			0x0008
#define KEY_AIRMASS				0x0010
#define KEY_HA						0x0020
#define KEY_MOON					0x0040
#define KEY_ILLUMINATION	0x0080
#define KEY_TWILIGHT			0x0100
#define KEY_WINDOW				0x0200
#define KEY_DURATION			0x0400
#define KEY_REPEAT				0x0800
#define KEY_RETRY					0x1000
#define KEY_PREEMPTED			0x2000
#define KEY_ENABLED				0x4000

typedef struct {
	const char *name;
	int mask;
} key_definition;

static const key_definition keys[] = {
	{ "task", KEY_TASK },
	{ "priority", KEY_PRIORITY },
	{ "target", KEY_TARGET },
	{ "altitude", KEY_ALTITUDE },
	{ "airmass", KEY_AIRMASS },
	{ "ha", KEY_HA },
	{ "moon", KEY_MOON },
	{ "illumination", KEY_ILLUMINATION },
	{ "twilight", KEY_TWILIGHT },
	{ "window", KEY_WINDOW },
	{ "duration", KEY_DURATION },
	{ "repeat", KEY_REPEAT },
	{ "retry", KEY_RETRY },
	{ "preempted", KEY_PREEMPTED },
	{ "enabled", KEY_ENABLED },
	{ NULL, 0 }
};

static const char *skip_blanks(const char *s) {
	while (*s == ' ' || *s == '\t') {
		s++;
	}
	return s;
}

// splits value into whitespace separated tokens, returns their count
static int tokenize(char *value, char **tokens, int max) {
	int count = 0;
	char *s = value;
	while (*s) {
		while (*s == ' ' || *s == '\t') {
			*s++ = 0;
		}
		if (*s == 0) {
			break;
		}
		if (count == max) {
			return max + 1;
		}
		tokens[count++] = s;
		while (*s && *s != ' ' && *s != '\t') {
			s++;
		}
	}
	return count;
}

// plain decimal numbers only, strtod() alone would also accept hexadecimal, exponents and infinity
static bool is_decimal(const char *token) {
	return *token && strspn(token, "0123456789+-.") == strlen(token);
}

static bool parse_number(const char *token, double min, double max, double *result) {
	if (!is_decimal(token)) {
		return false;
	}
	char *end;
	double value = strtod(token, &end);
	if (end == token || *end != 0 || !isfinite(value) || value < min || value > max) {
		return false;
	}
	*result = value;
	return true;
}

static bool parse_integer(const char *token, int min, int max, int *result) {
	char *end;
	long value = strtol(token, &end, 10);
	if (end == token || *end != 0 || value < min || value > max) {
		return false;
	}
	*result = (int)value;
	return true;
}

// decimal or sexagesimal value "d", "d:m" or "d:m:s" with an optional sign
static bool parse_sexagesimal(const char *token, double min, double max, double *result) {
	if (strchr(token, ':') == NULL) {
		return parse_number(token, min, max, result);
	}
	const char *s = token;
	double sign = 1;
	if (*s == '-' || *s == '+') {
		sign = *s == '-' ? -1 : 1;
		s++;
	}
	double parts[3] = { 0, 0, 0 };
	int count = 0;
	while (*s) {
		if (count == 3) {
			return false;
		}
		char *end;
		if (!isdigit((unsigned char)*s) || strspn(s, "0123456789.:") != strlen(s)) {
			return false;
		}
		parts[count] = strtod(s, &end);
		if (end == s || (*end != ':' && *end != 0)) {
			return false;
		}
		if (count > 0 && parts[count] >= 60) {
			return false;
		}
		// only the last part may have a fraction
		if (*end == ':' && parts[count] != floor(parts[count])) {
			return false;
		}
		count++;
		s = *end == ':' ? end + 1 : end;
		if (*end == ':' && *s == 0) {
			return false;
		}
	}
	double value = sign * (parts[0] + parts[1] / 60.0 + parts[2] / 3600.0);
	if (value < min || value > max) {
		return false;
	}
	*result = value;
	return true;
}

// "> value" or ">value", returns pointer past the operator or NULL
static char *parse_operator(char *value, bool minimum) {
	char *s = (char *)skip_blanks(value);
	if (*s != (minimum ? '>' : '<')) {
		return NULL;
	}
	s++;
	if (*s == '=') {
		s++;
	}
	return s;
}

static bool parse_clock(const char *token, int *minute) {
	int hours, minutes;
	char extra;
	if (strlen(token) > 5 || sscanf(token, "%d:%d%c", &hours, &minutes, &extra) != 2 || hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
		return false;
	}
	*minute = hours * 60 + minutes;
	return true;
}

static bool parse_date(const char *date, const char *clock, time_t *result) {
	int year, month, day, minute;
	char extra;
	if (sscanf(date, "%d-%d-%d%c", &year, &month, &day, &extra) != 3 || year < 1970 || year > 2999 || month < 1 || month > 12 || day < 1 || day > 31) {
		return false;
	}
	if (!parse_clock(clock, &minute)) {
		return false;
	}
	struct tm tm_time;
	memset(&tm_time, 0, sizeof(tm_time));
	tm_time.tm_year = year - 1900;
	tm_time.tm_mon = month - 1;
	tm_time.tm_mday = day;
	tm_time.tm_hour = minute / 60;
	tm_time.tm_min = minute % 60;
	time_t value = indigo_timegm(&tm_time);
	struct tm check;
	indigo_gmtime(&value, &check);
	if (check.tm_mday != day || check.tm_mon != month - 1) {
		return false;
	}
	*result = value;
	return true;
}

// one window endpoint: empty, "HH:MM" or "YYYY-MM-DD HH:MM"
static bool parse_window_endpoint(char *text, scheduler_window_kind *kind, bool *present, time_t *absolute, int *minute) {
	char *tokens[3];
	int count = tokenize(text, tokens, 2);
	*present = count > 0;
	if (count == 0) {
		*kind = SCHEDULER_WINDOW_NONE;
		return true;
	}
	if (count == 1) {
		*kind = SCHEDULER_WINDOW_NIGHTLY;
		return parse_clock(tokens[0], minute);
	}
	if (count == 2) {
		*kind = SCHEDULER_WINDOW_ABSOLUTE;
		return parse_date(tokens[0], tokens[1], absolute);
	}
	return false;
}

static bool parse_window(char *value, scheduler_header *header) {
	char *separator = strstr(value, "..");
	if (separator == NULL || strstr(separator + 2, "..") != NULL) {
		return false;
	}
	*separator = 0;
	scheduler_window_kind start_kind, end_kind;
	if (!parse_window_endpoint(value, &start_kind, &header->has_window_start, &header->window_start, &header->window_start_minute)) {
		return false;
	}
	if (!parse_window_endpoint(separator + 2, &end_kind, &header->has_window_end, &header->window_end, &header->window_end_minute)) {
		return false;
	}
	if (!header->has_window_start && !header->has_window_end) {
		return false;
	}
	if (header->has_window_start && header->has_window_end && start_kind != end_kind) {
		return false;
	}
	header->window_kind = header->has_window_start ? start_kind : end_kind;
	if (header->window_kind == SCHEDULER_WINDOW_ABSOLUTE && header->has_window_start && header->has_window_end && header->window_end <= header->window_start) {
		return false;
	}
	return true;
}

static bool parse_keyword(const char *token, const char **words, int count, int *result) {
	for (int i = 0; i < count; i++) {
		if (!strcasecmp(token, words[i])) {
			*result = i;
			return true;
		}
	}
	return false;
}

static bool parse_value(int key, char *value, scheduler_header *header, const char **expected) {
	char *tokens[5];
	int count;
	char *s;
	switch (key) {
		case KEY_TASK: {
			*expected = "a name of 1 to 40 characters";
			size_t length = strlen(value);
			if (length == 0 || length >= SCHEDULER_TASK_NAME_SIZE) {
				return false;
			}
			for (const char *c = value; *c; c++) {
				if ((unsigned char)*c < 0x20 || *c == 0x7f) {
					return false;
				}
			}
			strcpy(header->name, value);
			return true;
		}
		case KEY_PRIORITY: {
			static const char *classes[] = { "low", "normal", "high", "urgent" };
			int priority_class;
			*expected = "'urgent', 'high', 'normal' or 'low' optionally followed by a level 0-100";
			count = tokenize(value, tokens, 2);
			if (count < 1 || count > 2 || !parse_keyword(tokens[0], classes, 4, &priority_class)) {
				return false;
			}
			header->priority_class = priority_class;
			if (count == 2) {
				return parse_integer(tokens[1], 0, 100, &header->level);
			}
			return true;
		}
		case KEY_TARGET:
			*expected = "'ra dec' with RA in hours 0-24 and Dec in degrees -90-90";
			count = tokenize(value, tokens, 2);
			if (count != 2 || !parse_sexagesimal(tokens[0], 0, 24, &header->ra) || !parse_sexagesimal(tokens[1], -90, 90, &header->dec)) {
				return false;
			}
			if (header->ra == 24) {
				header->ra = 0;
			}
			header->has_target = true;
			return true;
		case KEY_ALTITUDE:
			*expected = "'> degrees' with degrees 0-90";
			if ((s = parse_operator(value, true)) == NULL || tokenize(s, tokens, 1) != 1 || !parse_number(tokens[0], 0, 90, &header->min_altitude)) {
				return false;
			}
			header->has_min_altitude = true;
			return true;
		case KEY_AIRMASS:
			*expected = "'< value' with value at least 1";
			if ((s = parse_operator(value, false)) == NULL || tokenize(s, tokens, 1) != 1 || !parse_number(tokens[0], 1, 100, &header->max_airmass)) {
				return false;
			}
			header->has_max_airmass = true;
			return true;
		case KEY_HA: {
			*expected = "'from .. to' with hours -12-12";
			char *separator = strstr(value, "..");
			if (separator == NULL) {
				return false;
			}
			*separator = 0;
			if (tokenize(value, tokens, 1) != 1 || !parse_number(tokens[0], -12, 12, &header->ha_from)) {
				return false;
			}
			if (tokenize(separator + 2, tokens, 1) != 1 || !parse_number(tokens[0], -12, 12, &header->ha_to)) {
				return false;
			}
			header->has_ha = true;
			return true;
		}
		case KEY_MOON:
			*expected = "'> degrees' with degrees 0-180, optionally followed by 'width days' with days 0.5-30";
			if ((s = parse_operator(value, true)) == NULL) {
				return false;
			}
			count = tokenize(s, tokens, 3);
			if ((count != 1 && count != 3) || !parse_number(tokens[0], 0, 180, &header->moon_separation)) {
				return false;
			}
			header->moon_width = 0;
			if (count == 3 && (strcasecmp(tokens[1], "width") || !parse_number(tokens[2], 0.5, 30, &header->moon_width))) {
				return false;
			}
			header->has_moon = true;
			return true;
		case KEY_ILLUMINATION:
			*expected = "'< percent' with percent 0-100";
			if ((s = parse_operator(value, false)) == NULL || tokenize(s, tokens, 1) != 1 || !parse_number(tokens[0], 0, 100, &header->max_illumination)) {
				return false;
			}
			header->has_illumination = true;
			return true;
		case KEY_TWILIGHT: {
			static const char *levels[] = { "none", "civil", "nautical", "astronomical" };
			int twilight;
			*expected = "'none', 'civil', 'nautical' or 'astronomical'";
			if (tokenize(value, tokens, 1) != 1 || !parse_keyword(tokens[0], levels, 4, &twilight)) {
				return false;
			}
			header->twilight = twilight;
			return true;
		}
		case KEY_WINDOW:
			*expected = "'[start] .. [end]' with 'HH:MM' or 'YYYY-MM-DD HH:MM' UTC times of the same form";
			return parse_window(value, header);
		case KEY_DURATION:
			*expected = "minutes 1-1440";
			if (tokenize(value, tokens, 1) != 1 || !parse_integer(tokens[0], 1, 1440, &header->duration)) {
				return false;
			}
			header->has_duration = true;
			return true;
		case KEY_REPEAT:
			*expected = "'count' with count 1-10000, optionally followed by 'every minutes'";
			count = tokenize(value, tokens, 3);
			if ((count != 1 && count != 3) || !parse_integer(tokens[0], 1, 10000, &header->repeat)) {
				return false;
			}
			return count == 1 || (!strcasecmp(tokens[1], "every") && parse_integer(tokens[2], 0, 100000, &header->repeat_every));
		case KEY_RETRY:
			*expected = "'count' with count 0-100, optionally followed by 'after minutes'";
			count = tokenize(value, tokens, 3);
			if ((count != 1 && count != 3) || !parse_integer(tokens[0], 0, 100, &header->retry)) {
				return false;
			}
			return count == 1 || (!strcasecmp(tokens[1], "after") && parse_integer(tokens[2], 0, 100000, &header->retry_after));
		case KEY_PREEMPTED: {
			static const char *actions[] = { "retry", "fail" };
			int action;
			*expected = "'retry' or 'fail'";
			if (tokenize(value, tokens, 1) != 1 || !parse_keyword(tokens[0], actions, 2, &action)) {
				return false;
			}
			header->preempted_fail = action == 1;
			return true;
		}
		case KEY_ENABLED: {
			static const char *answers[] = { "no", "yes" };
			int answer;
			*expected = "'yes' or 'no'";
			if (tokenize(value, tokens, 1) != 1 || !parse_keyword(tokens[0], answers, 2, &answer)) {
				return false;
			}
			header->enabled = answer == 1;
			return true;
		}
	}
	return false;
}

static void set_defaults(scheduler_header *header) {
	memset(header, 0, sizeof(*header));
	header->priority_class = SCHEDULER_PRIORITY_NORMAL;
	header->level = 50;
	header->repeat = 1;
	header->enabled = true;
}

bool indigo_scheduler_parse_header(const char *text, scheduler_header *header, size_t *body_offset, char *error, size_t error_size) {
	set_defaults(header);
	if (text == NULL) {
		snprintf(error, error_size, "empty task");
		return false;
	}
	if (strlen(text) > SCHEDULER_TASK_TEXT_LIMIT) {
		snprintf(error, error_size, "task text is longer than %d bytes", SCHEDULER_TASK_TEXT_LIMIT);
		return false;
	}
	const char *s = text;
	if (!strncmp(s, "\xEF\xBB\xBF", 3)) {
		s += 3;
	}
	int found = 0;
	int line_number = 0;
	while (*s) {
		const char *end = strchr(s, '\n');
		size_t length = end ? (size_t)(end - s) : strlen(s);
		line_number++;
		char line[LINE_SIZE];
		bool too_long = length >= LINE_SIZE;
		size_t copy = too_long ? LINE_SIZE - 1 : length;
		memcpy(line, s, copy);
		line[copy] = 0;
		while (copy > 0 && (line[copy - 1] == '\r' || line[copy - 1] == ' ' || line[copy - 1] == '\t')) {
			line[--copy] = 0;
		}
		const char *c = skip_blanks(line);
		if (c[0] != '/' || c[1] != '/') {
			break;
		}
		c = skip_blanks(c + 2);
		if (*c == 0) {
			s = end ? end + 1 : s + length;
			continue;
		}
		if (*c != '@') {
			break;
		}
		if (too_long) {
			snprintf(error, error_size, "line %d: header line is longer than %d characters", line_number, LINE_SIZE - 1);
			return false;
		}
		c++;
		char key[32];
		size_t key_length = 0;
		while (isalpha((unsigned char)c[key_length]) || c[key_length] == '_') {
			key_length++;
		}
		if (key_length == 0 || key_length >= sizeof(key)) {
			snprintf(error, error_size, "line %d: invalid key", line_number);
			return false;
		}
		memcpy(key, c, key_length);
		key[key_length] = 0;
		c += key_length;
		if (*c != 0 && *c != ' ' && *c != '\t') {
			snprintf(error, error_size, "line %d: invalid key", line_number);
			return false;
		}
		int mask = 0;
		for (int i = 0; keys[i].name; i++) {
			if (!strcasecmp(key, keys[i].name)) {
				mask = keys[i].mask;
				break;
			}
		}
		if (mask == 0) {
			snprintf(error, error_size, "line %d: @%s: unknown key", line_number, key);
			return false;
		}
		if (found & mask) {
			snprintf(error, error_size, "line %d: @%s: repeated key", line_number, key);
			return false;
		}
		found |= mask;
		char value[LINE_SIZE];
		strcpy(value, skip_blanks(c));
		const char *expected = "a valid value";
		if (*value == 0 || !parse_value(mask, value, header, &expected)) {
			snprintf(error, error_size, "line %d: @%s: expected %s", line_number, key, expected);
			return false;
		}
		s = end ? end + 1 : s + length;
	}
	if (!(found & KEY_TASK)) {
		snprintf(error, error_size, "missing @task");
		return false;
	}
	if (!header->has_target && (found & (KEY_ALTITUDE | KEY_AIRMASS | KEY_HA | KEY_MOON | KEY_ILLUMINATION))) {
		snprintf(error, error_size, "@altitude, @airmass, @ha, @moon and @illumination require @target");
		return false;
	}
	const char *body = s;
	bool empty = true;
	for (const char *b = body; *b; b++) {
		if (!isspace((unsigned char)*b)) {
			empty = false;
			break;
		}
	}
	if (empty) {
		snprintf(error, error_size, "the task has no body");
		return false;
	}
	if (body_offset) {
		*body_offset = (size_t)(body - text);
	}
	return true;
}

int indigo_scheduler_header_duration(const scheduler_header *header) {
	return header->has_duration ? header->duration : SCHEDULER_DEFAULT_DURATION;
}

const char *indigo_scheduler_priority_name(scheduler_priority_class priority_class) {
	switch (priority_class) {
		case SCHEDULER_PRIORITY_LOW:
			return "low";
		case SCHEDULER_PRIORITY_NORMAL:
			return "normal";
		case SCHEDULER_PRIORITY_HIGH:
			return "high";
		case SCHEDULER_PRIORITY_URGENT:
			return "urgent";
	}
	return "normal";
}
