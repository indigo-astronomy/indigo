# Design: wait for a better slot near culmination

Ticket: cc3c31d5-4e48-4dcf-9d9d-4a78450e946b, `indigo_agent_scheduler`.

## 1. Problem

The scheduler is greedy. At every decision it starts the best eligible task, and it idles only when no task is eligible (SCHEDULER.md, Selection, step 7). Ticket 85f0bbec made the altitude term of the score relative to each target's culmination. That helps when several tasks are eligible at once, but the score only ranks tasks against each other at one moment. It never compares "now" with "later tonight". A queue shorter than the night therefore still runs back to back from dusk.

Simulation with the plan code on master (`indigo_scheduler_plan`): site 48.15 N 17.11 E, 2026-11-19, `@twilight astronomical`, `@altitude > 30`, weights 1/1, one run each. *Quality* is the mean altitude over the run as a percentage of the transit altitude.

| Task | Greedy (master) | Quality | Look-ahead, gain 5 (recommended option) | Quality |
|---|---|---|---|---|
| M31 | 17:00–18:00 | 79 % | 18:20–19:20 | 94 % |
| M45 | 18:05–19:05 | 54 % | 21:10–22:10 | 95 % |
| M81 | 19:20–20:20 | 46 % | 02:20–03:20 | 93 % |
| M42 | 22:30–23:30 | 90 % | 23:00–00:00 | 95 % |
| Mean | | **67 %** | | **94 %** |

M45 runs 4 h 40 min before its 22:45 transit, and M81 runs near its lower culmination in the evening instead of its upper one before dawn. The equipment is idle for most of the night in both plans. With the greedy plan, all the idle time falls after 23:30.

## 2. Options

The scratch prototype, outside the repository, is `/tmp/claude-1000/-home-rumen-work-indigo-git/fecc735e-0b63-4bee-b2c9-d84127f538b6/scratchpad/altmap/proto.c`. It reimplements the plan loop without hooks and adds each option. All rows use the site and date above. *Runs* is the number of runs placed before sunrise.

| Scenario | Greedy | A: threshold 80 % | C: look-ahead, gain 5 | C: gain 10 |
|---|---|---|---|---|
| 4 tasks × 60 min (ticket case) | 4 runs, 67 % | 4, 98 % | 4, 94 % | 4, 89 % |
| 4 tasks × 30 min | 4, 64 % | 4, 98 % | 4, 95 % | 4, 90 % |
| 4 tasks × 120 min | 4, 79 % | 4, 94 % | 4, 93 % | 4, 89 % |
| 4 tasks × 60 min, `@repeat 3 every 60` | 10, 86 % | **8**, 89 % | 10, 86 % | 10, 86 % |
| 4 tasks × 60 min, `@repeat 5` | 11, 93 % | **9**, 91 % | 11, 93 % | 11, 93 % |
| 7 spread targets × 45 min | 7, 78 % | 7, 93 % | 7, 92 % | 7, 89 % |
| 7 spread targets × 90 min (full night) | 7, 95 % | 7, 95 % | 7, 95 % | 7, 95 % |
| 8 crowded targets (RA 3–5 h) × 60 min | 8, 80 % | **6**, 89 % | 8, 86 % | 8, 83 % |
| 8 crowded targets × 90 min | 7, 81 % | **4**, 89 % | 7, 81 % | 7, 82 % |

### A. A scheduler-wide threshold

**Behaviour.** A new setting, either a minimum relative altitude or a maximum number of hours from transit. A task below the threshold is not started while a slot above it exists later tonight.

**Trade-off.** It is excellent for short queues. In crowded or repeating queues it loses runs: 6 of 8, 4 of 7, 8 of 10 and 9 of 11. Every deferred task aims at the same few hours, and the night ends first. A good threshold depends on the queue length, which the user would have to tune every night.

**Complexity.** Low. Each candidate needs one scan for a later slot.

**Consistency.** The plan applies the same test, so the plan and selection agree. Nothing in it checks the cost of waiting. Adding a no-loss check would turn it into option C with a different trigger.

### B. A per-task header key

**Behaviour.** For example `@culmination <hours>`: prefer a start within ±hours of transit, and wait for such a slot when one exists tonight.

**Trade-off.** It behaves like A per task and loses runs in the same crowded cases. It needs a value in every task. The hard form already exists as `@ha`.

**Complexity.** A new key with parsing, validation and documentation. The plan logic is the same as A.

**Consistency.** The same as A. It could be added later on top of C as a per-task override; see Q5.

### C. Look-ahead that defers without losing a run (recommended)

**Behaviour.** For the best-scoring candidate, find its best slot later tonight. When that slot is clearly better and holding the task back loses no run that the plan places tonight, defer it. The next candidate is then considered. When every candidate is deferred, the scheduler waits.

**Trade-off.** Waiting happens only when the forecast says it costs nothing. In the simulations C never placed fewer runs than greedy and never had a lower mean quality. In short queues it improves the mean by 14–31 points. A full queue is left unchanged. Some quality is given up compared with A: a deferred task starts as soon as it is within the gain of its best slot (section 3.2).

**Complexity.** Medium. The plan simulation is split so that it can run from an intermediate state. A decision may need up to one baseline simulation plus one simulation per examined candidate. The prototype needed 0–12 nested simulations for a whole night. The cost is bounded in section 3.5.

**Consistency.** Selection and the plan call the same decision function in `indigo_agent_scheduler_plan.c`, so a waiting task appears in the night plan at the time it will actually start.

A variant that also shifts a whole block (all tasks idle Δ minutes, Δ up to 4 h) was tested. It gains in the crowded 60-min case (88 % instead of 86 %) but adds nothing in the ticket case (94 %, the same as C). It also costs about 120–200 nested simulations a night. It is not recommended now; see Q6.

## 3. Recommendation: option C

### 3.1 Setting

A new `AGENT_SCHEDULER_SETTINGS` item:

| Item | Unit | Default | Range | Label | Meaning |
|---|---|---|---|---|---|
| `DEFER_GAIN` | % | 0 | 0–100, step 1 | `Defer for altitude gain (%)` | 0 is off (today's behaviour). Otherwise a task without urgency is held back while a slot later tonight raises its mean relative altitude by at least this many percentage points, and holding it back loses no planned run. |

**Default 0.** This keeps today's behaviour. Turning deferral on creates idle periods in the middle of the night. During those periods the `IDLE` hook can park, and `RESUME` can unpark, which changes what existing setups do (section 3.4). Users who want deferral turn it on; the guide recommends 5. Whether the default should be 5 instead is Q1.

The item is saved with the other settings. An old configuration without it loads the default.

### 3.2 Decision rule (shared by selection and the plan)

Definitions for task X at time t:

- `q(X, s)` is the mean of `100 * max(0, alt) / alt_transit` over the grid points from `s` to `s + duration`, clamped to 0–100, using the existing target grid. A task without `@target` is never deferred.
- `s*` is the start time in [t, plan end) with the highest `q(X, s)` among the times where X is eligible for its whole duration (`indigo_scheduler_points_ok`, at grid steps). This includes `@window`, `@ha`, `@twilight` and the other constraints. Ties go to the earliest time.
- The release time `r` is the first eligible grid time in [t, s*] with `q(X, s*) − q(X, r) < DEFER_GAIN`. Holding X until `r` rather than `s*` makes the rule stable: re-evaluated 30 s later, the same `s*` and `r` come out. The runtime therefore needs no stored hold, and the forecast start equals the real start.

At a decision at time t, after urgent tasks and the estimate rule exactly as today:

1. Rank the eligible non-urgent tasks by score, as today.
2. For each candidate X in that order, at most `MAX_DEFER_CANDIDATES` = 8 of them:
   - If `DEFER_GAIN` is 0, or `s*` does not exist or is not later than t, or `q(X, s*) − q(X, t) < DEFER_GAIN`, select X.
   - Otherwise simulate the night twice from the current state at t, both times greedily (no deferral inside the nested simulations), with the same hooks:
     - **baseline:** the state as it is, plus the holds already decided at this step;
     - **held:** the same, with X not available before `r`.

     Defer X when both conditions hold:
     - no task has fewer planned runs in *held* than in *baseline*;
     - the sum of `q` over the planned runs in *held* exceeds that in *baseline*.

     The second condition stops one hold from pushing others to worse slots. Without it, the crowded case dropped from 80 % to 77 %. If X is not deferred, select X.
3. Any candidate beyond the eighth is selected without the look-ahead.
4. If every candidate is deferred, nothing starts.

**In the plan.** A deferred task gets `available_from = r` in the simulation. When nothing starts, the simulation advances to the earliest of the release times, the next eligibility and the next hook, as it already does for delays.

**At run time.** `select_task()` passes the eligible candidates to the same function. It builds the `scheduler_plan_task` array with the code that `update_plan()` uses today, factored into one helper. Its result either names the task to start or says that every candidate waits.

### 3.3 Interactions

- **Urgent tasks** are never deferred and are selected before the look-ahead, as today. A pending urgent task is part of both nested simulations, so it counts as a planned run. Preemption is unchanged.
- **Repeats and intervals.** The hold covers only the next run. Each run is decided again. The run counts include all repeats, so a task with many runs that fills the night is not deferred (`@repeat 5`: 11 runs in both plans).
- **Retries.** A task back from RETRY is a candidate like any other. The rule is the same.
- **`@window` ends** limit the search for `s*` through `points_ok`. An absolute end that a hold would cross shows up as a lost run, so the task is not deferred.
- **The night end.** `s*` is searched only up to the plan end (sunrise or the scheduler's night end), and a deferred task never moves into the next night.
- **`@ha`** restricts the eligible starts and therefore `s*`. `@ha` stays the hard window and `DEFER_GAIN` the soft preference.
- **`IDLE_PARK_AFTER`, the `IDLE` hook and standby.** Waiting for a release time is idle time, exactly like waiting for a constraint. The idle period starts at the first decision that starts nothing. After `IDLE_PARK_AFTER` minutes the `IDLE` hook runs, then standby, then `RESUME` before the deferred task. This keeps one rule. Suppressing the `IDLE` hook when the next planned run is near is Q2.
- **Night hooks.** `NIGHT_START` still runs when the plan has work in the night, even when the first run is deferred. `NIGHT_END` is unchanged. The nested simulations include the planned night hooks, so a hold cannot push a run past `NIGHT_END` unnoticed.
- **Automatic stop.** It is unchanged. A deferred task is QUEUED, not finished.
- **STOP and ABORT.** They are unchanged. While the scheduler waits nothing runs, so `STOP` runs the `SHUTDOWN` hook at once, as during any wait.
- **Estimates.** They are unchanged: no ordinary task starts while an estimate is pending, deferred or not.
- **Task state.** A deferred task stays QUEUED because it is eligible. A new DEFERRED state is Q3.

### 3.4 What the user sees

- **`AGENT_SCHEDULER_CURRENT.REASON`** while the scheduler waits only because of deferral: `<name> waits until HH:MM UTC, nearer its culmination`. It names the deferred task with the earliest release time; the time is that release time. When some tasks are deferred and others are not eligible, this reason takes precedence, because it names the next start. It is added to the Reasons table of the guide.
- **`AGENT_SCHEDULER_PLAN`**: a deferred run is listed at its planned start. Its reason column, `-` today for planned runs, shows `deferred from HH:MM`, where HH:MM is the time at which the run would have started without the hold. The gap before it is visible in the times.
- **Messages**: none per decision; they would repeat every check.
- **`AGENT_SCHEDULER_HISTORY`**: unchanged.

### 3.5 Cost

A nested simulation costs what one plan costs today. The existing performance test plans 256 tasks in well under 1 s. For each candidate whose gain reaches `DEFER_GAIN`, the look-ahead adds one held simulation, plus one baseline per decision. A hold persists inside the outer simulation, so a deferred task is examined once per hold, not at every step. With at most 8 candidates per decision, the worst case is 9 extra simulations per planned decision. The scan for `s*` uses running sums over the grid, which makes it linear in the grid points. The 256-task performance test gets a variant with `DEFER_GAIN` = 5. If it exceeds about 2 s on the reference machine, the candidate cap drops to 4 or the look-ahead ends at the first deferred candidate; see Q4.

## 4. Test plan

**Unit, `indigo_test/unit/test_agent_scheduler_plan.c`:**

- `DEFER_GAIN` 0 reproduces today's plan bit for bit, in the existing cases and in the ticket case.
- Ticket case (4 × 60 min) with gain 5: each run starts at its release time; mean quality ≥ 90 %; same number of runs.
- Release rule: the run starts at the first grid time within the gain of `s*`, not at `s*`; with gain 100 nothing is deferred.
- No-loss: crowded set (8 targets, RA 3–5 h, 60 and 90 min) and `@repeat 5` place as many runs per task as greedy.
- Quality guard: a case where a hold would lower the summed quality is not deferred.
- A task without `@target` is never deferred and fills the idle gap.
- Urgent tasks are never deferred and keep their order; a pending urgent task counts as a run in the comparison.
- An absolute `@window` end or `@ha` limit before `s*` caps the search; a hold that crosses a window end is refused.
- `@repeat n every m`: only the next run is held, and later runs are decided again.
- Hooks: with a planned `NIGHT_START`/`NIGHT_END`, the deferred plan keeps them and the no-loss rule sees their reservation.
- Consistency: for a sequence of times t, t + 30 s, …, up to the release, the decision function at time t returns the same choice as the first run of the plan computed at t.
- Performance: 256 random tasks with gain 5, under the time limit from section 3.5.

**Integration, `indigo_test/integration/test_agent_scheduler.c`.** The site longitude is moved, as the night tests do, so that a target transits about 2 h from now with constraints that already hold.

- Gain 0: the task starts at once, as today.
- Gain 5: the task does not start, `REASON` reads `… waits until HH:MM UTC, nearer its culmination`, the task stays QUEUED, and the plan lists it at the release time with `deferred from HH:MM`.
- Changing `DEFER_GAIN` back to 0 starts the task at the next decision.
- With `IDLE_PARK_AFTER` 0 and an `IDLE` hook, the idle hook runs while the scheduler waits, and `RESUME` runs before the deferred task when it is named. To keep the test short, the task's release is forced by a site move.
- An urgent submission during the wait starts at once.
- `STOP` during the wait runs `SHUTDOWN` and stops.
- A second task without `@target` runs during the wait, and the deferred task follows.
- The setting is saved and restored with the configuration.

Recorded runs follow the repository rules (`tools/run_driver_test.py agent_scheduler`).

## 5. Documentation changes

- **`indigo_drivers/agent_scheduler/doc/SCHEDULER.md`:**
  - the settings table: the new `DEFER_GAIN` row;
  - Selection: a new step between Score and Run with the rule from 3.2, and the note in the altitude bullet ("It adds no waiting …") rewritten to point to it;
  - Night plan preview: the deferred run, its `deferred from` reason, and the statement that the plan applies the same deferral;
  - Hooks: the `IDLE` paragraph, noting that waiting for a deferred task is idle;
  - the survey row "Selection" (score with optional look-ahead);
  - Testing;
  - the revision date.
- **`indigo_docs/INDIGO_SCHEDULER_GUIDE.md`:**
  - Priorities: the altitude bullet's "never idles" sentence;
  - Settings: the row and a short example of a short queue with gain 5;
  - Reasons: the new reason;
  - the night plan section: `deferred from`;
  - the `IDLE` hook rows: deferral waits count as idle.
- **`indigo_docs/PROPERTIES.md`:** add `DEFER_GAIN` to the item list and the description of `AGENT_SCHEDULER_SETTINGS`.
- **`indigo_names.h`:** `AGENT_SCHEDULER_SETTINGS_DEFER_GAIN_ITEM_NAME`.
- **Agent version:** bump the agent version.
- **`CHANGELOG.md`:** not changed until a release.

## 6. Open questions for the owner

1. **Default.** Should `DEFER_GAIN` default to 0 (off, today's behaviour, recommended) or to 5 (on for everyone, which creates mid-night idle periods with the `IDLE`/`RESUME` hooks)?
2. **`IDLE` hook during a deferral wait.** Should it run as for any wait (recommended, one rule), or be skipped when the plan's next run starts soon after the hook would run (for example within another `IDLE_PARK_AFTER` minutes), to avoid a park and unpark shortly before the deferred run?
3. **Task state.** Should a deferred task stay QUEUED with the reason in `CURRENT` and the plan (recommended), or get a new visible state `DEFERRED`? A new state changes the task-state labels and the documented state list.
4. **Cost limit.** Is a cap of 8 examined candidates per decision acceptable, with a fallback to 4 if the 256-task test is too slow on a Raspberry Pi?
5. **Per-task control.** Is the scheduler-wide setting enough for now, or should a header key such as `@defer no` / `@defer <gain>` (a per-task override of `DEFER_GAIN`) be added in this ticket?
6. **Block shift.** Should the variant that also delays the whole queue (better for crowded queues, about 20× the cost) be left for a later ticket, as recommended?
7. **Quality measure.** Is the mean relative altitude over the run the right quality for deferral? The score term uses the altitude at the start only, and changing that is out of scope here.
