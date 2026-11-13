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
 \file indigo_agent_scheduler_plan.h
 */

#ifndef agent_scheduler_plan_h
#define agent_scheduler_plan_h

#include <stdbool.h>
#include <time.h>

#include "indigo_agent_scheduler_header.h"
#include "indigo_agent_scheduler_sky.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A task as the plan sees it.
 */
typedef struct {
	const scheduler_header *header;
	const scheduler_target_grid *target;
	const scheduler_task_points *points;
	bool selectable;
	int duration;
	int runs_left;
	time_t available_from;
	unsigned long urgent_order;
	bool running;
	time_t running_start, running_end;
} scheduler_plan_task;

/** A hook run expected at a time, for example at the start of the night.
 */
typedef struct {
	time_t time;
	int duration;
} scheduler_plan_hook;

/** One planned run: of a task, or of a hook when hook is not -1 (task is -1 then).
 */
typedef struct {
	int task;
	int hook;
	int run;
	time_t start, end;
	bool running;
} scheduler_plan_run;

/** Score of a task at time t; higher is selected first.
 */
extern double indigo_scheduler_score(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_task_points *points, const scheduler_header *header, time_t t, double setting_weight, double altitude_weight);

/** Simulate the scheduling of tasks from now until end, assuming every run takes its duration and succeeds. A task that becomes available again after a repeat or retry delay queues behind the urgent tasks already waiting, as in the scheduler. Returns the number of runs stored, at most max_runs.
 */
extern int indigo_scheduler_plan(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs);

/** Same as indigo_scheduler_plan(), with hook runs sorted by time. A hook runs at its time or, when a run is in progress then, after it; it takes its duration. Hooks due at end are still planned.
 */
extern int indigo_scheduler_plan_with_hooks(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs);

#ifdef __cplusplus
}
#endif

#endif /* agent_scheduler_plan_h */
