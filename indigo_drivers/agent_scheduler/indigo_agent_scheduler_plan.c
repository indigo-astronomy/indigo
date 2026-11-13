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

#include "indigo_agent_scheduler_plan.h"

#define MAX_PLAN_TASKS	256

double indigo_scheduler_score(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_task_points *points, const scheduler_header *header, time_t t, double setting_weight, double altitude_weight) {
	static const double class_base[] = { 0, 1000, 2000, 3000 };
	double result = class_base[header->priority_class] + header->level;
	int to_set = indigo_scheduler_points_time_to_set(sky, points, header, t);
	result += setting_weight * 100 * (1 - (double)to_set / SCHEDULER_SETTING_HORIZON);
	if (header->has_target && target != NULL && target->valid && indigo_scheduler_sky_covers(sky, t, t)) {
		int index = (int)((t - sky->start) / SCHEDULER_GRID_STEP);
		double altitude = target->altitude[index];
		result += altitude_weight * 100 * (altitude > 0 ? altitude : 0) / 90;
	}
	return result;
}

int indigo_scheduler_plan(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs) {
	return indigo_scheduler_plan_with_hooks(sky, tasks, count, NULL, 0, now, end, setting_weight, altitude_weight, runs, max_runs);
}

int indigo_scheduler_plan_with_hooks(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs) {
	if (count > MAX_PLAN_TASKS) {
		count = MAX_PLAN_TASKS;
	}
	// the simulation works on its own copy of the counters
	int runs_left[MAX_PLAN_TASKS];
	time_t available_from[MAX_PLAN_TASKS];
	unsigned long urgent_order[MAX_PLAN_TASKS];
	unsigned long last_order = 0;
	for (int i = 0; i < count; i++) {
		if (tasks[i].urgent_order > last_order) {
			last_order = tasks[i].urgent_order;
		}
	}
	// a task queued again after a delay gets the next urgent order at that time, later delays get later orders
	unsigned long requeue_base = last_order + 1;
	for (int i = 0; i < count; i++) {
		runs_left[i] = tasks[i].selectable ? tasks[i].runs_left : 0;
		available_from[i] = tasks[i].available_from;
		urgent_order[i] = available_from[i] > now ? requeue_base + (unsigned long)(available_from[i] - now) : tasks[i].urgent_order;
	}
	int stored = 0;
	time_t t = now;
	for (int i = 0; i < count && stored < max_runs; i++) {
		if (tasks[i].running) {
			const scheduler_header *header = tasks[i].header;
			runs[stored].task = i;
			runs[stored].hook = -1;
			runs[stored].run = header->repeat - tasks[i].runs_left + 1;
			runs[stored].start = tasks[i].running_start;
			runs[stored].end = tasks[i].running_end;
			runs[stored].running = true;
			stored++;
			if (tasks[i].running_end > t) {
				t = tasks[i].running_end;
			}
			if (runs_left[i] > 0) {
				runs_left[i]--;
				available_from[i] = tasks[i].running_end + header->repeat_every * 60;
				urgent_order[i] = requeue_base + (unsigned long)(available_from[i] - now);
			}
		}
	}
	int next_hook = 0;
	while (t < end && stored < max_runs) {
		// a hook never interrupts a run, it runs when the run ends
		if (next_hook < hook_count && hooks[next_hook].time <= t) {
			runs[stored].task = -1;
			runs[stored].hook = next_hook;
			runs[stored].run = 1;
			runs[stored].start = t;
			runs[stored].end = t + hooks[next_hook].duration;
			runs[stored].running = false;
			stored++;
			t += hooks[next_hook].duration;
			next_hook++;
			continue;
		}
		int best = -1;
		double best_score = 0;
		for (int i = 0; i < count; i++) {
			if (runs_left[i] <= 0 || available_from[i] > t) {
				continue;
			}
			const char *reason;
			if (!indigo_scheduler_points_ok(sky, tasks[i].points, tasks[i].header, t, t + tasks[i].duration, &reason)) {
				continue;
			}
			// the same order as the scheduler: urgent tasks as submitted, then the highest score, ties to the earlier task
			bool urgent = tasks[i].header->priority_class == SCHEDULER_PRIORITY_URGENT;
			double value = urgent ? 0 : indigo_scheduler_score(sky, tasks[i].target, tasks[i].points, tasks[i].header, t, setting_weight, altitude_weight);
			if (best < 0) {
				best = i;
				best_score = value;
			} else if (tasks[best].header->priority_class == SCHEDULER_PRIORITY_URGENT) {
				if (urgent && urgent_order[i] < urgent_order[best]) {
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
			runs[stored].run = header->repeat - runs_left[best] + 1;
			runs[stored].start = t;
			runs[stored].end = t + tasks[best].duration;
			runs[stored].running = false;
			stored++;
			t += tasks[best].duration;
			runs_left[best]--;
			available_from[best] = t + header->repeat_every * 60;
			urgent_order[best] = requeue_base + (unsigned long)(available_from[best] - now);
			continue;
		}
		// nothing can start now, continue at the first time a task can
		time_t next = 0;
		for (int i = 0; i < count; i++) {
			if (runs_left[i] <= 0) {
				continue;
			}
			time_t from = available_from[i] > t ? available_from[i] : t;
			time_t start = indigo_scheduler_points_next(sky, tasks[i].points, tasks[i].header, from, tasks[i].duration, 0, NULL);
			if (start > 0 && (next == 0 || start < next)) {
				next = start;
			}
		}
		if (next_hook < hook_count && hooks[next_hook].time < end && (next == 0 || hooks[next_hook].time < next)) {
			next = hooks[next_hook].time;
		}
		if (next == 0 || next >= end) {
			break;
		}
		t = next > t ? next : t + SCHEDULER_GRID_STEP;
	}
	// hooks due at the end, such as the night end hook, are planned when no run is in progress then
	while (next_hook < hook_count && stored < max_runs && hooks[next_hook].time <= end) {
		time_t start = hooks[next_hook].time > t ? hooks[next_hook].time : t;
		runs[stored].task = -1;
		runs[stored].hook = next_hook;
		runs[stored].run = 1;
		runs[stored].start = start;
		runs[stored].end = start + hooks[next_hook].duration;
		runs[stored].running = false;
		stored++;
		t = runs[stored - 1].end;
		next_hook++;
	}
	return stored;
}
