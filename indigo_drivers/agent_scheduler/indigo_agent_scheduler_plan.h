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

/** The most task runs the VISIBILITY plan arranges; a night with more is planned as COMPACT.
 */
#define SCHEDULER_VISIBILITY_MAX_RUNS	64

/** The work budget of one VISIBILITY plan, counted in cell updates of its dynamic programme, not timed, so the plan is the same on every machine.
 */
#define SCHEDULER_VISIBILITY_BUDGET	20000000L

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

/** A hook run expected at a time, for example at the start of the night. A running hook (it has to precede the other hooks) started at time; it ends at time + duration, but not earlier than a minute from now.
 */
typedef struct {
	time_t time;
	int duration;
	bool running;
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

/** How a VISIBILITY plan was decided.
 */
typedef enum {
	SCHEDULER_VISIBILITY_PLANNED,
	SCHEDULER_VISIBILITY_NOT_BETTER,
	SCHEDULER_VISIBILITY_NO_ARRANGEMENT,
	SCHEDULER_VISIBILITY_TOO_LARGE
} scheduler_visibility_result;

/** The meridian flip as the VISIBILITY plan sees it: whether the flip is enabled, its offset to the meridian in hours (negative before it) and the cost of one flip in hours of full-quality imaging.
 */
typedef struct {
	bool enabled;
	double offset;
	double penalty;
} scheduler_flip;

/** What indigo_scheduler_plan_visibility() did: the task runs and the summed quality (percent of the transit altitude times hours, less the flip costs of the runs that are not urgent) of the COMPACT plan and of the plan returned, whether the COMPACT plan was kept and why, the work used and the time taken.
 */
typedef struct {
	int compact_runs;
	double compact_value;
	int runs;
	double value;
	scheduler_visibility_result result;
	long work;
	double seconds;
} scheduler_plan_info;

/** Score of a task at time t; higher is selected first.
 */
extern double indigo_scheduler_score(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_task_points *points, const scheduler_header *header, time_t t, double setting_weight, double altitude_weight);

/** Mean altitude of the target over a run from start to start + duration, in percent of its transit altitude, on the grid points of the run (each clamped to 0..100); -1 for a task without a usable target or beyond the grid.
 */
extern double indigo_scheduler_run_quality(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, int duration);

/** True when a run from start to start + duration needs a meridian flip: it starts east of the meridian (hour angle < 0) and reaches flip->offset hours of hour angle before it ends. False for a disabled or NULL flip, a task without a usable target or a start outside the grid.
 */
extern bool indigo_scheduler_run_flips(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, int duration, const scheduler_flip *flip);

/** The cost in hours of a run's meridian flip: 0 without a flip or with flip->penalty 0, otherwise flip->penalty plus, for a negative offset, the wait -max(HA_start, offset) until the meridian.
 */
extern double indigo_scheduler_flip_loss(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, int duration, const scheduler_flip *flip);

/** Simulate the scheduling of tasks from now until end, assuming every run takes its duration and succeeds. A task that becomes available again after a repeat or retry delay queues behind the urgent tasks already waiting, as in the scheduler. Returns the number of runs stored, at most max_runs.
 */
extern int indigo_scheduler_plan(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs);

/** Same as indigo_scheduler_plan(), with hook runs sorted by time. A hook runs at its time or, when a run is in progress then, after it; it takes its duration. The runs after a running hook are simulated from its end. Hooks due at end are still planned.
 */
extern int indigo_scheduler_plan_with_hooks(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, scheduler_plan_run *runs, int max_runs);

/** The night plan of the VISIBILITY strategy, with the same inputs and runs as indigo_scheduler_plan_with_hooks(). It never plans fewer task runs than the COMPACT plan, starts from its runs, adds the runs COMPACT leaves out where they fit, and chooses their order and start times, on whole UTC minutes, so that the summed run quality weighted by the run duration is highest. Runs of urgent tasks keep their COMPACT start. The COMPACT plan is returned when no arrangement is better. previous (may be NULL) is the rest of the last VISIBILITY plan, with task indexes into tasks; it seeds the search and wins ties, which keeps the plan stable. flip (may be NULL) adds the cost of a meridian flip to every run that is not urgent. info (may be NULL) tells what was decided.
 */
extern int indigo_scheduler_plan_visibility(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, const scheduler_flip *flip, const scheduler_plan_run *previous, int previous_count, scheduler_plan_run *runs, int max_runs, scheduler_plan_info *info);

/** The best start times of the VISIBILITY plan for task runs in a fixed order, given as task indexes into tasks, without urgent runs; for tests. Stores the starts and returns the summed quality weighted by the run duration, or -1 when the order has no feasible arrangement.
 */
extern double indigo_scheduler_plan_order(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, const int *order, int order_count, time_t *starts);

#ifdef __cplusplus
}
#endif

#endif /* agent_scheduler_plan_h */
