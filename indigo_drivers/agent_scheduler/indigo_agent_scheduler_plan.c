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

/** INDIGO Scheduler agent task score and night plan
 \file indigo_agent_scheduler_plan.c
 */

#include <stdlib.h>
#include <string.h>

#include <indigo/indigo_bus.h>
#include <indigo/indigo_timer.h>

#include "indigo_agent_scheduler_plan.h"

#define MAX_PLAN_TASKS	256

// hours of hour angle per hour of time
#define SIDEREAL_RATE	1.00273790935

// summed qualities closer than this are equal, a start must be better by more than this to be the best one
#define QUALITY_EPSILON	1e-6

// the inputs of a simulation
typedef struct {
	const scheduler_sky_grid *sky;
	const scheduler_plan_task *tasks;
	int count;
	const scheduler_plan_hook *hooks;
	int hook_count;
	time_t now, end;
	double setting_weight, altitude_weight;
	// the meridian flip of a VISIBILITY plan, NULL otherwise
	const scheduler_flip *flip;
	unsigned long requeue_base;
	// per task, a stretch [from, until) of start times at which the task is known not to be eligible; it depends only on the constraints
	time_t *ineligible_from, *ineligible_until;
} plan_context;

// the counters of a simulation
typedef struct {
	time_t t;
	int next_hook;
	int runs_left[MAX_PLAN_TASKS];
	time_t available_from[MAX_PLAN_TASKS];
	unsigned long urgent_order[MAX_PLAN_TASKS];
} plan_state;

double indigo_scheduler_score(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_task_points *points, const scheduler_header *header, time_t t, double setting_weight, double altitude_weight) {
	static const double class_base[] = { 0, 1000, 2000, 3000, 0 };
	double result = class_base[header->priority_class] + header->level;
	int to_set = indigo_scheduler_points_time_to_set(sky, points, header, t);
	result += setting_weight * 100 * (1 - (double)to_set / SCHEDULER_SETTING_HORIZON);
	if (header->has_target && target != NULL && target->valid && indigo_scheduler_sky_covers(sky, t, t)) {
		int index = (int)((t - sky->start) / SCHEDULER_GRID_STEP);
		double altitude = target->altitude[index];
		if (altitude > 0 && target->transit_altitude > 0) {
			double relative = 100 * altitude / target->transit_altitude;
			result += altitude_weight * (relative < 100 ? relative : 100);
		}
	}
	return result;
}

// the altitude at a grid point in percent of the transit altitude, clamped to 0..100
static double relative_altitude(const scheduler_target_grid *target, int index) {
	double altitude = target->altitude[index];
	if (altitude <= 0) {
		return 0;
	}
	double relative = 100 * altitude / target->transit_altitude;
	return relative < 100 ? relative : 100;
}

static bool quality_known(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start) {
	return header->has_target && target != NULL && target->valid && target->transit_altitude > 0 && sky->valid && start >= sky->start;
}

double indigo_scheduler_run_quality(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, int duration) {
	if (!quality_known(sky, target, header, start)) {
		return -1;
	}
	// the grid points start, start + step, ... up to start + duration
	int first = (int)((start - sky->start) / SCHEDULER_GRID_STEP);
	int last = first + duration / SCHEDULER_GRID_STEP;
	if (last >= SCHEDULER_GRID_POINTS) {
		return -1;
	}
	double sum = 0;
	for (int k = first; k <= last; k++) {
		sum += relative_altitude(target, k);
	}
	return sum / (last - first + 1);
}

// the hour angle of the target at start, normalised to [-12, 12), from the grid point before it; false when the run cannot flip: a disabled or NULL flip, a task without a usable target or a start outside the grid
static bool flip_hour_angle(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, const scheduler_flip *flip, double *ha) {
	if (flip == NULL || !flip->enabled || !header->has_target || target == NULL || !target->valid || !sky->valid || start < sky->start) {
		return false;
	}
	int k = (int)((start - sky->start) / SCHEDULER_GRID_STEP);
	if (k >= SCHEDULER_GRID_POINTS) {
		return false;
	}
	double value = target->hour_angle[k] + (double)(start - sky->start - (time_t)k * SCHEDULER_GRID_STEP) / 3600.0 * SIDEREAL_RATE;
	if (value >= 12) {
		value -= 24;
	} else if (value < -12) {
		value += 24;
	}
	*ha = value;
	return true;
}

// a run started west of the meridian is slewed to the west side and does not flip; one started east of it flips when the hour angle reaches the offset before the run ends
static bool flip_needed(double ha, int duration, const scheduler_flip *flip) {
	return ha < 0 && ha + duration / 3600.0 * SIDEREAL_RATE >= flip->offset;
}

bool indigo_scheduler_run_flips(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, int duration, const scheduler_flip *flip) {
	double ha;
	return flip_hour_angle(sky, target, header, start, flip, &ha) && flip_needed(ha, duration, flip);
}

double indigo_scheduler_flip_loss(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, int duration, const scheduler_flip *flip) {
	double ha;
	if (flip == NULL || flip->penalty <= 0 || !flip_hour_angle(sky, target, header, start, flip, &ha) || !flip_needed(ha, duration, flip)) {
		return 0;
	}
	// with a negative offset the capture also waits from the pause (at the offset, or at the start when it is later) until the meridian
	double wait = 0;
	if (flip->offset < 0) {
		wait = -(ha > flip->offset ? ha : flip->offset);
	}
	return flip->penalty + wait;
}

static void init_context(plan_context *ctx, const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight) {
	ctx->sky = sky;
	ctx->tasks = tasks;
	ctx->count = count > MAX_PLAN_TASKS ? MAX_PLAN_TASKS : count;
	ctx->hooks = hooks;
	ctx->hook_count = hook_count;
	ctx->now = now;
	ctx->end = end;
	ctx->setting_weight = setting_weight;
	ctx->altitude_weight = altitude_weight;
	ctx->flip = NULL;
	for (int i = 0; i < ctx->count; i++) {
		ctx->ineligible_from[i] = ctx->ineligible_until[i] = 0;
	}
	unsigned long last_order = 0;
	for (int i = 0; i < ctx->count; i++) {
		if (tasks[i].urgent_order > last_order) {
			last_order = tasks[i].urgent_order;
		}
	}
	// a task queued again after a delay gets the next urgent order at that time, later delays get later orders
	ctx->requeue_base = last_order + 1;
}

// the state at now: the running hooks and the running task come first and the simulation continues from their end; stores their runs when runs is not NULL
static int init_state(const plan_context *ctx, plan_state *state, scheduler_plan_run *runs, int max_runs) {
	const scheduler_plan_task *tasks = ctx->tasks;
	const scheduler_plan_hook *hooks = ctx->hooks;
	time_t now = ctx->now;
	for (int i = 0; i < ctx->count; i++) {
		state->runs_left[i] = tasks[i].selectable ? tasks[i].runs_left : 0;
		state->available_from[i] = tasks[i].available_from;
		state->urgent_order[i] = state->available_from[i] > now ? ctx->requeue_base + (unsigned long)(state->available_from[i] - now) : tasks[i].urgent_order;
	}
	int stored = 0;
	state->t = now;
	state->next_hook = 0;
	while (state->next_hook < ctx->hook_count && hooks[state->next_hook].running && (runs == NULL || stored < max_runs)) {
		const scheduler_plan_hook *hook = hooks + state->next_hook;
		time_t end = hook->time + hook->duration;
		if (end < now + 60) {
			end = now + 60;
		}
		if (runs != NULL) {
			runs[stored].task = -1;
			runs[stored].hook = state->next_hook;
			runs[stored].run = 1;
			runs[stored].start = hook->time;
			runs[stored].end = end;
			runs[stored].running = true;
			stored++;
		}
		if (end > state->t) {
			state->t = end;
		}
		state->next_hook++;
	}
	for (int i = 0; i < ctx->count && (runs == NULL || stored < max_runs); i++) {
		if (tasks[i].running) {
			const scheduler_header *header = tasks[i].header;
			if (runs != NULL) {
				runs[stored].task = i;
				runs[stored].hook = -1;
				runs[stored].run = header->repeat - tasks[i].runs_left + 1;
				runs[stored].start = tasks[i].running_start;
				runs[stored].end = tasks[i].running_end;
				runs[stored].running = true;
				stored++;
			}
			if (tasks[i].running_end > state->t) {
				state->t = tasks[i].running_end;
			}
			if (state->runs_left[i] > 0) {
				state->runs_left[i]--;
				state->available_from[i] = tasks[i].running_end + header->repeat_every * 60;
				state->urgent_order[i] = ctx->requeue_base + (unsigned long)(state->available_from[i] - now);
			}
		}
	}
	return stored;
}

// simulates the greedy schedule from the state until the end and stores the runs
static int simulate(const plan_context *ctx, plan_state *state, scheduler_plan_run *runs, int stored, int max_runs) {
	const scheduler_sky_grid *sky = ctx->sky;
	const scheduler_plan_task *tasks = ctx->tasks;
	const scheduler_plan_hook *hooks = ctx->hooks;
	int count = ctx->count;
	while (state->t < ctx->end && stored < max_runs) {
		time_t t = state->t;
		// a hook never interrupts a run, it runs when the run ends
		if (state->next_hook < ctx->hook_count && hooks[state->next_hook].time <= t) {
			runs[stored].task = -1;
			runs[stored].hook = state->next_hook;
			runs[stored].run = 1;
			runs[stored].start = t;
			runs[stored].end = t + hooks[state->next_hook].duration;
			runs[stored].running = false;
			stored++;
			state->t += hooks[state->next_hook].duration;
			state->next_hook++;
			continue;
		}
		int best = -1;
		double best_score = 0;
		for (int i = 0; i < count; i++) {
			if (state->runs_left[i] <= 0 || state->available_from[i] > t) {
				continue;
			}
			if (t >= ctx->ineligible_from[i] && t < ctx->ineligible_until[i]) {
				continue;
			}
			const char *reason;
			if (!indigo_scheduler_points_ok(sky, tasks[i].points, tasks[i].header, t, t + tasks[i].duration, &reason)) {
				// the search for the next start skips only starts that cannot pass, except within a nightly window, which may open at any minute
				if (tasks[i].header->window_kind != SCHEDULER_WINDOW_NIGHTLY) {
					time_t start = indigo_scheduler_points_next(sky, tasks[i].points, tasks[i].header, t, tasks[i].duration, ctx->end + tasks[i].duration, NULL);
					ctx->ineligible_from[i] = t;
					ctx->ineligible_until[i] = start > 0 ? start : ctx->end + 1;
				}
				continue;
			}
			// the same order as the scheduler: urgent tasks as submitted, then the highest score, ties to the earlier task
			bool urgent = tasks[i].header->priority_class == SCHEDULER_PRIORITY_URGENT;
			double value = urgent ? 0 : indigo_scheduler_score(sky, tasks[i].target, tasks[i].points, tasks[i].header, t, ctx->setting_weight, ctx->altitude_weight);
			if (best < 0) {
				best = i;
				best_score = value;
			} else if (tasks[best].header->priority_class == SCHEDULER_PRIORITY_URGENT) {
				if (urgent && state->urgent_order[i] < state->urgent_order[best]) {
					best = i;
				}
			} else if (urgent || value > best_score) {
				best = i;
				best_score = value;
			}
		}
		if (best >= 0) {
			const scheduler_header *header = tasks[best].header;
			runs[stored].task = best;
			runs[stored].hook = -1;
			runs[stored].run = header->repeat - state->runs_left[best] + 1;
			runs[stored].start = t;
			runs[stored].end = t + tasks[best].duration;
			runs[stored].running = false;
			stored++;
			state->t += tasks[best].duration;
			state->runs_left[best]--;
			state->available_from[best] = state->t + header->repeat_every * 60;
			state->urgent_order[best] = ctx->requeue_base + (unsigned long)(state->available_from[best] - ctx->now);
			continue;
		}
		// nothing can start now, continue at the first time a task can
		time_t next = 0;
		for (int i = 0; i < count; i++) {
			if (state->runs_left[i] <= 0) {
				continue;
			}
			time_t from = state->available_from[i] > t ? state->available_from[i] : t;
			// a start at or after the end ends the simulation like no start, so the search stops there
			time_t start = indigo_scheduler_points_next(sky, tasks[i].points, tasks[i].header, from, tasks[i].duration, ctx->end + tasks[i].duration, NULL);
			if (start > 0 && (next == 0 || start < next)) {
				next = start;
			}
		}
		if (state->next_hook < ctx->hook_count && hooks[state->next_hook].time < ctx->end && (next == 0 || hooks[state->next_hook].time < next)) {
			next = hooks[state->next_hook].time;
		}
		if (next == 0 || next >= ctx->end) {
			break;
		}
		state->t = next > t ? next : t + SCHEDULER_GRID_STEP;
	}
	return stored;
}

// the hooks due at the end, such as the night end hook, are planned when no run is in progress then
static int store_final_hooks(const plan_context *ctx, plan_state *state, scheduler_plan_run *runs, int stored, int max_runs) {
	const scheduler_plan_hook *hooks = ctx->hooks;
	while (state->next_hook < ctx->hook_count && stored < max_runs && hooks[state->next_hook].time <= ctx->end) {
		time_t start = hooks[state->next_hook].time > state->t ? hooks[state->next_hook].time : state->t;
		runs[stored].task = -1;
		runs[stored].hook = state->next_hook;
		runs[stored].run = 1;
		runs[stored].start = start;
		runs[stored].end = start + hooks[state->next_hook].duration;
		runs[stored].running = false;
		stored++;
		state->t = runs[stored - 1].end;
		state->next_hook++;
	}
	return stored;
}

int indigo_scheduler_plan(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs) {
	return indigo_scheduler_plan_with_hooks(sky, tasks, count, NULL, 0, now, end, setting_weight, altitude_weight, runs, max_runs);
}

int indigo_scheduler_plan_with_hooks(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs) {
	plan_context ctx;
	time_t ineligible_from[MAX_PLAN_TASKS], ineligible_until[MAX_PLAN_TASKS];
	ctx.ineligible_from = ineligible_from;
	ctx.ineligible_until = ineligible_until;
	init_context(&ctx, sky, tasks, count, hooks, hook_count, now, end, setting_weight, altitude_weight);
	// the state is large, it stays off the stack of the caller's thread
	plan_state *state = indigo_safe_malloc(sizeof(plan_state));
	int stored = init_state(&ctx, state, runs, max_runs);
	stored = simulate(&ctx, state, runs, stored, max_runs);
	stored = store_final_hooks(&ctx, state, runs, stored, max_runs);
	free(state);
	return stored;
}

// -------------------------------------------------------------------------------- VISIBILITY

// the start times are whole minutes
#define VIS_STEP	60
// the longest plan arranged, in minutes
#define VIS_MAX_MINUTES	1440
// the states of the remaining repeat spacing within one span of runs
#define VIS_MAX_UNITS	120
// the most states of one dynamic programme, which bounds its memory
#define VIS_MAX_STATES	2000000L
#define VIS_MAX_HOOKS	8
// the value of an infeasible start or order
#define VIS_NONE	-1e300
// a minute before every start
#define VIS_NO_MINUTE	-1000000

// a task run in an order, pinned to the minute pin when it is not -1
typedef struct {
	int task;
	int pin;
} vis_run;

// task runs in an order with their best starts and the summed value, VIS_NONE when they have no feasible arrangement
typedef struct {
	vis_run runs[SCHEDULER_VISIBILITY_MAX_RUNS];
	int minutes[SCHEDULER_VISIBILITY_MAX_RUNS];
	int count;
	double value;
} vis_order;

// the inputs and buffers of the dynamic programme of one plan
typedef struct {
	const plan_context *ctx;
	const plan_state *state;
	// minute g starts at base + g * VIS_STEP, the starts are the minutes 0 to minutes - 1
	time_t base;
	int minutes;
	// per task: the run length in minutes, the first minute of its first run, the repeat spacing in minutes, whether its run may start at each minute and its run quality there weighted by the run duration (NULL for a task without runs to plan), the minute of the best value and whether it may start at all
	int length[MAX_PLAN_TASKS];
	int lower[MAX_PLAN_TASKS];
	int spacing[MAX_PLAN_TASKS];
	unsigned char *ok[MAX_PLAN_TASKS];
	double *value[MAX_PLAN_TASKS];
	int best_minute[MAX_PLAN_TASKS];
	bool startable[MAX_PLAN_TASKS];
	// the hooks not yet run, by time, as their first minute; the hooks i to j - 1 move the end x of a run to max(x + shift[i][j], floor[i][j])
	int hook_count;
	int hook_minute[VIS_MAX_HOOKS];
	int hook_shift[VIS_MAX_HOOKS + 1][VIS_MAX_HOOKS + 1];
	int hook_floor[VIS_MAX_HOOKS + 1][VIS_MAX_HOOKS + 1];
	// buffers, grown as needed
	double *layers[2];
	size_t layer_size;
	int *back;
	size_t back_size;
	double *prefix;
	int *prefix_arg;
	double *window;
	int *window_arg;
	size_t window_size;
	double *keys;
	int *keys_arg;
	size_t keys_size;
	double *diagonals;
	int *diagonals_arg;
	size_t diagonals_size;
	// the cell updates done and allowed
	long work, limit;
	bool exhausted;
} vis_context;

static int max_int(int a, int b) {
	return a > b ? a : b;
}

static int min_int(int a, int b) {
	return a < b ? a : b;
}

// the first minute at or after t, negative before the first start
static int vis_minute(const vis_context *vc, time_t t) {
	if (t <= vc->base) {
		return -(int)((vc->base - t) / VIS_STEP);
	}
	return (int)((t - vc->base + VIS_STEP - 1) / VIS_STEP);
}

static bool is_urgent_task(const plan_context *ctx, int i) {
	return ctx->tasks[i].header->priority_class == SCHEDULER_PRIORITY_URGENT;
}

// the tables of a task with runs to plan
static void vis_table(vis_context *vc, int i) {
	const plan_context *ctx = vc->ctx;
	const scheduler_plan_task *task = ctx->tasks + i;
	vc->ok[i] = indigo_safe_malloc(vc->minutes);
	vc->value[i] = indigo_safe_malloc(sizeof(double) * vc->minutes);
	vc->best_minute[i] = -1;
	vc->startable[i] = false;
	// an urgent run keeps its COMPACT start, its value is needed there whatever the constraints say at the whole minute
	bool urgent = is_urgent_task(ctx, i);
	// a flip cost can make every value negative
	double best = VIS_NONE;
	for (int g = 0; g < vc->minutes; g++) {
		time_t start = vc->base + (time_t)g * VIS_STEP;
		const char *reason;
		vc->ok[i][g] = start < ctx->end && indigo_scheduler_points_ok(ctx->sky, task->points, task->header, start, start + task->duration, &reason);
		vc->value[i][g] = 0;
		if (vc->ok[i][g] || urgent) {
			double quality = indigo_scheduler_run_quality(ctx->sky, task->target, task->header, start, task->duration);
			vc->value[i][g] = (quality > 0 ? quality : 0) * task->duration / 3600.0;
			// an hour lost to a flip costs as much as an hour of full-quality imaging; an urgent run keeps its COMPACT start, its flip cannot be avoided
			if (!urgent) {
				vc->value[i][g] -= 100 * indigo_scheduler_flip_loss(ctx->sky, task->target, task->header, start, task->duration, ctx->flip);
			}
		}
		if (vc->ok[i][g] && g >= vc->lower[i]) {
			vc->startable[i] = true;
			if (vc->value[i][g] > best + QUALITY_EPSILON) {
				best = vc->value[i][g];
				vc->best_minute[i] = g;
			}
		}
	}
}

static vis_context *vis_prepare(const plan_context *ctx, const plan_state *state) {
	int pending = ctx->hook_count - state->next_hook;
	time_t base = (state->t + VIS_STEP - 1) / VIS_STEP * VIS_STEP;
	int minutes = ctx->end > base ? (int)((ctx->end - base + VIS_STEP - 1) / VIS_STEP) : 0;
	if (pending > VIS_MAX_HOOKS || minutes > VIS_MAX_MINUTES) {
		return NULL;
	}
	vis_context *vc = indigo_safe_malloc(sizeof(vis_context));
	memset(vc, 0, sizeof(vis_context));
	vc->ctx = ctx;
	vc->state = state;
	vc->base = base;
	vc->minutes = minutes;
	if (minutes <= 0) {
		return vc;
	}
	for (int i = 0; i < ctx->count; i++) {
		int length = (ctx->tasks[i].duration + VIS_STEP - 1) / VIS_STEP;
		vc->length[i] = length > 0 ? length : 1;
		vc->lower[i] = max_int(vis_minute(vc, state->available_from[i]), 0);
		vc->spacing[i] = ctx->tasks[i].header->repeat_every > 0 ? ctx->tasks[i].header->repeat_every : 0;
		if (state->runs_left[i] > 0) {
			vis_table(vc, i);
		}
	}
	vc->hook_count = pending;
	int length[VIS_MAX_HOOKS];
	for (int h = 0; h < pending; h++) {
		const scheduler_plan_hook *hook = ctx->hooks + state->next_hook + h;
		vc->hook_minute[h] = vis_minute(vc, hook->time);
		length[h] = (hook->duration + VIS_STEP - 1) / VIS_STEP;
	}
	for (int i = 0; i < pending; i++) {
		int shift = 0, floor = VIS_NO_MINUTE;
		for (int j = i + 1; j <= pending; j++) {
			shift += length[j - 1];
			floor = max_int(floor, vc->hook_minute[j - 1]) + length[j - 1];
			vc->hook_shift[i][j] = shift;
			vc->hook_floor[i][j] = floor;
		}
	}
	vc->prefix = indigo_safe_malloc(sizeof(double) * minutes);
	vc->prefix_arg = indigo_safe_malloc(sizeof(int) * minutes);
	return vc;
}

static void vis_release(vis_context *vc) {
	if (vc == NULL) {
		return;
	}
	for (int i = 0; i < MAX_PLAN_TASKS; i++) {
		indigo_safe_free(vc->ok[i]);
		indigo_safe_free(vc->value[i]);
	}
	indigo_safe_free(vc->layers[0]);
	indigo_safe_free(vc->layers[1]);
	indigo_safe_free(vc->back);
	indigo_safe_free(vc->prefix);
	indigo_safe_free(vc->prefix_arg);
	indigo_safe_free(vc->window);
	indigo_safe_free(vc->window_arg);
	indigo_safe_free(vc->keys);
	indigo_safe_free(vc->keys_arg);
	indigo_safe_free(vc->diagonals);
	indigo_safe_free(vc->diagonals_arg);
	free(vc);
}

// grows a pair of buffers of size elements each
static void vis_reserve(double **values, int **args, size_t *size, size_t needed) {
	if (needed > *size) {
		*values = indigo_safe_realloc(*values, sizeof(double) * needed);
		*args = indigo_safe_realloc(*args, sizeof(int) * needed);
		*size = needed;
	}
}

// the value of a run starting at minute g, VIS_NONE when it may not start there
static double vis_value(const vis_context *vc, const vis_run *run, int g, int lower) {
	if (g < lower) {
		return VIS_NONE;
	}
	if (run->pin >= 0) {
		return g == run->pin ? vc->value[run->task][g] : VIS_NONE;
	}
	return vc->ok[run->task][g] ? vc->value[run->task][g] : VIS_NONE;
}

// checks an arrangement in seconds against the hooks, the night end, the delays and the repeat spacing, as the scheduler would run it
static bool vis_valid(const vis_context *vc, const vis_run *runs, const int *minutes, int count) {
	const plan_context *ctx = vc->ctx;
	const scheduler_plan_hook *hooks = ctx->hooks;
	time_t x = vc->state->t;
	int next_hook = vc->state->next_hook;
	time_t last_end[MAX_PLAN_TASKS];
	bool seen[MAX_PLAN_TASKS] = { false };
	for (int k = 0; k < count; k++) {
		int i = runs[k].task;
		const scheduler_plan_task *task = ctx->tasks + i;
		time_t start = vc->base + (time_t)minutes[k] * VIS_STEP;
		while (next_hook < ctx->hook_count && hooks[next_hook].time <= start) {
			x = (hooks[next_hook].time > x ? hooks[next_hook].time : x) + hooks[next_hook].duration;
			next_hook++;
		}
		if (start < x || start >= ctx->end) {
			return false;
		}
		if (seen[i] ? start < last_end[i] + task->header->repeat_every * 60 : start < vc->state->available_from[i]) {
			return false;
		}
		seen[i] = true;
		last_end[i] = x = start + task->duration;
	}
	return true;
}

// the best starts of task runs in a fixed order and their summed value, VIS_NONE when the order has no feasible arrangement or the work budget is used up; a dynamic programme over the start minute of each run, extended by the remaining repeat spacing within a span of runs between two runs of a task
static double vis_arrange(vis_context *vc, const vis_run *runs, int count, int *minutes) {
	if (count <= 0) {
		return 0;
	}
	if (vc->minutes <= 0) {
		return VIS_NONE;
	}
	if (vc->work >= vc->limit) {
		vc->exhausted = true;
		return VIS_NONE;
	}
	int grid = vc->minutes;
	int length[SCHEDULER_VISIBILITY_MAX_RUNS], local[SCHEDULER_VISIBILITY_MAX_RUNS], lower[SCHEDULER_VISIBILITY_MAX_RUNS], span[SCHEDULER_VISIBILITY_MAX_RUNS], units[SCHEDULER_VISIBILITY_MAX_RUNS], unit[SCHEDULER_VISIBILITY_MAX_RUNS], width[SCHEDULER_VISIBILITY_MAX_RUNS];
	long offset[SCHEDULER_VISIBILITY_MAX_RUNS];
	for (int k = 0; k < count; k++) {
		length[k] = vc->length[runs[k].task];
		local[k] = lower[k] = units[k] = 0;
		span[k] = -1;
		unit[k] = 1;
	}
	// the first run of a task starts after its delay; the next one keeps the repeat spacing after the previous one: as a minimal gap when they are adjacent, otherwise the gaps of the span of runs from the previous one must add up to the part of the spacing the runs between do not fill
	for (int k = 0; k < count; k++) {
		int i = runs[k].task, previous = -1;
		for (int j = k - 1; j >= 0 && previous < 0; j--) {
			if (runs[j].task == i) {
				previous = j;
			}
		}
		if (previous < 0) {
			lower[k] = vc->lower[i];
		} else if (previous == k - 1) {
			local[k] = vc->spacing[i];
		} else {
			int need = length[previous] + vc->spacing[i];
			for (int j = previous; j < k; j++) {
				need -= length[j];
			}
			if (need > grid) {
				return VIS_NONE;
			}
			if (need > 0) {
				// a longer spacing is counted in units of several minutes, the gaps rounded down
				int u = (need + VIS_MAX_UNITS - 1) / VIS_MAX_UNITS;
				for (int j = previous; j < k; j++) {
					// two spans over the same run are not arranged
					if (span[j] >= 0) {
						return VIS_NONE;
					}
					span[j] = previous;
					units[j] = (need + u - 1) / u;
					unit[j] = u;
				}
			}
		}
	}
	long states = 0;
	int widest = 1;
	for (int k = 0; k < count; k++) {
		// neither is a minimal gap within a span
		if (k > 0 && span[k - 1] >= 0 && local[k] > 0) {
			return VIS_NONE;
		}
		width[k] = (span[k] >= 0 ? units[k] : 0) + 1;
		widest = max_int(widest, width[k]);
		offset[k] = states;
		states += (long)grid * width[k];
	}
	if (states > VIS_MAX_STATES) {
		return VIS_NONE;
	}
	if (states > (long)vc->back_size) {
		vc->back = indigo_safe_realloc(vc->back, sizeof(int) * states);
		vc->back_size = (size_t)states;
	}
	if ((size_t)grid * widest > vc->layer_size) {
		vc->layer_size = (size_t)grid * widest;
		vc->layers[0] = indigo_safe_realloc(vc->layers[0], sizeof(double) * vc->layer_size);
		vc->layers[1] = indigo_safe_realloc(vc->layers[1], sizeof(double) * vc->layer_size);
	}
	int hook_count = vc->hook_count;
	double *previous_layer = NULL;
	for (int k = 0; k < count; k++) {
		int w = width[k];
		// the first run of a span starts it with the whole spacing to fill
		int initial = span[k] == k ? w - 1 : 0;
		double *layer = vc->layers[k & 1];
		int *back = vc->back + offset[k];
		for (long x = 0; x < (long)grid * w; x++) {
			layer[x] = VIS_NONE;
			back[x] = -1;
		}
		if (k == 0) {
			// the hooks due up to the first start run before it, after the running task or hook
			int due = 0;
			for (int g = 0; g < grid; g++) {
				while (due < hook_count && vc->hook_minute[due] <= g) {
					due++;
				}
				double value = vis_value(vc, runs, g, lower[0]);
				if (value > VIS_NONE && (due == 0 || max_int(vc->hook_shift[0][due], vc->hook_floor[0][due]) <= g)) {
					layer[g * w + initial] = value;
				}
			}
			vc->work += grid;
		} else if (width[k - 1] == 1) {
			// the hooks due after the start of the previous run and up to this start run between them; the best previous start is taken in each stretch between hook minutes, followed by the hooks after the stretch
			int d = length[k - 1];
			double *prefix = vc->prefix;
			int *prefix_arg = vc->prefix_arg;
			int h = 0;
			for (int g = 0; g < grid; g++) {
				bool restart = g == 0;
				while (h < hook_count && vc->hook_minute[h] <= g) {
					restart = restart || vc->hook_minute[h] == g;
					h++;
				}
				if (restart || previous_layer[g] > prefix[g - 1]) {
					prefix[g] = previous_layer[g];
					prefix_arg[g] = g;
				} else {
					prefix[g] = prefix[g - 1];
					prefix_arg[g] = prefix_arg[g - 1];
				}
			}
			int due = 0;
			for (int g = 0; g < grid; g++) {
				while (due < hook_count && vc->hook_minute[due] <= g) {
					due++;
				}
				double value = vis_value(vc, runs + k, g, lower[k]);
				if (value <= VIS_NONE) {
					continue;
				}
				double best = VIS_NONE;
				int arg = -1;
				for (int i = 0; i <= due; i++) {
					int from = i == 0 ? 0 : max_int(vc->hook_minute[i - 1], 0);
					int to = i < due ? vc->hook_minute[i] - 1 : grid - 1;
					int shift = i < due ? vc->hook_shift[i][due] : 0;
					if (i < due && vc->hook_floor[i][due] > g) {
						continue;
					}
					to = min_int(to, g - d - max_int(shift, local[k]));
					if (to >= from && prefix[to] > best) {
						best = prefix[to];
						arg = prefix_arg[to];
					}
				}
				if (best > VIS_NONE) {
					layer[g * w + initial] = value + best;
					back[g * w + initial] = arg;
				}
			}
			vc->work += (long)grid * (hook_count + 2);
		} else {
			// within a span the remaining spacing r of the previous run falls by the whole units u in the gap; a hook due within a span is rejected by the check of the result
			int d = length[k - 1], pw = width[k - 1], np = pw - 1, u = unit[k - 1];
			bool continues = span[k] >= 0 && span[k] == span[k - 1];
			int key_count = grid + np * u;
			// the best previous start over the u minutes up to each minute, for each remaining spacing
			const double *window = previous_layer;
			const int *window_arg = NULL;
			if (u > 1) {
				vis_reserve(&vc->window, &vc->window_arg, &vc->window_size, (size_t)grid * pw);
				for (int e = 0; e < grid; e++) {
					for (int r = 0; r < pw; r++) {
						double best = VIS_NONE;
						int arg = -1;
						for (int g = max_int(e - u + 1, 0); g <= e; g++) {
							if (previous_layer[g * pw + r] > best) {
								best = previous_layer[g * pw + r];
								arg = g * pw + r;
							}
						}
						vc->window[e * pw + r] = best;
						vc->window_arg[e * pw + r] = arg;
					}
				}
				window = vc->window;
				window_arg = vc->window_arg;
				vc->work += (long)grid * pw * u;
			}
			// the spacing is filled when start + r * u of the previous run is at most this start minus the previous run: the best such previous start by that key
			vis_reserve(&vc->keys, &vc->keys_arg, &vc->keys_size, (size_t)key_count);
			for (int key = 0; key < key_count; key++) {
				vc->keys[key] = VIS_NONE;
				vc->keys_arg[key] = -1;
			}
			for (int g = 0; g < grid; g++) {
				for (int r = 0; r < pw; r++) {
					int key = g + r * u;
					if (previous_layer[g * pw + r] > vc->keys[key]) {
						vc->keys[key] = previous_layer[g * pw + r];
						vc->keys_arg[key] = g * pw + r;
					}
				}
			}
			for (int key = 1; key < key_count; key++) {
				if (!(vc->keys[key] > vc->keys[key - 1])) {
					vc->keys[key] = vc->keys[key - 1];
					vc->keys_arg[key] = vc->keys_arg[key - 1];
				}
			}
			// r units remain when the previous start in window e with r' >= r units lies on the diagonal e + r' * u = this start minus the previous run + r * u: the best along each diagonal from r' up
			if (continues) {
				vis_reserve(&vc->diagonals, &vc->diagonals_arg, &vc->diagonals_size, (size_t)key_count * pw);
				for (int key = 0; key < key_count; key++) {
					double best = VIS_NONE;
					int arg = -1;
					for (int r = np; r >= 0; r--) {
						int e = key - r * u;
						if (e >= 0 && e < grid && window[e * pw + r] > best) {
							best = window[e * pw + r];
							arg = window_arg != NULL ? window_arg[e * pw + r] : e * pw + r;
						}
						vc->diagonals[key * pw + r] = best;
						vc->diagonals_arg[key * pw + r] = arg;
					}
				}
			}
			for (int g = 0; g < grid; g++) {
				double value = vis_value(vc, runs + k, g, lower[k]);
				int c = g - d;
				if (value <= VIS_NONE || c < 0) {
					continue;
				}
				int key = min_int(c, key_count - 1);
				if (vc->keys[key] > VIS_NONE) {
					int r = continues ? 0 : initial;
					layer[g * w + r] = value + vc->keys[key];
					back[g * w + r] = vc->keys_arg[key];
				}
				for (int r = 1; continues && r < w && c + r * u < key_count; r++) {
					int diagonal = (c + r * u) * pw + r;
					if (vc->diagonals[diagonal] > VIS_NONE) {
						layer[g * w + r] = value + vc->diagonals[diagonal];
						back[g * w + r] = vc->diagonals_arg[diagonal];
					}
				}
			}
			vc->work += (long)key_count * pw * (continues ? 2 : 1) + (long)grid * w;
		}
		previous_layer = layer;
	}
	// the earliest best start of the last run, then back to the first one
	int last = count - 1;
	double best = VIS_NONE;
	long arg = -1;
	for (int g = 0; g < grid; g++) {
		if (previous_layer[g * width[last]] > best) {
			best = previous_layer[g * width[last]];
			arg = g * width[last];
		}
	}
	if (arg < 0) {
		return VIS_NONE;
	}
	for (int k = last; k >= 0; k--) {
		minutes[k] = (int)(arg / width[k]);
		arg = vc->back[offset[k] + arg];
	}
	if (!vis_valid(vc, runs, minutes, count)) {
		return VIS_NONE;
	}
	return best;
}

static void vis_evaluate(vis_context *vc, vis_order *order) {
	order->value = vis_arrange(vc, order->runs, order->count, order->minutes);
}

// order with the run at position from moved to position to
static void vis_move(const vis_order *order, int from, int to, vis_order *result) {
	vis_run run = order->runs[from];
	int n = 0;
	for (int k = 0; k < order->count; k++) {
		if (k != from) {
			result->runs[n++] = order->runs[k];
		}
	}
	for (int k = n; k > to; k--) {
		result->runs[k] = result->runs[k - 1];
	}
	result->runs[to] = run;
	result->count = order->count;
}

// order with run inserted at position
static void vis_insert(const vis_order *order, int position, vis_run run, vis_order *result) {
	for (int k = 0; k < position; k++) {
		result->runs[k] = order->runs[k];
	}
	result->runs[position] = run;
	for (int k = position; k < order->count; k++) {
		result->runs[k + 1] = order->runs[k];
	}
	result->count = order->count + 1;
}

// inserts a run at the position with the highest value; false when it fits nowhere
static bool vis_insert_best(vis_context *vc, vis_order *order, vis_run run) {
	if (order->count >= SCHEDULER_VISIBILITY_MAX_RUNS) {
		return false;
	}
	vis_order *best = indigo_safe_malloc(sizeof(vis_order)), *trial = indigo_safe_malloc(sizeof(vis_order));
	best->value = VIS_NONE;
	for (int position = 0; position <= order->count; position++) {
		vis_insert(order, position, run, trial);
		vis_evaluate(vc, trial);
		if (trial->value > VIS_NONE && (best->value <= VIS_NONE || trial->value > best->value + QUALITY_EPSILON)) {
			*best = *trial;
		}
	}
	bool found = best->value > VIS_NONE;
	if (found) {
		*order = *best;
	}
	free(best);
	free(trial);
	return found;
}

static int vis_runs_of(const vis_order *order, int i) {
	int result = 0;
	for (int k = 0; k < order->count; k++) {
		if (order->runs[k].task == i) {
			result++;
		}
	}
	return result;
}

// adds the runs COMPACT left out, each where the value is highest, as long as one fits
static void vis_add_left_out(vis_context *vc, vis_order *order) {
	const plan_context *ctx = vc->ctx;
	vis_order *best = indigo_safe_malloc(sizeof(vis_order)), *trial = indigo_safe_malloc(sizeof(vis_order));
	while (order->count < SCHEDULER_VISIBILITY_MAX_RUNS && !vc->exhausted) {
		best->value = VIS_NONE;
		for (int i = 0; i < ctx->count; i++) {
			if (vc->ok[i] == NULL || !vc->startable[i] || is_urgent_task(ctx, i) || vis_runs_of(order, i) >= vc->state->runs_left[i]) {
				continue;
			}
			vis_run run = { i, -1 };
			for (int position = 0; position <= order->count; position++) {
				vis_insert(order, position, run, trial);
				vis_evaluate(vc, trial);
				if (trial->value > VIS_NONE && (best->value <= VIS_NONE || trial->value > best->value + QUALITY_EPSILON)) {
					*best = *trial;
				}
			}
		}
		if (best->value <= VIS_NONE) {
			break;
		}
		*order = *best;
	}
	free(best);
	free(trial);
}

// local search: moves one run to another position while that raises the value, the first improvement found in a fixed scan order
static void vis_improve(vis_context *vc, vis_order *order) {
	vis_order *trial = indigo_safe_malloc(sizeof(vis_order));
	bool improved = true;
	while (improved && !vc->exhausted) {
		improved = false;
		for (int from = 0; from < order->count && !improved && !vc->exhausted; from++) {
			if (order->runs[from].pin >= 0) {
				continue;
			}
			for (int to = 0; to < order->count && !improved && !vc->exhausted; to++) {
				if (to == from || order->runs[to].task == order->runs[from].task) {
					continue;
				}
				vis_move(order, from, to, trial);
				vis_evaluate(vc, trial);
				if (trial->value > VIS_NONE && trial->value > order->value + QUALITY_EPSILON) {
					*order = *trial;
					improved = true;
				}
			}
		}
	}
	free(trial);
}

// the search from one seed within the work allowed up to limit
static void vis_search(vis_context *vc, vis_order *order, long limit) {
	vc->limit = limit;
	vc->exhausted = false;
	vis_evaluate(vc, order);
	vis_add_left_out(vc, order);
	vis_improve(vc, order);
}

// a better candidate: more task runs, then a higher value
static bool vis_better(const vis_order *order, const vis_order *other) {
	if (order->value <= VIS_NONE) {
		return false;
	}
	if (other->value <= VIS_NONE) {
		return true;
	}
	return order->count > other->count || (order->count == other->count && order->value > other->value + QUALITY_EPSILON);
}

// the seed from the previous plan: its runs that can still run, and the COMPACT runs it lacks inserted where they fit best; a lacking run that fits nowhere is left out when its priority class is not above the class of a run of the previous plan, so the plan keeps the runs it promised; false when such a run of a higher class or an urgent run fits nowhere
static bool vis_previous_seed(vis_context *vc, const vis_order *compact, const scheduler_plan_run *previous, int previous_count, vis_order *seed) {
	const plan_context *ctx = vc->ctx;
	int extra[MAX_PLAN_TASKS] = { 0 };
	seed->count = 0;
	for (int k = 0; k < previous_count; k++) {
		int i = previous[k].task;
		if (i >= 0 && i < ctx->count && !previous[k].running && !is_urgent_task(ctx, i)) {
			extra[i]++;
		}
	}
	// the runs made since the previous plan came first in it
	for (int i = 0; i < ctx->count; i++) {
		extra[i] = max_int(extra[i] - vc->state->runs_left[i], 0);
	}
	for (int k = 0; k < previous_count && seed->count < SCHEDULER_VISIBILITY_MAX_RUNS; k++) {
		int i = previous[k].task;
		if (i < 0 || i >= ctx->count || previous[k].running || is_urgent_task(ctx, i) || vc->state->runs_left[i] <= 0) {
			continue;
		}
		if (extra[i] > 0) {
			extra[i]--;
			continue;
		}
		seed->runs[seed->count].task = i;
		seed->runs[seed->count].pin = -1;
		seed->count++;
	}
	vis_evaluate(vc, seed);
	int lowest = SCHEDULER_PRIORITY_HOOK;
	for (int k = 0; k < seed->count; k++) {
		lowest = min_int(lowest, ctx->tasks[seed->runs[k].task].header->priority_class);
	}
	int missing[MAX_PLAN_TASKS];
	for (int i = 0; i < ctx->count; i++) {
		missing[i] = vis_runs_of(compact, i) - vis_runs_of(seed, i);
	}
	for (int k = 0; k < compact->count; k++) {
		const vis_run *run = compact->runs + k;
		if (run->pin >= 0 || missing[run->task] > 0) {
			missing[run->task]--;
			if (!vis_insert_best(vc, seed, *run) && (run->pin >= 0 || (int)ctx->tasks[run->task].header->priority_class > lowest)) {
				return false;
			}
		}
	}
	return true;
}

// stores the runs of an order with the hooks between them and after them
static int vis_store(const vis_context *vc, const vis_order *order, scheduler_plan_run *runs, int stored, int max_runs) {
	const plan_context *ctx = vc->ctx;
	const scheduler_plan_hook *hooks = ctx->hooks;
	plan_state *state = indigo_safe_malloc(sizeof(plan_state));
	*state = *vc->state;
	for (int k = 0; k < order->count && stored < max_runs; k++) {
		int i = order->runs[k].task;
		time_t start = vc->base + (time_t)order->minutes[k] * VIS_STEP;
		while (state->next_hook < ctx->hook_count && hooks[state->next_hook].time <= start && stored < max_runs) {
			time_t from = hooks[state->next_hook].time > state->t ? hooks[state->next_hook].time : state->t;
			runs[stored].task = -1;
			runs[stored].hook = state->next_hook;
			runs[stored].run = 1;
			runs[stored].start = from;
			runs[stored].end = from + hooks[state->next_hook].duration;
			runs[stored].running = false;
			stored++;
			state->t = from + hooks[state->next_hook].duration;
			state->next_hook++;
		}
		if (stored >= max_runs) {
			break;
		}
		runs[stored].task = i;
		runs[stored].hook = -1;
		runs[stored].run = ctx->tasks[i].header->repeat - state->runs_left[i] + 1;
		runs[stored].start = start;
		runs[stored].end = start + ctx->tasks[i].duration;
		runs[stored].running = false;
		stored++;
		state->runs_left[i]--;
		state->t = start + ctx->tasks[i].duration;
	}
	stored = store_final_hooks(ctx, state, runs, stored, max_runs);
	free(state);
	return stored;
}

int indigo_scheduler_plan_visibility(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, const scheduler_flip *flip, const scheduler_plan_run *previous, int previous_count, scheduler_plan_run *runs, int max_runs, scheduler_plan_info *info) {
	double started = indigo_monotonic_time();
	scheduler_plan_info local_info;
	if (info == NULL) {
		info = &local_info;
	}
	memset(info, 0, sizeof(scheduler_plan_info));
	// the COMPACT plan decides which runs are planned at least
	int stored = indigo_scheduler_plan_with_hooks(sky, tasks, count, hooks, hook_count, now, end, setting_weight, altitude_weight, runs, max_runs);
	plan_context ctx;
	time_t ineligible_from[MAX_PLAN_TASKS], ineligible_until[MAX_PLAN_TASKS];
	ctx.ineligible_from = ineligible_from;
	ctx.ineligible_until = ineligible_until;
	init_context(&ctx, sky, tasks, count, hooks, hook_count, now, end, setting_weight, altitude_weight);
	ctx.flip = flip;
	plan_state *state = indigo_safe_malloc(sizeof(plan_state));
	init_state(&ctx, state, NULL, 0);
	int fixed = 0;
	while (fixed < stored && runs[fixed].running) {
		fixed++;
	}
	vis_order *compact = indigo_safe_malloc(sizeof(vis_order));
	time_t compact_start[SCHEDULER_VISIBILITY_MAX_RUNS];
	compact->count = 0;
	compact->value = 0;
	bool too_large = stored >= max_runs;
	for (int k = fixed; k < stored; k++) {
		int i = runs[k].task;
		if (i < 0) {
			continue;
		}
		info->compact_runs++;
		double quality = indigo_scheduler_run_quality(sky, tasks[i].target, tasks[i].header, runs[k].start, tasks[i].duration);
		info->compact_value += (quality > 0 ? quality : 0) * tasks[i].duration / 3600.0;
		// COMPACT is compared with the same flip costs
		if (!is_urgent_task(&ctx, i)) {
			info->compact_value -= 100 * indigo_scheduler_flip_loss(sky, tasks[i].target, tasks[i].header, runs[k].start, tasks[i].duration, flip);
		}
		if (compact->count < SCHEDULER_VISIBILITY_MAX_RUNS) {
			compact->runs[compact->count].task = i;
			compact->runs[compact->count].pin = -1;
			compact_start[compact->count] = runs[k].start;
			compact->count++;
		} else {
			too_large = true;
		}
	}
	info->runs = info->compact_runs;
	info->value = info->compact_value;
	vis_context *vc = too_large ? NULL : vis_prepare(&ctx, state);
	if (vc == NULL) {
		info->result = SCHEDULER_VISIBILITY_TOO_LARGE;
	} else {
		// an urgent run keeps its COMPACT start, the others are free
		bool pins_ok = true;
		for (int k = 0; k < compact->count; k++) {
			vis_run *run = compact->runs + k;
			if (is_urgent_task(&ctx, run->task)) {
				run->pin = vis_minute(vc, compact_start[k]);
				pins_ok = pins_ok && run->pin >= 0 && run->pin < vc->minutes;
			}
		}
		compact->value = info->compact_value;
		vis_order *best = indigo_safe_malloc(sizeof(vis_order)), *seed = indigo_safe_malloc(sizeof(vis_order));
		best->count = 0;
		best->value = VIS_NONE;
		if (pins_ok && vc->minutes > 0) {
			long budget = SCHEDULER_VISIBILITY_BUDGET;
			// the rest of the previous plan comes first and wins ties, then the COMPACT order, then the COMPACT runs by the time of their best quality
			long limit = budget / 3;
			vc->limit = limit;
			if (previous != NULL && previous_count > 0 && vis_previous_seed(vc, compact, previous, previous_count, seed)) {
				vis_search(vc, seed, limit);
				*best = *seed;
			}
			*seed = *compact;
			vis_search(vc, seed, vc->work + (budget - vc->work) / 2);
			if (vis_better(seed, best)) {
				*best = *seed;
			}
			*seed = *compact;
			for (int a = 1; a < seed->count; a++) {
				for (int b = a; b > 0; b--) {
					const vis_run *x = seed->runs + b - 1, *y = seed->runs + b;
					int key_x = x->pin >= 0 ? x->pin : vc->best_minute[x->task], key_y = y->pin >= 0 ? y->pin : vc->best_minute[y->task];
					if (key_y >= key_x) {
						break;
					}
					vis_run swap = *x;
					seed->runs[b - 1] = *y;
					seed->runs[b] = swap;
				}
			}
			vis_search(vc, seed, budget);
			if (vis_better(seed, best)) {
				*best = *seed;
			}
		}
		if (compact->count == 0 && best->count == 0) {
			info->result = SCHEDULER_VISIBILITY_NOT_BETTER;
		} else if (best->value <= VIS_NONE || best->count < compact->count) {
			info->result = SCHEDULER_VISIBILITY_NO_ARRANGEMENT;
		} else if (best->count == compact->count && best->value <= compact->value + QUALITY_EPSILON) {
			info->result = SCHEDULER_VISIBILITY_NOT_BETTER;
		} else {
			info->result = SCHEDULER_VISIBILITY_PLANNED;
			info->runs = best->count;
			info->value = best->value;
			stored = vis_store(vc, best, runs, fixed, max_runs);
		}
		info->work = vc->work;
		free(best);
		free(seed);
		vis_release(vc);
	}
	free(compact);
	free(state);
	info->seconds = indigo_monotonic_time() - started;
	return stored;
}

double indigo_scheduler_plan_order(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, const int *order, int order_count, time_t *starts) {
	if (order_count > SCHEDULER_VISIBILITY_MAX_RUNS) {
		return -1;
	}
	plan_context ctx;
	time_t ineligible_from[MAX_PLAN_TASKS], ineligible_until[MAX_PLAN_TASKS];
	ctx.ineligible_from = ineligible_from;
	ctx.ineligible_until = ineligible_until;
	init_context(&ctx, sky, tasks, count, hooks, hook_count, now, end, 0, 0);
	plan_state *state = indigo_safe_malloc(sizeof(plan_state));
	init_state(&ctx, state, NULL, 0);
	vis_context *vc = vis_prepare(&ctx, state);
	double result = -1;
	if (vc != NULL) {
		vis_order *arranged = indigo_safe_malloc(sizeof(vis_order));
		arranged->count = 0;
		for (int k = 0; k < order_count; k++) {
			int i = order[k];
			if (i < 0 || i >= ctx.count || vc->ok[i] == NULL) {
				arranged->count = -1;
				break;
			}
			arranged->runs[k].task = i;
			arranged->runs[k].pin = -1;
			arranged->count++;
		}
		if (arranged->count == order_count) {
			vc->limit = SCHEDULER_VISIBILITY_BUDGET * 1000;
			vis_evaluate(vc, arranged);
			if (arranged->value > VIS_NONE) {
				result = arranged->value;
				for (int k = 0; k < order_count; k++) {
					starts[k] = vc->base + (time_t)arranged->minutes[k] * VIS_STEP;
				}
			}
		}
		free(arranged);
		vis_release(vc);
	}
	free(state);
	return result;
}
