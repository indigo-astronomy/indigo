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

#include "indigo_agent_scheduler_plan.h"

#define MAX_PLAN_TASKS	256

// summed qualities closer than this are equal, a start must be better by more than this to be the best one
#define QUALITY_EPSILON	1e-6

// the inputs of a simulation, shared by the plan and its nested simulations
typedef struct {
	const scheduler_sky_grid *sky;
	const scheduler_plan_task *tasks;
	int count;
	const scheduler_plan_hook *hooks;
	int hook_count;
	time_t now, end;
	double setting_weight, altitude_weight, defer_gain;
	unsigned long requeue_base;
	// per task, a stretch [from, until) of start times at which the task is known not to be eligible; shared by all simulations of the plan, it depends only on the constraints
	time_t *ineligible_from, *ineligible_until;
} plan_context;

// the simulation works on its own copy of the counters, which a nested simulation copies again
typedef struct {
	time_t t;
	int next_hook;
	int runs_left[MAX_PLAN_TASKS];
	time_t available_from[MAX_PLAN_TASKS];
	unsigned long urgent_order[MAX_PLAN_TASKS];
	// when a held task would have started without the hold, 0 when it is not held
	time_t deferred_from[MAX_PLAN_TASKS];
} plan_state;

// the result of a nested simulation: the planned runs of each task and their summed quality
typedef struct {
	int counts[MAX_PLAN_TASKS];
	double quality;
} plan_outcome;

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

// the release of a candidate at t: the first eligible start within defer_gain of its best start before the end, 0 when the best start is not better by defer_gain than t
static time_t defer_release(const plan_context *ctx, int i, time_t t) {
	const scheduler_sky_grid *sky = ctx->sky;
	const scheduler_plan_task *task = ctx->tasks + i;
	if (ctx->defer_gain <= 0 || !quality_known(sky, task->target, task->header, t)) {
		return 0;
	}
	int points = task->duration / SCHEDULER_GRID_STEP + 1;
	int first = (int)((t - sky->start) / SCHEDULER_GRID_STEP);
	if (first + points > SCHEDULER_GRID_POINTS) {
		return 0;
	}
	// running sums over the whole grid, the same at every decision, give the quality of a start in constant time
	double sum[SCHEDULER_GRID_POINTS + 1];
	sum[0] = 0;
	for (int k = 0; k < SCHEDULER_GRID_POINTS; k++) {
		sum[k + 1] = sum[k] + relative_altitude(task->target, k);
	}
	double now_quality = (sum[first + points] - sum[first]) / points;
	time_t best = 0;
	double best_quality = 0;
	const char *reason;
	for (time_t s = t; s < ctx->end; s = s - s % SCHEDULER_GRID_STEP + SCHEDULER_GRID_STEP) {
		int k = (int)((s - sky->start) / SCHEDULER_GRID_STEP);
		if (k + points > SCHEDULER_GRID_POINTS) {
			break;
		}
		if (!indigo_scheduler_points_ok(sky, task->points, task->header, s, s + task->duration, &reason)) {
			continue;
		}
		double quality = (sum[k + points] - sum[k]) / points;
		// ties go to the earliest start
		if (best == 0 || quality > best_quality + QUALITY_EPSILON) {
			best = s;
			best_quality = quality;
		}
	}
	if (best <= t || best_quality - now_quality < ctx->defer_gain) {
		return 0;
	}
	// the task is held until the first start close enough to the best one, so a decision taken again a little later finds the same release
	for (time_t s = t; s < best; s = s - s % SCHEDULER_GRID_STEP + SCHEDULER_GRID_STEP) {
		int k = (int)((s - sky->start) / SCHEDULER_GRID_STEP);
		if (best_quality - (sum[k + points] - sum[k]) / points < ctx->defer_gain && indigo_scheduler_points_ok(sky, task->points, task->header, s, s + task->duration, &reason)) {
			return s;
		}
	}
	return best;
}

static void init_context(plan_context *ctx, const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, double defer_gain) {
	ctx->sky = sky;
	ctx->tasks = tasks;
	ctx->count = count > MAX_PLAN_TASKS ? MAX_PLAN_TASKS : count;
	ctx->hooks = hooks;
	ctx->hook_count = hook_count;
	ctx->now = now;
	ctx->end = end;
	ctx->setting_weight = setting_weight;
	ctx->altitude_weight = altitude_weight;
	ctx->defer_gain = defer_gain;
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
		state->deferred_from[i] = 0;
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
			runs[stored].deferred_from = 0;
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
				runs[stored].deferred_from = 0;
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

static int decide(const plan_context *ctx, plan_state *state, const int *ranked, int ranked_count, int *waiting, time_t *release);

// a run or a hook starts: the scheduler keeps no holds and decides again when it ends, so the held tasks become candidates again
static void release_holds(const plan_context *ctx, plan_state *state) {
	for (int i = 0; i < ctx->count; i++) {
		if (state->deferred_from[i] != 0 && state->available_from[i] > state->t) {
			state->available_from[i] = state->t;
		}
	}
}

// simulates from the state until the end, deferring tasks when defer is set; stores the runs when runs is not NULL, counts them and sums their quality when outcome is not NULL
static int simulate(const plan_context *ctx, plan_state *state, bool defer, scheduler_plan_run *runs, int stored, int max_runs, plan_outcome *outcome) {
	const scheduler_sky_grid *sky = ctx->sky;
	const scheduler_plan_task *tasks = ctx->tasks;
	const scheduler_plan_hook *hooks = ctx->hooks;
	int count = ctx->count;
	if (outcome != NULL) {
		memset(outcome, 0, sizeof(*outcome));
	}
	while (state->t < ctx->end && (runs == NULL || stored < max_runs)) {
		time_t t = state->t;
		// a hook never interrupts a run, it runs when the run ends
		if (state->next_hook < ctx->hook_count && hooks[state->next_hook].time <= t) {
			if (runs != NULL) {
				runs[stored].task = -1;
				runs[stored].hook = state->next_hook;
				runs[stored].run = 1;
				runs[stored].start = t;
				runs[stored].end = t + hooks[state->next_hook].duration;
				runs[stored].running = false;
				runs[stored].deferred_from = 0;
				stored++;
			}
			if (defer) {
				release_holds(ctx, state);
			}
			state->t += hooks[state->next_hook].duration;
			state->next_hook++;
			continue;
		}
		int best = -1;
		double best_score = 0;
		// the candidates without urgency by score, best first, for the deferral
		int ranked[MAX_PLAN_TASKS];
		double scores[MAX_PLAN_TASKS];
		int ranked_count = 0;
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
			if (defer && !urgent) {
				int k = ranked_count++;
				while (k > 0 && scores[k - 1] < value) {
					ranked[k] = ranked[k - 1];
					scores[k] = scores[k - 1];
					k--;
				}
				ranked[k] = i;
				scores[k] = value;
			}
		}
		// urgent tasks are never deferred
		if (defer && best >= 0 && tasks[best].header->priority_class != SCHEDULER_PRIORITY_URGENT) {
			int waiting;
			time_t release;
			best = decide(ctx, state, ranked, ranked_count, &waiting, &release);
		}
		if (best >= 0) {
			const scheduler_header *header = tasks[best].header;
			if (runs != NULL) {
				runs[stored].task = best;
				runs[stored].hook = -1;
				runs[stored].run = header->repeat - state->runs_left[best] + 1;
				runs[stored].start = t;
				runs[stored].end = t + tasks[best].duration;
				runs[stored].running = false;
				runs[stored].deferred_from = state->deferred_from[best];
				stored++;
			}
			if (outcome != NULL) {
				outcome->counts[best]++;
				double quality = indigo_scheduler_run_quality(sky, tasks[best].target, header, t, tasks[best].duration);
				if (quality > 0) {
					outcome->quality += quality;
				}
			}
			state->deferred_from[best] = 0;
			if (defer) {
				release_holds(ctx, state);
			}
			state->t += tasks[best].duration;
			state->runs_left[best]--;
			state->available_from[best] = state->t + header->repeat_every * 60;
			state->urgent_order[best] = ctx->requeue_base + (unsigned long)(state->available_from[best] - ctx->now);
			continue;
		}
		// nothing can start now, continue at the first time a task can or a held task is released
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

// the deferral decision at state->t, shared by the plan and the scheduler: holds the deferred candidates in state and returns the one to start, -1 when all of them wait
static int decide(const plan_context *ctx, plan_state *state, const int *ranked, int ranked_count, int *waiting, time_t *release) {
	plan_outcome base, held;
	bool base_known = false;
	*waiting = -1;
	*release = 0;
	for (int c = 0; c < ranked_count; c++) {
		int i = ranked[c];
		if (c >= SCHEDULER_MAX_DEFER_CANDIDATES) {
			return i;
		}
		time_t hold = defer_release(ctx, i, state->t);
		if (hold == 0) {
			return i;
		}
		// greedy simulations of the rest of the night: the state as it is, with the holds decided so far, and with this candidate held too
		plan_state copy;
		if (!base_known) {
			copy = *state;
			simulate(ctx, &copy, false, NULL, 0, 0, &base);
			base_known = true;
		}
		copy = *state;
		copy.available_from[i] = hold;
		simulate(ctx, &copy, false, NULL, 0, 0, &held);
		for (int k = 0; k < ctx->count; k++) {
			if (held.counts[k] < base.counts[k]) {
				return i;
			}
		}
		if (held.quality <= base.quality + QUALITY_EPSILON) {
			return i;
		}
		state->available_from[i] = hold;
		if (state->deferred_from[i] == 0) {
			state->deferred_from[i] = state->t;
		}
		base = held;
		if (*waiting < 0 || hold < *release) {
			*waiting = i;
			*release = hold;
		}
	}
	return -1;
}

int indigo_scheduler_plan(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs) {
	return indigo_scheduler_plan_with_deferral(sky, tasks, count, NULL, 0, now, end, setting_weight, altitude_weight, 0, runs, max_runs);
}

int indigo_scheduler_plan_with_hooks(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs) {
	return indigo_scheduler_plan_with_deferral(sky, tasks, count, hooks, hook_count, now, end, setting_weight, altitude_weight, 0, runs, max_runs);
}

int indigo_scheduler_plan_with_deferral(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, double defer_gain, scheduler_plan_run *runs, int max_runs) {
	plan_context ctx;
	time_t ineligible_from[MAX_PLAN_TASKS], ineligible_until[MAX_PLAN_TASKS];
	ctx.ineligible_from = ineligible_from;
	ctx.ineligible_until = ineligible_until;
	init_context(&ctx, sky, tasks, count, hooks, hook_count, now, end, setting_weight, altitude_weight, defer_gain);
	// the state is large, it stays off the stack of the caller's thread
	plan_state *state = indigo_safe_malloc(sizeof(plan_state));
	int stored = init_state(&ctx, state, runs, max_runs);
	stored = simulate(&ctx, state, defer_gain > 0, runs, stored, max_runs, NULL);
	// hooks due at the end, such as the night end hook, are planned when no run is in progress then
	while (state->next_hook < hook_count && stored < max_runs && hooks[state->next_hook].time <= end) {
		time_t start = hooks[state->next_hook].time > state->t ? hooks[state->next_hook].time : state->t;
		runs[stored].task = -1;
		runs[stored].hook = state->next_hook;
		runs[stored].run = 1;
		runs[stored].start = start;
		runs[stored].end = start + hooks[state->next_hook].duration;
		runs[stored].running = false;
		runs[stored].deferred_from = 0;
		stored++;
		state->t = runs[stored - 1].end;
		state->next_hook++;
	}
	free(state);
	return stored;
}

int indigo_scheduler_plan_decide(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, double defer_gain, const int *ranked, int ranked_count, int *waiting, time_t *release) {
	*waiting = -1;
	*release = 0;
	if (ranked_count <= 0) {
		return -1;
	}
	if (defer_gain <= 0) {
		return ranked[0];
	}
	plan_context ctx;
	time_t ineligible_from[MAX_PLAN_TASKS], ineligible_until[MAX_PLAN_TASKS];
	ctx.ineligible_from = ineligible_from;
	ctx.ineligible_until = ineligible_until;
	init_context(&ctx, sky, tasks, count, hooks, hook_count, now, end, setting_weight, altitude_weight, defer_gain);
	plan_state *state = indigo_safe_malloc(sizeof(plan_state));
	init_state(&ctx, state, NULL, 0);
	int result = decide(&ctx, state, ranked, ranked_count, waiting, release);
	free(state);
	return result;
}
