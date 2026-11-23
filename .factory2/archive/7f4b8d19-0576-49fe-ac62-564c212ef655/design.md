# Design: meridian flip penalty for the best visibility plan

Ticket 7f4b8d19-0576-49fe-ac62-564c212ef655, `indigo_agent_scheduler`.

## 1. Goal and scope

The `VISIBILITY` strategy places runs where their targets stand highest. That is usually around
culmination, so many runs cross the meridian, and a German equatorial mount then has to flip. This
change lets `VISIBILITY` avoid a flip when avoiding it costs little observing quality. Short tasks
benefit most.

In scope:

- the Scheduler Agent reads the meridian flip settings that `Sequencer.js` publishes;
- a flip cost in the `VISIBILITY` objective;
- one new setting;
- the night plan marks the runs that will flip;
- tests, the benchmark and the documentation.

Unchanged:

- `COMPACT` selection and the `COMPACT` plan;
- `SCHEDULER_ESTIMATE` and `Sequence.estimate()`;
- task durations;
- `Sequencer.js`, the Imager Agent and the Mount Agent.

## 2. Verified behaviour of a real flip

The ticket's claims were checked against the current sources:

- **Pause.** `indigo_drivers/agent_imager/indigo_agent_imager.c:1347-1360`: when `PAUSE_AFTER_TRANSIT` is enabled and a Mount Agent is related, the batch pauses before an exposure when `time_to_transit <= exposure/3600 - AGENT_IMAGER_BATCH_PAUSE_AFTER_TRANSIT`. `time_to_transit` is −HA east of the meridian, so the pause comes before the first exposure that would end at HA ≥ OFFSET. `pauseOnTTT = false` makes it happen once per batch.
- **Flip trigger.** `indigo_drivers/agent_mount/indigo_agent_mount.c:1144-1152`: `FLIP_REQUIRED = 1` when `TIME_TO_TRANSIT <= 0`, that is HA ≥ 0. While a slew is busy past transit, `show_negative_time_past_transit` is cleared. A target reached west of the meridian therefore shows a positive time to transit of about 24 h, and the Imager Agent does not pause.
- **Flipper.** `Sequencer.js` (around lines 680-690): the flipper waits for `AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY.FLIP_REQUIRED == 1`. It then slews and, as configured, recentres, restarts guiding and settles.
- **Settings published.** `Sequencer.js:647-652, 933-934, 1768-1816`: `MERIDIAN_FLIP` (number, RO; `ENABLED` 0|1, `USE_SOLVER`, `OFFSET` h in −2..2) is defined from `on_enumerate_properties`. The library calls that at load. The property is updated only by `set_meridian_flip()`, the last step of `enable_meridian_flip()` / `disable_meridian_flip()`. A library reload keeps `indigo_meridian_flip` and defines the property again with the same values. The property is not deleted on reload.

The resulting rule:

- A run flips if and only if HA_start < 0 and HA reaches OFFSET before the run ends.
- With OFFSET < 0, capture also idles from max(HA_start, OFFSET) until HA = 0.
- Exposure granularity is ignored.

## 3. Input: caching `MERIDIAN_FLIP` (`indigo_agent_scheduler.c`)

**`scheduler_observed`** (lines 236-265) gets these fields, all guarded by `observed->mutex` like the existing ones:

```c
	// MERIDIAN_FLIP of the Scripting Agent (Sequencer.js); disabled until defined and after its deletion
	bool flip_enabled;
	double flip_offset;
	// incremented when flip_enabled or flip_offset changes
	unsigned flip_generation;
```

**`observe_property()`** (line 3366), in the `SCRIPTING_AGENT` branch:

- On `MERIDIAN_FLIP`, read `ENABLED` and `OFFSET` with `indigo_get_item()`.
- When both items exist, compute `enabled = ENABLED->number.value > 0.5`. Compute `offset` from `OFFSET`, clamped to −2..2.
- When `enabled` or `offset` differs from the cache, store the new values and increment `flip_generation`.

An identical update, including the redefinition after a library reload, changes nothing. The same handler serves define and update.

**`agent_delete_property()`** (line 3470), in the `SCRIPTING_AGENT` branch:

- An empty name (the device is deleted) or `MERIDIAN_FLIP` sets `flip_enabled = false`.
- If it was enabled, `flip_generation` is incremented.
- `flip_offset` is kept. Only `enabled` matters while disabled.

**`USE_SOLVER` is not cached.** This differs from the ticket text. The ticket lists ENABLED/OFFSET/USE_SOLVER, but planning does not use `USE_SOLVER` because durations stay fixed (section 6). Caching it would only cause replans that change nothing. If flip time is later added to durations, it can be cached then.

**Private data** gets two fields:

- `scheduler_flip plan_flip`: the values the current plan was made with.
- `unsigned plan_flip_generation`.

**`process()`** (line 2534) checks the generation first, before `bool plan = ...`:

- Lock `observed->mutex` and read `flip_generation`.
- If it differs from `PRIVATE_DATA->plan_flip_generation`, set `PRIVATE_DATA->plan_dirty = true`.

`visibility_plan_valid()` already returns false while `plan_dirty` is set, so a kept `VISIBILITY` plan is replaced.

**`update_plan()`** (line 2338) copies the inputs once at its start, under `observed->mutex`:

```c
PRIVATE_DATA->plan_flip.enabled = observed->flip_enabled;
PRIVATE_DATA->plan_flip.offset = observed->flip_offset;
PRIVATE_DATA->plan_flip.penalty = SETTINGS_VISIBILITY_FLIP_PENALTY_ITEM->number.value / 60.0;
PRIVATE_DATA->plan_flip_generation = observed->flip_generation;
```

- `plan_visibility()` passes `&PRIVATE_DATA->plan_flip` to the plan function.
- `plan_run_label()` uses the same copy (section 7).
- Planning never reads bus properties.
- `select_planned()` also calls `update_plan()`, so every planning path uses current values.
- The plan assumes the values stay the same for the rest of the night.

## 4. The flip check (`indigo_agent_scheduler_plan.[ch]`)

New public type and two functions in `indigo_agent_scheduler_plan.h`:

```c
/** The meridian flip as the VISIBILITY plan sees it: whether the flip is enabled, its offset to the meridian in hours (negative before it) and the cost of one flip in hours of full-quality imaging.
 */
typedef struct {
	bool enabled;
	double offset;
	double penalty;
} scheduler_flip;

/** True when a run from start to start + duration needs a meridian flip: it starts east of the meridian (hour angle < 0) and reaches flip->offset hours of hour angle before it ends. False for a disabled or NULL flip, a task without a usable target or a start outside the grid.
 */
extern bool indigo_scheduler_run_flips(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, int duration, const scheduler_flip *flip);

/** The cost in hours of a run's meridian flip: 0 without a flip or with flip->penalty 0, otherwise flip->penalty plus, for a negative offset, the wait -max(HA_start, offset) until the meridian.
 */
extern double indigo_scheduler_flip_loss(const scheduler_sky_grid *sky, const scheduler_target_grid *target, const scheduler_header *header, time_t start, int duration, const scheduler_flip *flip);
```

`indigo_scheduler_run_flips()` replaces the prototype function of the same name, which took a bare offset and used globals:

- It returns false when any of these holds:
  - `flip == NULL` or `!flip->enabled`;
  - `!header->has_target`, `target == NULL`, `!target->valid` or `!sky->valid`;
  - `start < sky->start`;
  - `k = (start - sky->start) / SCHEDULER_GRID_STEP` is ≥ `SCHEDULER_GRID_POINTS`.
- It computes HA_start as `target->hour_angle[k] + (start - sky->start - k * SCHEDULER_GRID_STEP) / 3600.0 * SIDEREAL_RATE`, with `#define SIDEREAL_RATE 1.00273790935` local to the file.
- It normalises HA_start to [−12, 12) on both sides. The prototype handled only ≥ 12.
- It returns `ha < 0 && ha + duration / 3600.0 * SIDEREAL_RATE >= flip->offset`.

`indigo_scheduler_flip_loss()` returns 0 when either holds:

- `flip == NULL` or `flip->penalty <= 0`;
- `!indigo_scheduler_run_flips(...)`.

Otherwise it returns `flip->penalty + (flip->offset < 0 ? -max(ha_start, flip->offset) : 0)`. A small static helper computes HA_start once for both functions.

**Penalty 0 turns the whole cost off, the negative-offset wait included.** The ticket says the flip check returns penalty plus wait, but it also says "0 = off" and "penalty 0 … gives plans identical to today". Both cannot hold for OFFSET < 0. This design follows "0 = off" (open question Q1).

## 5. The cost in the objective

**Context.** `plan_context` gets `const scheduler_flip *flip`, set after `init_context()` in `indigo_scheduler_plan_visibility()`. It is NULL in `indigo_scheduler_plan_order()`, `indigo_scheduler_plan()` and `indigo_scheduler_plan_with_hooks()`. `init_context()` keeps its signature.

**`vis_table()`** (line 413), after the value is computed:

```c
if (!urgent) {
	vc->value[i][g] -= 100 * indigo_scheduler_flip_loss(ctx->sky, task->target, task->header, start, task->duration, ctx->flip);
}
```

- The unit is percent of transit altitude × hours. One hour lost is valued as one hour of full-quality imaging.
- Values may become negative. `best_minute` and the DP already compare plain doubles, and `VIS_NONE` (−1e300) stays far below any value.
- Pins of urgent runs, the previous-plan seed (`vis_previous_seed()` → `vis_evaluate()`), `vis_add_left_out()` and `vis_insert_best()` all read the same table. They need no change.

**The `COMPACT` value `VISIBILITY` compares itself with** (`info->compact_value`, line 1085) gets the same cost for a non-urgent run at `runs[k].start`:

```c
if (!is_urgent_task(&ctx, i)) {
	info->compact_value -= 100 * indigo_scheduler_flip_loss(sky, tasks[i].target, tasks[i].header, runs[k].start, tasks[i].duration, flip);
}
```

Without this, `VISIBILITY` would be measured against an unpenalised `COMPACT` and could fall back to `COMPACT` where `COMPACT` flips more.

- The `COMPACT` plan itself and the `COMPACT` selection are unchanged.
- `info->compact_value` and `info->value` now include flip costs. They appear only in the debug line of `plan_visibility()`; the change of meaning is noted in the `scheduler_plan_info` comment.

**Signature change:**

```c
extern int indigo_scheduler_plan_visibility(const scheduler_sky_grid *sky, const scheduler_plan_task *tasks, int count, const scheduler_plan_hook *hooks, int hook_count, time_t now, time_t end, double setting_weight, double altitude_weight, const scheduler_flip *flip, const scheduler_plan_run *previous, int previous_count, scheduler_plan_run *runs, int max_runs, scheduler_plan_info *info);
```

`flip` (NULL = none) goes after the weights. The doc comment gains: "flip (may be NULL) adds the cost of a meridian flip to every run that is not urgent".

**Callers to update (all verified):**

- `indigo_drivers/agent_scheduler/indigo_agent_scheduler.c:2320` (`plan_visibility()`): `&PRIVATE_DATA->plan_flip`.
- `indigo_test/unit/test_agent_scheduler_plan.c`: 18 calls (lines 779, 871, 895, 997, 1025, 1040, 1071, 1097, 1125, 1140, 1172, 1173, 1183, 1200, 1217, 1249, 1266, 1277) get `NULL`.
- `indigo_test/benchmark/bench_scheduler_plan.c:220`: the strategy's flip, see section 10.
- `tools/scheduler_sandbox.py`: does not call the function. It drives the agent over the bus through ctypes only for sun/LST. No change is needed: `set VISIBILITY_FLIP_PENALTY=20` works through its generic `set` verb, and `plan` shows the flip marks.

`indigo_scheduler_plan_order()` stays flip-free with an unchanged signature. `test_visibility_dp_exact` compares it with brute-force enumeration of the quality alone.

## 6. Durations

Durations stay fixed: flip time is not added to the run length, because the plan code assumes one fixed length per task. A run that flips takes longer in reality. The measured duration already includes past flips (guide line 334). Adding flip time is a possible later change, outside this ticket.

## 7. The published plan marks runs that flip

Today `plan_run_label()` (line 2227) writes six columns: when | name | run | priority | state | reason. A planned or running task run has `-` as the reason.

**Least intrusive choice: put `meridian flip` in the reason column.**

- Condition: `indigo_scheduler_run_flips(&PRIVATE_DATA->sky, &task->target, &task->header, run->start, plan_tasks[run->task].duration, &PRIVATE_DATA->plan_flip)`.
- The running run is evaluated with its actual start.
- Hooks are never marked.

Example: `2026-11-19 21:15-21:43 | Soul | run 1/1 | normal 50 | PLANNED | meridian flip`.

- No property, item or item-name change. `AGENT_SCHEDULER_PLAN` stays a light list of `PLAN_%d`.
- `publish_plan()` already redefines the list when a label changes.
- The mark depends only on `MERIDIAN_FLIP.ENABLED`/`OFFSET`, not on the penalty. It is shown under both strategies: under `COMPACT` it is information only (open question Q2).

Documentation updates for the reason column:

- `indigo_docs/PROPERTIES.md` (row `AGENT_SCHEDULER_PLAN`, line 706);
- `indigo_drivers/agent_scheduler/doc/SCHEDULER.md` (Night plan preview, line 702);
- the guide's night plan section (around line 788).

## 8. The setting

**`indigo_libs/indigo/indigo_names.h`**, after `ALTITUDE_WEIGHT` (line 1537):

```c
#define AGENT_SCHEDULER_SETTINGS_VISIBILITY_FLIP_PENALTY_ITEM_NAME   "VISIBILITY_FLIP_PENALTY"
```

with the file's tab alignment.

**`indigo_agent_scheduler.c`:**

- `SETTINGS_VISIBILITY_FLIP_PENALTY_ITEM` becomes `items + 5`, and the following macros shift by one: `IDLE_HOOK_DELAY` 6 … `NIGHT_END_OFFSET` 10.
- `indigo_init_number_property(..., 10)` becomes `11`.
- New init line after the altitude weight:

```c
indigo_init_number_item(SETTINGS_VISIBILITY_FLIP_PENALTY_ITEM, AGENT_SCHEDULER_SETTINGS_VISIBILITY_FLIP_PENALTY_ITEM_NAME, "Best visibility: meridian flip penalty (min)", 0, 120, 1, 10);
```

**Meaning:** the minutes of best-quality imaging the user is willing to give up to avoid one flip. Guidance:

- At least the time a flip takes.
- Higher values avoid flips more strongly.
- 0 turns the cost off.

Everywhere it is documented, the text states that the setting affects best visibility only:

- it is ignored under `COMPACT`;
- it has no effect while `MERIDIAN_FLIP.ENABLED` is 0 or the property is missing.

**Behaviour:**

- The existing `SETTINGS` change branch (line 2972) already sets `plan_dirty`, so a change replans.
- The configuration is saved by item name. An older saved configuration without the item keeps the default 10.
- Users who already run `VISIBILITY` with the flip enabled will therefore see fewer flips after the upgrade. This is the intended default.

**Version:** `DRIVER_VERSION` `0x0300001E` → `0x0300001F`.

## 9. Tests

**Unit tests** (`indigo_test/unit/test_agent_scheduler_plan.c`, new cases in the case table). They use the existing sky at `T_VISIBILITY_NOON` and `LATITUDE`/`LONGITUDE`, with targets placed by `transit_index()` / `grid_time()`.

- `flip_loss_offsets`: for OFFSET −0.5, 0 and +0.5 h, with penalty 10 min:
  - start after transit → no flip, loss 0;
  - start before transit, end just short of OFFSET → 0;
  - start before transit, end just past OFFSET → `run_flips` true, loss = penalty (+ wait for the negative offset);
  - for OFFSET −0.5, a start before −0.5 h: wait 0.5 h; a start between −0.5 h and transit: wait −HA_start.
  - Also: disabled flip, NULL flip and a task without target → false/0; penalty 0 → loss 0 while `run_flips` stays true.
- `visibility_flip_moves_short_task`: a 20-minute task whose best-quality start crosses the meridian is moved to a non-flipping start with penalty 10. It flips (`run_flips` on its start) with penalty 0. The quality loss is below the penalty.
- `visibility_flip_keeps_long_task`: a long task (about 4 h) centred on transit, where any non-flipping placement loses more than the penalty, keeps its flipping start.
- `visibility_flip_urgent_exempt`: an urgent task's run keeps its `COMPACT` start and flips; the table value at its pin is not reduced.
- `visibility_flip_off_is_today`: on the random queues of `test_visibility_never_fewer_runs`, plans with `NULL`, with `{ enabled = false, ... }` and with `{ true, 0, 0 }` are identical (`same_runs`).
- `visibility_flip_never_fewer_runs`: the existing "never fewer runs than `COMPACT`" and constraints-hold checks also hold with penalty 10.

**Integration tests** (`indigo_test/integration/test_agent_scheduler.c`). The suite runs the real Scripting Agent with the real `Sequencer.js`. Each case runs in its own child process, so `indigo_meridian_flip` does not leak between cases.

The test defines `MERIDIAN_FLIP` through the library itself. A full `Sequence` with `enable_meridian_flip()` is not usable here because `set_pause_after_transit()` changes `AGENT_IMAGER_BATCH`, which the Imager Agent stand-in does not have, so the step fails.

Setup and helpers:

- First the case starts the scheduler and waits until `Sequencer.js` is loaded, as `hand_started_sequence_blocks_all_but_urgent` does: `wait_sequence_state(INDIGO_OK_STATE, 15) || wait_sequence_state(INDIGO_IDLE_STATE, 1)`.
- A helper `set_flip(bool enabled, double offset)` sends through `AGENT_SCRIPTING_RUN_SCRIPT`:
  ```js
  indigo_meridian_flip.enabled = true; indigo_meridian_flip.offset = 0.25;
  indigo_update_number_property("Scripting Agent", "MERIDIAN_FLIP", indigo_meridian_flip_items(), "Ok");
  ```
  It then waits until the observer cache shows `MERIDIAN_FLIP` with those values. Using the library's own global keeps the value across any reload of `Sequencer.js` by the scheduler.
- Delete: `indigo_delete_property("Scripting Agent", "MERIDIAN_FLIP");`.
- Define again: `indigo_sequencer.on_enumerate_properties({ device: "Scripting Agent", name: "MERIDIAN_FLIP" });`.

Cases:

- **Change replans.** Under `VISIBILITY`, `set_setting(VISIBILITY_FLIP_PENALTY, 0)`. Add a 10-minute task placed with `upload_placed()` so that its best run crosses transit (for example HA −0.1 h at its planned start, `@duration 10`). Wait for its plan item. With the flip disabled the reason is `-`. Then `set_flip(true, 0)` → `wait_plan_contains("meridian flip", true, 10)`. Then penalty 10 → the run moves and `meridian flip` disappears.
- **Delete disables.** With the flip enabled and marked, delete `MERIDIAN_FLIP` → the mark disappears. Define it again → it returns.
- **Unchanged update.** `set_flip()` with the same values again → the plan labels stay the same. See open question Q3: the bus cannot show that no replan happened.
- **Item count.** `strategy_defaults_to_compact` asserts `ASSERT_EQ_INT(10, settings->count)` at line 2905; this becomes 11. Add an assertion of the new item's default 10 and range 0..120.

## 10. Benchmark

`indigo_test/benchmark/bench_scheduler_plan.c`:

- Pass `NULL` in the existing strategies.
- Add a `VISIBILITY F=10` strategy with `{ true, 0, 10 / 60.0 }`.
- Count flips per executed run with `indigo_scheduler_run_flips()` (OFFSET 0). Print flips and flips per night next to runs and weighted quality for `COMPACT`, `VISIBILITY` and `VISIBILITY F=10`.
- Keep the existing output lines; add one line per strategy.

The prototype `bench_flip.c` stays in the scratchpad and is not checked in.

## 11. Documentation

- `indigo_docs/INDIGO_SCHEDULER_GUIDE.md`:
  - a settings table row after `ALTITUDE_WEIGHT` (min, 10, 0–120, best visibility only);
  - a sentence in the "What the other settings do under `VISIBILITY`" paragraph (line 253): `VISIBILITY` reads `MERIDIAN_FLIP`, and a run that would flip costs `VISIBILITY_FLIP_PENALTY` minutes of full-quality imaging (plus the wait before the meridian with a negative offset), so a flip is avoided when moving the run costs less;
  - the night plan section: the `meridian flip` reason.
- `indigo_drivers/agent_scheduler/doc/SCHEDULER.md`:
  - the settings table (line 380);
  - the plan label (line 702);
  - a short description of the flip rule and cost near the `VISIBILITY` description, including the `MERIDIAN_FLIP` input and the assumption that it does not change during the night.
- `indigo_docs/PROPERTIES.md`:
  - `AGENT_SCHEDULER_SETTINGS` row (line 700): the new item and its meaning;
  - `AGENT_SCHEDULER_PLAN` row (line 706): the reason `meridian flip`.

## 12. Simulation results

### Method

- **Prototype:** `scratchpad/flip/plan_flip.diff` applied to `indigo_agent_scheduler_plan.c`, plus the benchmark `scratchpad/flip/bench_flip.c`.
- **Site and date:** latitude 42, longitude 23, night of 2026-11-19. Night start hook at 12:48 UTC, night end hook at 04:19 UTC, 120 s each.
- **Random nights:** 1000 per offset. The ticket says 5000, but the file headers all read `1000 random nights`, and 1436 flips at 1.44 flips/night confirms 1000.
- **Night contents:** 3–10 tasks per night with:
  - random RA, Dec −10..+70°;
  - `@altitude >` 10..34°, `@twilight astronomical`;
  - duration 10..150 min;
  - 25 % with `@repeat 2 every 60`, 20 % `@priority high`.
- **Execution:** each night is executed as the scheduler would run it. Every run starts at its planned time and succeeds, and the night is planned again after every run (seeded with the previous plan).
- **Flip model:** a run flips if it starts at HA < 0 and reaches HA ≥ OFFSET before it ends.
- **Cost:** `value -= F/60 * 100` for such a run (F in minutes), in `vis_table()` and in the `COMPACT` value. Urgent runs are exempt; the random nights have none.
- **Not modelled:** the negative-offset wait. Only offsets 0, 15, 30, 60 and 120 min were run.

Column meanings in the tables below:

- **Time-weighted quality:** Σ quality × duration / Σ duration.
- **Quality given up per avoided flip:** relative to plain `VISIBILITY`, in percent-hours (Qh). The same in minutes of 100 % imaging is in brackets.

### OFFSET 0 (`r0.txt`)

| Strategy | Runs | Flips | Flips/night | Time-weighted quality % | Quality given up per avoided flip |
|---|---|---|---|---|---|
| COMPACT | 5626 | 645 | 0.65 | 69.7 | – |
| VISIBILITY | 5879 | 1436 | 1.44 | 79.6 | – |
| VISIBILITY + F = 5 | 5877 | 498 | 0.50 | 79.3 | 2.8 (1.7m) |
| VISIBILITY + F = 10 | 5879 | 269 | 0.27 | 78.9 | 4.5 (2.7m) |
| VISIBILITY + F = 15 | 5881 | 189 | 0.19 | 78.7 | 5.6 (3.3m) |
| VISIBILITY + F = 20 | 5880 | 141 | 0.14 | 78.5 | 6.5 (3.9m) |
| VISIBILITY + F = 30 | 5879 | 97 | 0.10 | 78.2 | 7.7 (4.6m) |

Other measurements from the same run:

- **Mean run quality:** 68.7 / 79.7 / 79.4 / 79.1 / 78.9 / 78.7 / 78.5 %.
- **Flips by run length** (<30, 30–60, 60–120, >120 min):
  - VISIBILITY: 148, 256, 670, 362;
  - F = 10: 0, 13, 126, 130.
- **Fewer runs than COMPACT:** one night of 1000 at F = 5 executed fewer runs than `COMPACT` (night 840: 6 vs 7). None at F = 10, 15, 20 or 30.
- **Plan time:** p99 31.22 ms for VISIBILITY and 30.97 ms for F = 10, so the cost adds no measurable time.
- **Penalty 10 vs plain VISIBILITY:**
  - flips 1436 → 269 (−81 %);
  - time-weighted quality 79.6 → 78.9 %;
  - runs unchanged (5879).

A lexicographic variant (`FLIP LEX`, penalty 1e5) reached 49 flips at 77.7 %. It is not proposed.

### Other offsets (`r15.txt`, `r30.txt`, `r60.txt`, `r120.txt`)

| OFFSET | COMPACT flips | VISIBILITY flips (twQ %) | F = 10 flips (twQ %) | F = 10 given up per avoided flip | F = 20 flips |
|---|---|---|---|---|---|
| 15 min | 517 | 1152 (79.6) | 182 (79.0) | 4.8 (2.9m) | 96 |
| 30 min | 409 | 875 (79.6) | 137 (79.1) | 4.4 (2.7m) | 68 |
| 60 min | 225 | 389 (79.6) | 58 (79.4) | 4.3 (2.6m) | 21 |
| 120 min | 30 | 30 (79.6) | 2 (79.6) | 3.9 (2.4m) | 0 |

Runs at F = 10: 5882, 5881, 5879 and 5879 for the four offsets; VISIBILITY 5879 at every offset. A larger offset leaves fewer runs that reach it, so fewer flips are left to avoid. Penalty 10 removes 84–93 % of the remaining flips for less than 0.7 points of time-weighted quality.

### Penalising COMPACT (`rc.txt`, OFFSET 0, the same 1000 nights)

`rc.txt` tested a penalty of S score points on a `COMPACT` candidate whose run started now would flip. Columns "fewer", "more" and "changed" count nights relative to plain `COMPACT`. The last column is relative to `COMPACT` for the C rows and to `VISIBILITY` for `VIS F=10`.

| Strategy | Runs | Flips | Time-weighted quality % | Nights fewer / more runs / changed | Given up per avoided flip |
|---|---|---|---|---|---|
| COMPACT | 5626 | 645 | 69.7 | 0 / 0 / 0 | – |
| C S=25 | 5652 | 490 | 69.3 | 9 / 33 / 161 | 5.7 (3.4m) |
| C S=50 | 5653 | 387 | 68.9 | 16 / 41 / 232 | 17.0 (10.2m) |
| C S=100 | 5644 | 292 | 68.2 | 26 / 42 / 291 | 29.5 (17.7m) |
| C S=200 | 5644 | 285 | 68.2 | 26 / 42 / 295 | 29.6 (17.8m) |
| VISIBILITY | 5879 | 1436 | 79.6 | 0 / 230 / 985 | – |
| VIS F=10 | 5879 | 269 | 78.9 | 0 / 228 / 990 | 4.5 (2.7m) |

Why `COMPACT` stays unchanged:

- **The saving levels off.** The greedy `COMPACT` cannot wait for the meridian, so flips stay at about 285–292 even for S = 100–200.
- **It costs far more per flip.** Each avoided flip gives up 29.5 Qh (17.7 min) at S = 100, about 6.5 times the 4.5 Qh (2.7 min) of `VISIBILITY` with F = 10.
- **It changes many nights and loses runs.** It reorders 291 of 1000 nights and runs fewer tasks than plain `COMPACT` on 26 of them. More on 42, so the totals are slightly higher.
- **It would also change `VISIBILITY`.** `COMPACT` is the starting plan and run set of `VISIBILITY`.

`VISIBILITY` with F = 10 ends below `COMPACT` in flips (269 vs 645) at the cheaper rate and keeps its quality lead. The `COMPACT` value it compares itself with gets the same cost (section 5).

### The owner's sample night (`r0.txt`, OFFSET 0, UTC)

The six tasks:

- Orion `@priority high`, 31 min;
- Rosette, 21 min;
- Jellyfish, 121 min;
- Pleiades, 21 min;
- Elephant, 121 min;
- Soul, 28 min.

All have `@twilight astronomical` and `@altitude > 20`, except Pleiades and Soul with `> 10`.

COMPACT: 6 runs, mean quality 73.1 %, weighted 71.4 %, flips 0.

```
16:45-18:46 Elephant    91.2 %
18:46-19:14 Soul        85.3 %
19:14-19:35 Pleiades    67.3 %
19:35-21:36 Jellyfish   48.0 %
21:36-22:07 Orion       77.5 %
22:07-22:28 Rosette     69.4 %
```

VISIBILITY: 6 runs, mean quality 97.3 %, weighted 95.8 %, flips 3.

```
16:45-18:46 Elephant    91.2 %
21:15-21:43 Soul        99.9 %  FLIP
22:10-22:31 Pleiades    99.9 %  FLIP
23:20-23:51 Orion       98.4 %
23:51-01:52 Jellyfish   98.0 %  FLIP
01:52-02:13 Rosette     96.4 %
```

VISIBILITY + F = 10: 6 runs, mean quality 97.1 %, weighted 94.5 %, flips 0. F = 5, 15, 20 and 30 give the same plan.

```
16:45-18:46 Elephant    91.2 %
21:26-21:54 Soul        99.8 %
22:22-22:43 Pleiades    99.8 %
23:35-00:06 Orion       99.4 %
00:30-00:51 Rosette     99.0 %
00:51-02:52 Jellyfish   93.6 %
```

Soul and Pleiades start 11 and 12 minutes later, once they have crossed the meridian. Jellyfish starts an hour later, after its transit, and swaps places with Rosette. The night gives up 1.3 points of weighted quality (95.8 → 94.5 %) and avoids all three flips.

### Prototype vs. this design

- **Negative-offset wait:** added in this design, not simulated. It only increases the cost of a flipping run when OFFSET < 0.
- **Interface:**
  - The prototype set the cost with globals (`scheduler_flip_penalty` in quality-hours, `scheduler_flip_offset` in h).
  - The design uses the `scheduler_flip` parameter, with `penalty` in hours, from `VISIBILITY_FLIP_PENALTY` in minutes (default 10, 0..120, best visibility only).
  - `100 * penalty_h` equals the prototype's `F/60 * 100`, so the F = 10 rows correspond to the default.
- **Flip check:** `indigo_scheduler_run_flips()` takes the `scheduler_flip` instead of a bare offset. HA is normalised on both sides; the rule is otherwise the same.
- **Not adopted:** the `COMPACT` score penalty (`scheduler_compact_flip_score`, `rc.txt`) and `FLIP LEX`.

## 13. Files to change

| File | Change |
|---|---|
| `indigo_drivers/agent_scheduler/indigo_agent_scheduler_plan.h` | `scheduler_flip`, `indigo_scheduler_run_flips()`, `indigo_scheduler_flip_loss()`, new `flip` parameter of `indigo_scheduler_plan_visibility()`, info comment |
| `indigo_drivers/agent_scheduler/indigo_agent_scheduler_plan.c` | flip functions, `plan_context.flip`, `vis_table()`, `compact_value` |
| `indigo_drivers/agent_scheduler/indigo_agent_scheduler.c` | observed cache, delete handling, generation check in `process()`, copy in `update_plan()`, setting item, label mark, `DRIVER_VERSION` |
| `indigo_libs/indigo/indigo_names.h` | `AGENT_SCHEDULER_SETTINGS_VISIBILITY_FLIP_PENALTY_ITEM_NAME` |
| `indigo_test/unit/test_agent_scheduler_plan.c` | 18 call sites, new cases |
| `indigo_test/integration/test_agent_scheduler.c` | flip helpers and cases, settings count 11 |
| `indigo_test/benchmark/bench_scheduler_plan.c` | call site, flip strategy and counts |
| `indigo_docs/INDIGO_SCHEDULER_GUIDE.md`, `indigo_drivers/agent_scheduler/doc/SCHEDULER.md`, `indigo_docs/PROPERTIES.md` | section 11 |

`tools/scheduler_sandbox.py` needs no change. No new files, so `indigo.xcodeproj` is unchanged. The agent README is not touched.

Checks: the flow check `unit` (it builds the `agent_scheduler` archive). The scheduler integration suite and the benchmark are run by hand and recorded in a ticket note.

## 14. Open questions for the owner

- **Q1. Penalty 0 with a negative offset.** This design makes penalty 0 switch the whole cost off, including the wait before the meridian. Plans with 0 are then identical to today, as the ticket's tests require. The alternative is to always count the wait when the flip is enabled, with penalty 0 only removing the fixed part. Confirm "0 = everything off".
- **Q2. Flip mark under `COMPACT`.** The design marks flipping runs in the plan under both strategies whenever `MERIDIAN_FLIP.ENABLED` is 1, independent of the penalty. Under `COMPACT` it is information only. Alternative: mark only under `VISIBILITY`.
- **Q3. "An unchanged update does not replan" in the integration test.** The bus cannot show that a replan did not happen: a replan with the same inputs publishes the same labels. The design checks the generation logic by code review and checks in the integration test only that the labels stay. A direct test would need a test-only observable, which would expand scope. Is the label check sufficient?
- **Q4. Where users set the flip.** The ticket treats the meridian flip as a global setting made once at startup. The current documentation recommends the opposite:
  - `INDIGO_SCHEDULER_GUIDE.md` line 384 and `SCHEDULER.md` lines 764-766 tell users to call `enable_meridian_flip()` or `disable_meridian_flip()` in every task;
  - the guide's task examples do so (lines 148, 448, 472, 508, 549).

  With this design that still works when every task uses the same settings:
  - before the first task enables the flip, the plan is made with it disabled;
  - after the first run the generation changes and the night is planned again;
  - later identical calls change nothing.

  Tasks that use different flip settings make the plan approximate, and each change replans. Should the documentation in this ticket recommend setting the flip once in the `STARTUP` hook (or identically in every task) and explain that the planner uses the current `MERIDIAN_FLIP`? That is a documentation-only addition beyond the agreed scope, and it would adjust the per-task advice and examples.
