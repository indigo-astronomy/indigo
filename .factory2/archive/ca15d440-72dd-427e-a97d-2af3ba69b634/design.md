# Design: COMPACT / VISIBILITY switch for the scheduler (replaces DEFER)

Ticket: ca15d440-72dd-427e-a97d-2af3ba69b634 — `indigo_agent_scheduler`.
Status: design for owner approval. No project code or documentation is changed by this stage.

## 0. Summary

- The 0–100 `AGENT_SCHEDULER_SETTINGS.DEFER` item and the whole deferral/hold machinery are removed.
- A new switch property `AGENT_SCHEDULER_STRATEGY` selects one of two strategies:
  - `COMPACT` (default): the greedy selection and the greedy night plan, exactly the behaviour before DEFER.
  - `VISIBILITY`: the scheduler computes a night plan that keeps at least every run the COMPACT plan has, and
    chooses the order and start times of the runs so that the summed run quality, weighted by run duration, is
    as high as possible. It then starts the plan's first run at its planned time and plans again after every run.
- The VISIBILITY plan is computed by an exact dynamic programme (DP) of start times on a 1-minute grid for a
  fixed run order, including exact repeat spacing between non-adjacent runs of the same task, inside a local
  search over the order. The search is bounded by a deterministic work budget. The COMPACT plan is always a
  candidate, so VISIBILITY never plans fewer runs than COMPACT.
- History: DEFER was added on 2026-11-19 (d514bd0e4, 1414a26c6) and never released; the scheduler agent
  itself is not yet in any `CHANGELOG.md` release block (latest release 3.0-11, cd4267d46). No compatibility
  handling is needed: a stale `DEFER` item in a saved configuration is ignored by
  `indigo_property_copy_values()`.

Prototype evidence used below was produced in the session scratchpad
(`/tmp/claude-1000/-home-rumen-work-indigo-git/fecc735e-0b63-4bee-b2c9-d84127f538b6/scratchpad/defer/`):
`v.c`/`v.h` (owner's prototype), `vx.h` (exact repeat spacing, written for this design), `vtx.c`, `vtb.c`,
`vtt.c` (harnesses with timing and checks). Numbers are from an Intel i5-9400; nothing was measured on a
Raspberry Pi.

## 1. Property and API

### 1.1 New property `AGENT_SCHEDULER_STRATEGY`

| Field | Value |
| --- | --- |
| Name | `AGENT_SCHEDULER_STRATEGY` (`AGENT_SCHEDULER_STRATEGY_PROPERTY_NAME`) |
| Type / rule / perm | switch, `INDIGO_ONE_OF_MANY_RULE`, `INDIGO_RW_PERM` |
| Group | `SCHEDULER_GROUP`, like `AGENT_SCHEDULER_NIGHT` |
| Label | `Scheduling strategy` |
| Items | `COMPACT` — "Compact (start tasks as soon as they can run)", default `true`; `VISIBILITY` — "Best visibility (plan runs nearer their culmination)" |
| Item name defines | `AGENT_SCHEDULER_STRATEGY_COMPACT_ITEM_NAME` `"COMPACT"`, `AGENT_SCHEDULER_STRATEGY_VISIBILITY_ITEM_NAME` `"VISIBILITY"` in `indigo_libs/indigo/indigo_names.h`, next to the `AGENT_SCHEDULER_NIGHT_*` defines (indigo_names.h:1553-1556) |
| Persistence | saved with the configuration and restored by the config reader, exactly like `AGENT_SCHEDULER_NIGHT` |
| Change effect | `PRIVATE_DATA->dirty = PRIVATE_DATA->plan_dirty = true`; the plan and the next decision follow the new strategy at once. Changing it while a task runs does not stop the run. |

Implementation follows the `NIGHT_PROPERTY` pattern in `indigo_agent_scheduler.c` at every place it
appears: item macros (135-138), private pointer (283), `save_config()` (822), the change branch (2912-2919),
init (3076-3083), define (3145), `define_copy` (3187), the property list for `enumerate`/attach (3203) and
release (3266).

### 1.2 Removed

- `AGENT_SCHEDULER_SETTINGS_DEFER_ITEM_NAME` (indigo_names.h:1543).
- `SETTINGS_DEFER_ITEM` (indigo_agent_scheduler.c:121) and its init (3049); `AGENT_SCHEDULER_SETTINGS` goes
  back from 11 to 10 items (3035).
- In the agent: `defer_threshold()` (1560-1567), `select_deferred()` (384, 2419-2491), the private fields
  `defer_wait_until`, `defer_start_slot`, `defer_reason` (340-343), the deferral special cases of
  `refresh_sky()` (1541-1552), the `waiting` condition of `process()` (2522-2524), the reset in
  `update_plan()` (2293), the DEFER branch of `select_task()` (1667-1674) and the `deferred from` text of
  `plan_run_label()` (2238-2244).
- In the plan module: `SCHEDULER_MAX_DEFER_CANDIDATES` (plan.h:41), `scheduler_plan_run.deferred_from`
  (plan.h:74), `plan_context.defer_gain` (plan.c:46), `plan_state.deferred_from` (plan.c:59-60),
  `plan_outcome` (plan.c:63-67, only used by `decide()`), `defer_release()` (116-164), `release_holds()`
  (253-260), the `defer` parameter and the ranking of `simulate()` (263-397, lines 292-345), `decide()`
  (399-443), `indigo_scheduler_plan_with_deferral()` (453-479) and `indigo_scheduler_plan_decide()`
  (481-500).
- Kept, because they are result-preserving speed-ups or useful independently: the `ineligible_from/until`
  cache of `simulate()`, `next_good` and `next_bad_step` in `scheduler_task_points`,
  `indigo_scheduler_nights_changed()`, the coarse Moon grid of d3eb7f6bb.

After the removal `simulate()` is the pre-DEFER greedy loop again (compare `git show
d514bd0e4^:indigo_drivers/agent_scheduler/indigo_agent_scheduler_plan.c`), and `select_task()` without the
DEFER branch is textually the pre-DEFER `select_task()` (verified by diff: the only differences are the
`candidates` array and the DEFER branch).

### 1.3 Plan module API

```c
/** The night plan of the VISIBILITY strategy. previous (may be NULL) is the rest of the last VISIBILITY plan,
 with task indexes into tasks; it seeds the search and wins ties, which keeps the plan stable. */
extern int indigo_scheduler_plan_visibility(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, const scheduler_plan_run *previous, int previous_count, scheduler_plan_run *runs, int max_runs, scheduler_plan_info *info);
```

`scheduler_plan_info` (output, may be NULL) reports: the COMPACT run count and objective, the chosen run count
and objective, whether the COMPACT plan was kept and why (`no feasible arrangement`, `not better`, `too many
runs`), and the work used. It serves the unit tests, the benchmark and a debug log line; it is not published.

`indigo_scheduler_plan()` and `indigo_scheduler_plan_with_hooks()` stay and are the COMPACT plan.
`indigo_scheduler_run_quality()` stays (it is the quality used by the objective).

### 1.4 Effects outside the agent

- `indigo_docs/PROPERTIES.md`: remove `DEFER` and its text from the `AGENT_SCHEDULER_SETTINGS` row (700); add
  an `AGENT_SCHEDULER_STRATEGY` row next to `AGENT_SCHEDULER_NIGHT` (704); keep the per-section source note.
- `tools/scheduler_sandbox.py`: add the verb `strategy compact|visibility` (pattern of `limit`, lines
  442-447) and a line in the help text (around line 80); `status` prints the strategy. The sandbox has no
  DEFER use today.
- Clients: no INDIGO client in this repository uses DEFER (checked with `git grep`). Generic clients see a new
  switch property; external clients need nothing because the property is self-describing.
- Xcode project: no new file unless the benchmark of section 4.3 is accepted (then the new `.c` file is added
  to the test group of `indigo.xcodeproj`).

## 2. The VISIBILITY plan

### 2.1 Terms

- A *run* is one execution of a task (a task with `@repeat 3` has up to three runs). Each run has a task, a
  duration `d` (the task's duration estimate, as in COMPACT) and a start `s`.
- *Run quality* `q(task, s)` is `indigo_scheduler_run_quality()` (plan.c:99-114): the mean altitude over the run
  in percent of the target's transit altitude, each grid point clamped to 0–100; a task without a usable
  target has quality 0 for the objective.
- *Whole-run eligibility* `ok(task, s)` is `indigo_scheduler_points_ok(sky, points, header, s, s + d)`, the same
  test COMPACT uses: absolute and nightly `@window`, `@ha`, `@twilight`, `@altitude`, Moon constraints, sky
  coverage. It covers every constraint over the whole run.
- The *plan horizon* is `[t0, end)`: `t0` is now, or the end of the running task or running hook (as computed
  by `init_state()`, plan.c:191-249), rounded up to the next whole minute; `end` is the plan end computed in
  `update_plan()` (2319-2330). A run must start before `end`, as in COMPACT (`state->t < ctx->end`).

### 2.2 Objective

Maximise, over the planned runs that are not running,

```
    J = sum over runs of q(task, s) × d / 3600        (unit: percent × hours)
```

subject to the constraints of 2.3, with this precedence between candidate plans:

1. more task runs is better (never fewer than the COMPACT plan, see 2.5);
2. then higher `J`;
3. on a tie (difference ≤ 1e-6), the earlier candidate in the seed order of 2.4 wins (the previous plan first,
   which keeps the plan stable).

Weighting by duration (recommended, see owner question Q3) measures hours of data at good altitude. Evidence
(1000 random nights, section 4.3): weighted 5814 runs, time-weighted quality 80.7 %, and on **no** night with
the same run count a lower time-weighted quality than COMPACT; unweighted 5818 runs, 80.5 %, and 10 such nights
worse than COMPACT, because a 20-minute run then counts as much as a 2-hour one.

### 2.3 Constraints

A candidate is a run order `r_0 … r_{R-1}` with starts `s_0 … s_{R-1}` on the minute grid. It is feasible when:

1. **Whole-run eligibility:** `ok(task_k, s_k)` and `t0 ≤ s_k < end` for every run.
2. **No overlap, hooks delayed by runs, not blocking:** for consecutive runs `p = k-1`, `k`
   (and for `k = 0` a virtual predecessor ending at `t0`): let `x = s_p + d_p`; for each not-yet-run hook `h`
   of `plan_hooks` in time order with `s_p < H_h ≤ s_k`: `x = max(x, H_h) + dur_h`. The transition is valid
   when `s_k ≥ x`. This is exactly the hook rule of `simulate()` (plan.c:274-291: a hook due at or before a
   decision runs first, after the run in progress). Hooks due after the last run do not constrain runs; the
   final hook pass of the plan (plan.c:463-476) places them as now.
3. **Delays:** the first planned run of a task starts at or after its `available_from` (repeat or retry
   `delay_until`, and after the running run of the same task: plan.c:241-245). The prototype `v.h` did not
   apply this bound.
4. **Repeat spacing, exactly:** for two runs `a < b` of the same task with no run of that task between them,
   `s_b ≥ s_a + d_a + 60 × repeat_every`. See 2.6 for how the DP enforces this also when other runs lie
   between `a` and `b`.
5. **Urgent runs fixed:** every run of an urgent task keeps the start it has in the COMPACT plan (rounded up to
   the minute). In the DP its eligibility is restricted to that single minute and the local search never moves
   it. At run time urgent tasks are selected by `select_urgent()` and preempt as now (1764-1771), so pinning
   them at their COMPACT time is the honest forecast in both strategies.
6. **Running task and running hook:** not part of the optimisation; they come first in the plan as now
   (`init_state()`), and `t0` starts after them.
7. **Run set:** the candidate contains, per task, at least as many runs as the COMPACT plan (priority decides
   which runs are in the plan, VISIBILITY only rearranges them and adds runs COMPACT left out), and at most
   `runs_left` per task.

### 2.4 Algorithm

```
plan_visibility(now):
  C   = COMPACT plan (indigo_scheduler_plan_with_hooks), J(C) computed with the objective
  if C has more than VIS_MAX_RUNS task runs or the horizon exceeds 1440 min: return C (info: too many runs)
  build per task the tables ok[i][g], q[i][g] for g in the horizon (minute grid)
  seeds, in this order:
    S1 = the previous VISIBILITY plan's remaining runs (dropping runs whose task is gone, finished, disabled,
         or above runs_left), plus the COMPACT runs it lacks, inserted where they fit best
    S2 = the COMPACT order
    S3 = the COMPACT run set ordered by the start of each run's best-quality minute (ties: COMPACT order)
  for each seed:
    insert the runs COMPACT left out (tasks with runs_left above their count), one at a time, each at the
      position with the highest J, as long as some insertion is feasible
    local search: repeatedly move one run to another position (first improvement, positions scanned in
      ascending order, skip moves between runs of the same task and moves of pinned urgent runs), evaluate
      each order by the DP of 2.5; accept a move when the order is feasible and better by the precedence of
      2.2; stop when no move improves or the seed's share of the work budget is used
  V = best seed result by the precedence of 2.2
  if V is infeasible, or V has fewer task runs than C, or (same runs and J(V) ≤ J(C) + 1e-6): return C
  return V (followed by the hooks due at the end, as in plan.c:463-476)
```

The COMPACT fallback is part of the definition, not an error path: **VISIBILITY never plans fewer runs than
COMPACT, by construction**, and never a lower `J` than COMPACT for the same run count.

### 2.5 DP of start times for a fixed order

For runs `k = 0 … R-1` in the given order and minute index `g` in the horizon, `F_k(g)` is the best `J` of runs
`0 … k` with run `k` starting at minute `g`:

```
F_0(g) = v_0(g)                                   if run 0 may start at g (2.3 items 1, 3, 5 and hooks before it)
F_k(g) = v_k(g) + max { F_{k-1}(g') : transition g' -> g valid (2.3 item 2) }
v_k(g) = q(task_k, g) × d_k / 3600, or -infinity when not allowed
```

Durations enter the grid rounded up to whole minutes (a loss of under one minute per run against COMPACT's
second-exact times; the COMPACT fallback covers the rare case where this costs a run at the night end).

Without hooks the valid predecessors of `g` are `g' ≤ g - ceil(d_{k-1}/60)`, so the inner maximum is a running
prefix maximum and a layer costs O(G). With pending hooks the predecessor range splits at the hook times into
at most `hooks + 1` segments, each a prefix range (the bound `x` of 2.3 item 2 is non-decreasing in `g'`
within a segment), so a layer costs O(G × (hooks + 1)); `plan_hooks` holds at most 3 entries
(indigo_agent_scheduler.c:2167).

Backtracking stores the argmax predecessor per (layer, g). Ties: strictly better values only replace the
running maximum, so the earliest predecessor and, at the last layer, the earliest start win. The result is
deterministic.

### 2.6 Exact repeat spacing between non-adjacent runs

The prototype enforced spacing only between adjacent runs and iterated lower bounds that do not converge
(about 2 % of decisions fell back). It also missed the `available_from` bound of 2.3 item 3. Re-running it with
a check of the executed nights found **538 repeat-spacing violations** in 1000 nights. The design replaces
this with an exact formulation.

For a pair `a < b` of consecutive runs of the same task, write the gap after run `k` as
`gap_k = g_{k+1} - g_k - D_k` (minutes, `D_k = ceil(d_k/60)`; hook time inside a gap counts as gap, which is
correct because spacing is wall-clock time). Then

```
s_b - s_a = sum_{k=a}^{b-1} D_k + sum_{k=a}^{b-1} gap_k,
spacing   <=>  sum_{k=a}^{b-1} gap_k  >=  N_ab := D_a + repeat_every - sum_{k=a}^{b-1} D_k
```

- If `N_ab ≤ 0` the chain already implies the spacing: nothing to do (the common case when other runs lie
  between).
- If `b = a + 1` it is a local constraint: the transition into `b` requires `gap_a ≥ N_ab`.
- Otherwise the DP state of layers `a … b-1` is extended by the remaining required gap `r ∈ [0, N_ab]`:
  `r_a = N_ab`, `r_{k+1} = max(0, r_k - gap_k)`, and layer `b` accepts only `r = 0`. With
  `c = g - D_{k-1}`, the predecessors of `(g, r > 0)` lie on the anti-diagonal `g' + r' = c + r` with
  `r' ≥ r`, and those of `(g, 0)` are all `(g', r')` with `g' + r' ≤ c`. A suffix maximum along each
  anti-diagonal and a prefix maximum over the diagonals give every transition in O(1), so an extended layer
  costs O(G × (N + 1)).

Restrictions that keep the cost and memory bounded (orders that hit them are skipped by the search, never
planned with a violated constraint):

- at most one extended pair is open at any layer (two overlapping non-implied pairs of different tasks:
  the order is skipped);
- no pending hook is due inside an extended span (the hooks are the night start and end and lie at the edges
  of the night; such an order is skipped);
- `N_ab ≤ 120` minutes is handled exactly; above that the remaining gap is counted in units of
  `u = ceil(N_ab / 120)` minutes with the requirement rounded up and gaps rounded down — conservative (never
  violates spacing, may waste up to `u - 1` minutes per gap in that span), bounding the state to 121 values.

Verification in the prototype (`vx.h`, exact form without the unit scaling): 1000 random nights, **0
violations**, fallbacks to COMPACT 15 of about 5900 decisions (prototype 135), never fewer runs than COMPACT.
The unit tests of 4.1 check the DP against exhaustive enumeration on small cases.

### 2.7 Start-time resolution

Starts are whole UTC minutes (the sky grid start is a multiple of 300 s, so the minute grid is aligned to UTC
minutes). Quality changes only at the 5-minute sky grid, but window edges, repeat and retry delays and run ends
need finer starts; a 5-minute DP grid would leave up to 5 minutes idle between runs. Windows given in minutes
are exact on this grid; delays at arbitrary seconds are rounded up.

### 2.8 Bounds, budget and cost

| Bound | Value | Reason |
| --- | --- | --- |
| Horizon `G` | ≤ 1440 minutes (`[t0, end)`; longer: COMPACT for that plan) | `end` is the end of the coming night; restricting the DP to the horizon instead of the 48-hour grid made the prototype 4.6× faster with identical results |
| Runs optimised `VIS_MAX_RUNS` | 64 task runs (more: COMPACT plan, `info` says why, one message per night) | a night with more than 64 runs has runs under about 15 minutes, where waiting gains little |
| Extended DP state | ≤ 121 values per minute (2.6) | bounds memory: one extended layer ≤ 1440 × 121 states |
| States per DP | ≤ 2 000 000 (backpointers 4 bytes: ≤ 8 MB, allocated once and reused); larger orders are skipped | bounded memory on a Raspberry Pi |
| Work budget | 20 000 000 DP cell updates per plan, counted, not timed (deterministic); seeds S1, S2 get a third each of what remains, S3 the rest | see measurements |

Measurements (i5-9400, 1000 nights of 3-10 tasks, replanning after every run; prototype harness):

| Variant | Runs | Mean q per run | Time-weighted q | Worst plan | Runs moved > 15 min from the first plan of the night |
| --- | --- | --- | --- | --- | --- |
| COMPACT | 5595 | 68.8 % | 70.0 % | — | — |
| DEFER 100 (current) | 5673 | 77.4 % | 76.8 % | — | — |
| owner's prototype (v.h, weighted + previous plan) | 5839 | 80.9 % | 80.9 % | — | 808 (538 spacing violations) |
| exact spacing, no budget | 5815 | 80.7 % | 80.7 % | 194 ms | 78 |
| exact, budget 20 M | 5814 | 80.7 % | 80.7 % | 66 ms | 127 (49 nights) |
| exact, budget 5 M | 5812 | 80.7 % | 80.6 % | 23 ms | 497 (151 nights) |

The owner's prototype's higher numbers come partly from plans that violate repeat spacing. A larger stress
case (16 tasks of 10-60 minutes, prototype without budget) took 1.35 s per plan on the full 48-hour grid and
294 ms on the horizon only; the budget caps this case as well. Average work is about 1.8 M cells per plan
(about 6 ms on the i5).

Raspberry Pi (not measured): a Pi 4 is typically 4-6× slower single-threaded than an i5-9400 for this kind of
code, so the 20 M budget means an estimated 0.3-0.4 s worst case and about 30 ms on average per plan on a
Pi 4, about half of that on a Pi 5. Acceptance target proposed: worst case ≤ 0.5 s on a Pi 4 (owner
question Q6). The implementation adds the work count and the wall time to `info` and to a debug log line,
so it can be measured on a Pi without a special build.

Further cost reductions available to the implementer (recommended, not required): reuse the DP prefix up
to `min(a, b)` when evaluating a move from `a` to `b`; precompute the `ok`/`q` tables once per plan with
prefix sums of relative altitude (as `defer_release()` did, plan.c:128-133).

### 2.9 Determinism

No randomness, no wall-clock limits, fixed scan orders, fixed seed order and fixed tie rules: the same inputs
give the same plan on every platform (up to floating-point summation order, which is fixed by the code).

## 3. Runtime integration

### 3.1 Plan computation (`update_plan()`, 2288-2417)

- COMPACT: unchanged (pre-DEFER), the plan is computed when `plan_dirty` is set and at `plan_due`.
- VISIBILITY:
  - The first pass that decides whether the night has work for the night hooks (2340, 2353-2369) stays a
    COMPACT pass in both strategies (cheap, and the run set is the COMPACT one). The second pass, with the
    night hooks, calls `indigo_scheduler_plan_visibility()`. If the VISIBILITY plan has a run inside the night
    but the night hooks were not added (only possible when a run COMPACT places before the night start is
    moved into it), the night hooks are added and the plan is computed once more, seeded with the result.
  - The previous plan is kept in the private data as `(slot, run number, start)` per task run (task indexes
    of `plan_tasks` change between plans; slots do not) and mapped to indexes for the seed.
  - When `plan_dirty` is not set, a periodic refresh (`plan_due`) **keeps** the previous plan as long as it is
    valid: its first planned task run starts at or after `max(now, end of the running run or hook)`. Otherwise
    it is recomputed (seeded). The hourly sky and night rebuild that does not move the site
    (`indigo_scheduler_nights_changed()`) is not a change. This replaces the DEFER wait logic and is cheap: a
    long wait costs one plan, not one per check.
- `plan_first_run` (2341-2347) is taken from the plan of the active strategy, so `process_night()` (1931)
  starts `NIGHT_START` when the VISIBILITY plan has work in the night even if its first run is later.

### 3.2 Decision (`select_task()`, 1622-1690)

Unchanged up to the choice of `best`: estimates, eligibility, `TASK_WAITING`/`TASK_QUEUED`, urgent tasks
first. Then, with VISIBILITY and a non-urgent `best`, `select_planned()` replaces `select_deferred()`:

1. If `plan_dirty`, compute the plan now.
2. Take the plan's first task run that is not running: task slot `X`, start `S`.
3. `now < S`: nothing starts. `REASON` is `"<X> starts at HH:MM UTC, planned for better visibility"`
   (wording: Q9). The next idle check is scheduled no later than `S` (`next_check` = min(current, monotonic
   time of `S`)), so the run starts on time and not up to `IDLE_CHECK_INTERVAL` late.
4. `now ≥ S` and `X` is an eligible candidate: start `X`.
5. `now ≥ S` and `X` cannot start (a constraint changed, the start was missed while a manual sequence
   occupied the Scripting Agent, an estimate arrived late): set `plan_dirty`, compute the plan once more and
   repeat 2-4 once; if the new plan's first run still cannot start now, start the COMPACT `best` for this
   decision. This guarantees progress and no loop.
6. No planned task run but eligible candidates exist (the plan horizon ends, for example after the night
   end hook): start the COMPACT `best`.

When nothing is eligible, the existing reasons stay (`first_reason`, `no task is ready`, …). The VISIBILITY
reason takes precedence over the reasons of tasks that are not eligible, because it names the next start
(as the DEFER reason did).

### 3.3 Events

| Event | Behaviour |
| --- | --- |
| A run ends (success, failure, abort) | `finish_task()` (1368) and `finish_stop()` (1382, 1402) set `plan_dirty`; the next decision plans again from now, seeded with the previous plan, so the remaining order is kept unless a strictly better one is found. Repeats and retries enter as `available_from` (`delay_until`). |
| Run shorter or longer than estimated | as above; while a run overruns, the periodic refresh finds the kept plan invalid (first start before the new expected end) and recomputes. |
| Urgent task added or becoming eligible | `select_urgent()` starts it at once, also during a planned wait, and preempts a running non-urgent task as now (1764-1771); the plan is recomputed afterwards. |
| Task added, edited, deleted, enabled, disabled, reset; window expiry; settings, site, night limit, strategy changed | `plan_dirty` (existing call sites) → full recomputation seeded with the remaining previous plan. |
| Waiting for the plan's first run | Waiting is idle time, but the `IDLE` hook does not start when the plan's next run starts within `IDLE_PARK_AFTER` minutes (Q5, owner's choice b). When the moment for `IDLE` comes (after `IDLE_PARK_AFTER` minutes idle) and the next planned run is still more than `IDLE_PARK_AFTER` minutes away, `IDLE` runs and `RESUME` runs before the planned run, as for any idle period. |
| `STOP` / `ABORT` / automatic stop | unchanged; during a wait nothing runs, so `STOP` runs `SHUTDOWN` at once. A planned task is `QUEUED`, not finished, so it keeps the scheduler running. |
| Strategy switched | `plan_dirty`; COMPACT starts the best eligible task at once; VISIBILITY plans and may wait. |

### 3.4 Stability and minimum improvement

A new plan replaces the previous one only when it is strictly better by the precedence of 2.2; on ties the
previous plan wins because it is the first seed. A minimum-improvement threshold was tested: with budget 5 M,
thresholds of 0, 1 and 5 (percent × hours) left 497, 497 and 491 runs moved by more than 15 minutes; with
budget 20 M thresholds of 20 and 50 gave 109 and 101 against 127. What moves runs is mainly an incomplete
first search (the budget) and, in the prototype, plans that broke repeat spacing and had to be corrected
later (808 moved runs). **Recommendation: no threshold**; keep the tie rule and the budget of 20 M (Q4).

### 3.5 PLAN labels and REASON

- `AGENT_SCHEDULER_PLAN`: the reason column of a planned run is `-` in both strategies; `deferred from HH:MM`
  is removed. Runs are listed at their planned starts, so the gaps of a VISIBILITY plan are visible as idle
  time. Showing the expected quality in the reason column would be new behaviour (Q11).
- `AGENT_SCHEDULER_CURRENT.REASON`: the two DEFER texts (guide 921-922) are replaced by the one of 3.2 step 3.

## 4. Tests

### 4.1 Unit tests (`indigo_test/unit/test_agent_scheduler_plan.c`)

Remove `T_DEFER_NOON`/`DEFER_GAIN` uses that test DEFER semantics and the `test_defer_*` cases
(687-1157+, registrations 1270-1283); keep `test_run_quality`. Add (the 2026-11-19 date constant can stay,
renamed):

1. `compact_is_greedy`: the COMPACT plan equals the greedy simulation run for run (moved from
   `test_defer_gain_zero_unchanged`, with hooks and 128 random tasks), and the pre-existing greedy tests
   (96-509) pass unchanged.
2. `visibility_owner_night`: regression with the owner's tasks of 2026-11-19 at latitude 42, longitude 23
   (Orion with `@priority high` 31 min, Rosette 21, Jellyfish 121, Pleiades 21, Elephant 121, Soul 28; `@altitude` and
   `@twilight astronomical` as in the report): 6 runs; every run at least 90 %; the Jellyfish run within
   30 minutes of its transit centring; mean quality at least 95 % (prototype: 97.3 %, Pleiades 22:10 99.9 %,
   Orion 23:20 98.4 %, Jellyfish 23:51-01:52 98.0 %, Rosette 01:52 96.4 %, Soul 21:15 99.9 %, Elephant 16:45
   91.2 %; COMPACT 73.1 %, DEFER 100 93.0 %).
3. `visibility_never_fewer_runs`: 200 seeded random queues (3-10 tasks, `@repeat 2 every 60` on a quarter,
   high priority on a fifth, durations 10-150 min) and 50 crowded ones: per task at least the COMPACT run count,
   never a lower `J` than COMPACT at the same run count.
4. `visibility_constraints_hold`: for the same queues, every planned run passes `indigo_scheduler_points_ok()`
   over its whole run, starts before the end, does not overlap, respects `available_from`, and every pair of
   runs of one task respects `repeat_every`, adjacent and non-adjacent.
5. `visibility_dp_exact`: on small synthetic cases (2-4 runs, 90-minute horizon, a repeated task with runs
   apart), the DP value equals exhaustive enumeration of all start minutes; one case where the spacing is
   implied, one adjacent, one non-adjacent extended, one with a hook inside the gap.
6. `visibility_urgent`: urgent runs keep their COMPACT starts; non-urgent runs are arranged around them.
7. `visibility_windows`: an absolute `@window`, a nightly window and `@ha` limit the starts; a run never
   leaves its window.
8. `visibility_hooks`: night start and end hooks and a running hook: runs delay hooks, hooks shift the next
   run by their duration, hooks are not blocked.
9. `visibility_running_task`: the running task and its repeat delay come first, as in COMPACT.
10. `visibility_untargeted`: a task without `@target` fills a gap and is placed at its earliest feasible start.
11. `visibility_stable`: replanning at the end of every planned run with nothing changed keeps the order and
    starts of the remaining runs (on 60 seeded queues).
12. `visibility_deterministic`: two computations with the same inputs give identical plans; the previous plan
    seed wins ties.
13. `visibility_limits`: more than `VIS_MAX_RUNS` runs gives the COMPACT plan with `info` reason; an order with
    two overlapping extended pairs is skipped, never planned with a violation.
14. `visibility_performance`: the worst of 128 tasks of 5 minutes, 64 runs, with and without windows, and the
    random queues stay within the work budget and within a wall time of 2 s on the build machine (as the
    existing performance tests).

### 4.2 Integration tests (`indigo_test/integration/test_agent_scheduler.c`)

Replace the DEFER cases (2858-3040, registrations 4197-4203) and the DEFER part of the settings-persistence
case (1249, 1271):

1. `strategy_defaults_to_compact`: the property exists with `COMPACT` on; a target two hours before its
   culmination starts at once.
2. `visibility_waits_for_the_plan`: with `VISIBILITY` the same task stays `QUEUED`, `REASON` names it and its
   planned start, the plan lists it at that start, and the plan does not change when the clock passes the next
   minute.
3. `visibility_starts_at_the_planned_time`: moving the site so the planned start is now (or a plan start a
   few seconds ahead) starts the task within a second of the planned time.
4. `visibility_wait_idle_hook`: a wait shorter than `IDLE_PARK_AFTER` does not start `IDLE`; a wait whose next
   planned run is more than `IDLE_PARK_AFTER` minutes away when the idle time is reached runs `IDLE`, and
   `RESUME` runs before the task.
5. `visibility_wait_lets_urgent_and_untargeted_tasks_run`: an urgent task starts at once during a wait; an
   untargeted task fills the gap if the plan puts it there.
6. `visibility_wait_stops_with_the_shutdown_hook`.
7. `strategy_switch_back_starts_at_once`: switching to `COMPACT` during the wait starts the task.
8. `strategy_persists`: the strategy survives a restart (in `data_settings_persist`, replacing DEFER).
9. `visibility_replans_after_failure`: a failing run with retry is planned again after its delay and the
   remaining runs keep their order.

Sandbox: `tools/scheduler_sandbox.py` gets the `strategy` verb; `tools/scheduler_sandbox_night.txt` may set
`strategy visibility` once to show a waiting plan (optional).

### 4.3 Offline randomized comparison (evidence)

A deterministic comparison of COMPACT and VISIBILITY over N seeded random nights, executing each night with
replanning after every run, and reporting runs, mean and time-weighted quality, nights with fewer runs than
COMPACT (must be 0), spacing violations (must be 0), plan changes, worst work and wall time per plan. Proposal:
`indigo_test/benchmark/bench_scheduler_plan.c` under the existing `make -C indigo_test benchmark` target
(Makefile:425), not part of `test-unit` (Q10). The 1000-night result of the final implementation goes into a
ticket note as the evidence; a Raspberry Pi run is added when a Pi is available.

## 5. Documentation

Version: `DRIVER_VERSION` 0x0300001C → 0x0300001D (indigo_agent_scheduler.c:29).

### 5.1 `indigo_drivers/agent_scheduler/doc/SCHEDULER.md` (design document)

- Replace the `Deferral` section (635-705) by a `Strategies` section: the property; COMPACT (the greedy rules,
  unchanged); VISIBILITY with the objective, the constraints of 2.3, the algorithm of 2.4, the DP and the
  exact repeat spacing of 2.5-2.6, the resolution, bounds, budget and the COMPACT fallback guarantee, the
  determinism and the measured costs (with the caveat that the Pi numbers are estimates until measured).
- Selection (615-633): rule 7 becomes "Strategy" (COMPACT starts the best task; VISIBILITY starts the plan's
  first run at its planned time, 3.2); rules 4 and 9 lose "deferred".
- Night plan (715-745): the simulation is COMPACT; VISIBILITY plans as above; the plan is kept while valid
  (3.1); the reason column has no `deferred from`.
- Properties table (359, 384): remove DEFER, add `AGENT_SCHEDULER_STRATEGY`; idle hook (531) and night hooks.
- Tests (896, 930): describe the new unit and integration tests and the benchmark.

### 5.2 `indigo_docs/INDIGO_SCHEDULER_GUIDE.md` (user guide)

- "How the next task is chosen" (238-277): step 4 explains both strategies in plain words: COMPACT fills the
  night from dusk and never leaves the equipment idle while a task can run; VISIBILITY first plans the whole
  night, keeps every run COMPACT would make, and moves runs to the times where their targets stand highest,
  leaving gaps where waiting gains more than it costs. Replace the "Waiting for a better slot: DEFER" section
  by "Choosing a strategy" with:
  - what the quality of a run is (mean altitude in percent of the culmination altitude), and that VISIBILITY
    maximises it weighted by the run length;
  - the worked example of the owner's night of 2026-11-19 (lat 42°, lon 23°), as a table with each task's
    transit, the COMPACT run and quality, and the VISIBILITY run and quality (numbers from the final
    implementation; prototype values in 4.1 item 2), and the night's mean;
  - when to use which: COMPACT for a full queue, uncertain weather (data early in the night) or many short
    tasks; VISIBILITY for a short queue that does not fill the night;
  - what never waits (urgent tasks), how repeats, windows, `@ha` and hooks are respected, that the plan is
    recomputed after every run and on every change, and that waiting is idle time (`IDLE_PARK_AFTER`, the
    `IDLE` and `RESUME` hooks);
  - how to read `REASON` and the plan during a wait.
- Priorities (216): drop the DEFER reference, point to "Choosing a strategy".
- Settings (735, 741-744): remove DEFER and its `indigo_prop_tool` example; add the property with an example
  `indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_STRATEGY.VISIBILITY=ON"`.
- Hooks table (590), `REASON` (779, 921-922), plan example and reason column (838, 846), troubleshooting
  (1047-1048): replace DEFER by VISIBILITY.
- Sandbox (1009-1037): mention `strategy`.

## 6. Questions for the owner

- **Q1 Names.** `AGENT_SCHEDULER_STRATEGY` with items `COMPACT` and `VISIBILITY` (labels in 1.1)? Alternatives:
  `AGENT_SCHEDULER_MODE`; items `ASAP` / `BEST_ALTITUDE`.
- **Q2 COMPACT identity.** COMPACT will be the current greedy code without DEFER: its selection is textually
  the pre-DEFER `select_task()` and its plan is the pre-DEFER greedy loop plus the result-preserving
  `ineligible` cache. The coarse Moon grid of d3eb7f6bb is kept, so Moon-limited tasks can differ slightly
  from a pre-DEFER build. Is "same algorithm as before DEFER, on today's sky grid" enough, or must the plan be
  byte-identical to a pre-DEFER build (that would require reverting d3eb7f6bb's Moon positions)?
- **Q3 Weighting.** Weight run quality by run duration (recommended; 2.2), or count every run equally?
- **Q4 Minimum improvement.** Recommended: no threshold, ties keep the previous plan (3.4). Agree?
- **Q5 Idle hook during a planned wait.** (a) as with DEFER: the wait is idle time, `IDLE` runs after
  `IDLE_PARK_AFTER` and `RESUME` before the run (recommended, consistent); or (b) do not start `IDLE` when the
  next planned run starts within `IDLE_PARK_AFTER` minutes.
- **Q6 Cost target.** Work budget 20 M cells per plan, estimated ≤ 0.4 s worst case on a Pi 4 (not measured).
  Is ≤ 0.5 s worst case on a Pi 4 acceptable, and is a measurement on a real Pi required before closing (is a
  Pi available)?
- **Q7 Size limit.** Above 64 task runs in a night, or a horizon above 24 h, VISIBILITY uses the COMPACT plan
  with one message per night. Acceptable?
- **Q8 Urgent tasks.** Urgent runs keep their COMPACT start times in a VISIBILITY plan (2.3 item 5). Agree?
- **Q9 REASON text.** `"<task> starts at HH:MM UTC, planned for better visibility"`?
- **Q10 Evidence tool.** Commit the randomized comparison as `indigo_test/benchmark/bench_scheduler_plan.c`
  (Makefile and Xcode project entries), or keep it out of the repository and record only its results in the
  ticket?
- **Q11 Plan column.** Keep `-` as the reason of planned runs (recommended, no new behaviour), or show the
  expected quality, for example `altitude 98 %` (a scope extension)?
- **Q12 Priority.** Priority classes decide only which runs are in the plan; within the plan a high-priority
  run may be planned later than a normal one if that raises `J`. Agree, or should a higher class never wait
  for a lower one?
- **Q13 Guide example.** Replace the M31/M45/M81/M42 example of the guide with the owner's 2026-11-19 night?

## 7. Owner decisions (2026-11-19)

- Q1 Names: `AGENT_SCHEDULER_STRATEGY`, `COMPACT`, `VISIBILITY` as proposed.
- Q2 COMPACT: the pre-DEFER algorithm on today's sky grid; d3eb7f6bb is kept.
- Q3 Run quality weighted by run duration.
- Q4 No minimum-improvement threshold; ties keep the previous plan.
- Q5 Option (b): `IDLE` does not start when the next planned run starts within `IDLE_PARK_AFTER` minutes
  (3.3 and test 4.2-4 updated).
- Q6 Accepted: work budget 20 M cells, about 0.5 s worst case on a Pi 4; a real-Pi measurement is not
  required before closing.
- Q7 COMPACT plan above 64 runs or a 24 h horizon, with one message per night.
- Q8 Urgent runs keep their COMPACT start times.
- Q9 REASON text as proposed.
- Q10 Commit the randomized comparison as `indigo_test/benchmark/bench_scheduler_plan.c` under the
  `make -C indigo_test benchmark` target, with Makefile and Xcode project entries (owner, 2026-11-19).
- Q11 `-` stays in the reason column.
- Q12 Agreed for VISIBILITY only: there priority decides which runs are in the plan, and a high-priority run
  may be planned after a normal one. COMPACT keeps its current priority behaviour unchanged.
- Q13 The owner's 2026-11-19 night replaces the M31/M45/M81/M42 example in the guide.
