# Design: Scheduler Agent unsafe hold (UNSAFE and SAFE hooks, AGENT_AUX_SAFETY)

Ticket a4c962a2-adfd-4b6a-aa99-fe53ea870579. Depends on edf38e12-685f-4848-be72-3489f0871c19 (light `AGENT_AUX_SAFETY`, item `SAFE`, on "Auxiliary Agent").

The decisions already written in the ticket are not reopened here. Section 13 lists the points that are still open, each with a recommendation. The design text below assumes the recommended answers.

Code references are to `indigo_drivers/agent_scheduler/indigo_agent_scheduler.c` unless another file is named.

## 1. Summary

- The scheduler watches `AGENT_AUX_SAFETY.SAFE` on "Auxiliary Agent" all the time, also while it is stopped. It reduces what it sees to one *effective* value, safe or unsafe, and keeps `last_unsafe`, the wall-clock time of the latest unsafe report.
- When the effective value is unsafe while the scheduler runs, it enters the **unsafe hold** at once. Everything that runs is aborted, except a running `SHUTDOWN`, `NIGHT_END` or `UNSAFE` hook. The `UNSAFE` hook then runs once, and nothing else starts. The night end hook still runs at its boundary (open question Q4).
- The hold ends at a decision once the effective value is safe and `now - last_unsafe >= SAFE_DELAY`. In the night with work, the `SAFE` hook runs. Otherwise the hold is lifted into standby.
- `START` while unsafe, or within `SAFE_DELAY` of the last unsafe report, starts the scheduler in a **start hold**. The startup hook waits, and no `UNSAFE` hook runs for it.
- `last_unsafe` is saved, so after a restart `START` still waits until conditions have been safe for `SAFE_DELAY`.

## 2. New properties and items

Names go into `indigo_libs/indigo/indigo_names.h`, next to the existing `AGENT_SCHEDULER_*` names. Items are appended at the end, so existing saved configurations still load.

| What | Name macro | Value | Details |
|---|---|---|---|
| Hook item | `AGENT_SCHEDULER_HOOKS_UNSAFE_ITEM_NAME` | `UNSAFE` | `HOOKS_PROPERTY->items + 6`, label "Unsafe task", default empty. The property grows from 6 to 8 items. |
| Hook item | `AGENT_SCHEDULER_HOOKS_SAFE_ITEM_NAME` | `SAFE` | `items + 7`, label "Safe task", default empty |
| Setting | `AGENT_SCHEDULER_SETTINGS_SAFE_DELAY_ITEM_NAME` | `SAFE_DELAY` | `SETTINGS_PROPERTY->items + 11`, label "Safe delay (min)", min 0, max 1440, step 0.1, default 10 (Q7). The property grows from 11 to 12 items. |
| Property | `AGENT_SCHEDULER_SAFETY_MISSING_PROPERTY_NAME` | `AGENT_SCHEDULER_SAFETY_MISSING` | switch, RW, `INDIGO_ONE_OF_MANY_RULE`, group "Scheduler", label "Missing safety information" |
| Item | `AGENT_SCHEDULER_SAFETY_MISSING_SAFE_ITEM_NAME` | `SAFE` | "Treat as safe", **default on** |
| Item | `AGENT_SCHEDULER_SAFETY_MISSING_UNSAFE_ITEM_NAME` | `UNSAFE` | "Treat as unsafe" |

- `AGENT_SCHEDULER_SAFETY_MISSING` is handled in `command_handler()` the same way as `AGENT_SCHEDULER_NIGHT`: copy the values, Ok, `schedule_save()` unless the change comes from the configuration reader, and `dirty = plan_dirty = true`. It is defined and enumerated through `define_copy()`, saved in `save_config()` after `STRATEGY_PROPERTY`, and released in detach.
- `hook_kind()` gains `RUN_UNSAFE_HOOK` and `RUN_SAFE_HOOK`, checked after `RESUME`. The task list therefore shows the first matching hook in the order `STARTUP, SHUTDOWN, NIGHT_START, NIGHT_END, IDLE, RESUME, UNSAFE, SAFE`. `task_priority_text()` gains the labels `hook unsafe` and `hook safe`, and `plan_hook_name()` gains `unsafe` and `safe`. `is_hook()` then keeps both tasks out of selection, planning, `all_tasks_finished()` and `DELETE_ALL` without further changes.
- **Auxiliary Agent names.** The scheduler defines them locally, as it already does for `SCRIPTING_AGENT` and `MERIDIAN_FLIP_PROPERTY`: `AUX_AGENT "Auxiliary Agent"`, `AUX_SAFETY_PROPERTY "AGENT_AUX_SAFETY"` and `AUX_SAFETY_SAFE_ITEM "SAFE"`. A comment points to `indigo_agent_auxiliary.h`. No driver includes another driver's header today, and that header also declares `indigo_agent_auxiliary` with `INDIGO_EXTERN` dllexport/dllimport, which is awkward inside another driver on Windows. The test includes `indigo_drivers/agent_auxiliary/indigo_agent_auxiliary.h` and builds its stand-in from `AUX_AGENT_NAME`, `AGENT_AUX_SAFETY_PROPERTY_NAME` and `AGENT_AUX_SAFETY_SAFE_ITEM_NAME`, so a mismatch between the two sets of names fails the tests (Q9).

## 3. Observing the safety

New fields in `scheduler_observed`, guarded by `observed.mutex`:

| Field | Meaning |
|---|---|
| `bool safety_defined` | `AGENT_AUX_SAFETY` is defined on "Auxiliary Agent" |
| `indigo_property_state safety_light` | The light value of item `SAFE`, IDLE when the item is absent |
| `unsigned safety_alerts` | Incremented on every define or update whose `SAFE` is ALERT. This catches an ALERT that reverts to OK between two ticks (0.5 s apart). |

- `observe_property()` (called by define and update): when the device is `AUX_AGENT` and the name is `AUX_SAFETY_PROPERTY`, set `safety_defined`, copy the light and increment `safety_alerts` on ALERT.
- `agent_delete_property()`: when the device is `AUX_AGENT` and the name is empty (device deleted) or `AUX_SAFETY_PROPERTY`, clear `safety_defined`.
- The property state of `AGENT_AUX_SAFETY` is not used, only the item light (Q8).

**Evaluation.** A new function `evaluate_safety(device, now)` runs from `process()` on every tick, before the phase switch, also while the scheduler is stopped:

```
missing = !safety_defined || safety_light == IDLE || safety_light == BUSY
unsafe  = safety_light == ALERT
       || safety_alerts != PRIVATE_DATA->safety_alerts_seen
       || (missing && SAFETY_MISSING.UNSAFE)
safety_alerts_seen = safety_alerts
if unsafe: last_unsafe = now          // time(NULL), wall clock
```

The function also keeps `unsafe_now` (the last value) and `unsafe_missing` (unsafe only because the information is missing), which are used for the reason texts. A change of `unsafe_now` sets `dirty = plan_dirty = true` and calls `schedule_save()`.

**Clock.** `last_unsafe` uses `time(NULL)`, like `delay_until` and the nights. It has to be saved, and a monotonic clock cannot be saved. `SAFE_DELAY` is therefore measured as `now - last_unsafe` in wall-clock seconds against `SAFE_DELAY * 60`.

## 4. Hold state machine

New private data:

| Field | Saved | Meaning |
|---|---|---|
| `time_t last_unsafe` | yes (§10) | Time of the latest unsafe report; 0 means never |
| `bool unsafe_now`, `unsafe_missing` | at save time | Last evaluation |
| `unsigned safety_alerts_seen` | no | Baseline for `safety_alerts` |
| `hold_kind hold` | no | `HOLD_NONE`, `HOLD_START` (postponed start) or `HOLD_RUN` (entered while running) |
| `bool unsafe_hook_done` | no | The `UNSAFE` hook has been handled in this hold (started, skipped by chaining, or reported as missing) |
| `bool hand_abort_done` | no | A sequence started by hand has been aborted at hold entry |

`hold_safe(now)` is true when `!unsafe_now && now - last_unsafe >= SAFE_DELAY * 60`. `hold_remaining(now)` gives the remaining delay in minutes, rounded up.

| From | Event | To | Action |
|---|---|---|---|
| stopped | `START` with `!hold_safe` | `HOLD_START` | `started = true`, `startup_pending` as now; state Busy with message `Unsafe conditions, the start waits for safe conditions` instead of `Scheduler started`. No `UNSAFE` hook. |
| stopped | `START` with `hold_safe` | `HOLD_NONE` | as now |
| `HOLD_NONE`, started | `unsafe_now` (tick) | `HOLD_RUN` | `enter_hold()`, §5 |
| `HOLD_START` | `unsafe_now` | `HOLD_START` | nothing new (nothing runs); the delay restarts |
| `HOLD_RUN` | `unsafe_now` | `HOLD_RUN` | the delay restarts; a running `UNSAFE` hook continues; no second `UNSAFE` |
| `HOLD_START` / `HOLD_RUN` | decision with `hold_safe` | `HOLD_NONE` | `end_hold()`, §7 |
| any hold | `stop_scheduler()` | `HOLD_NONE` | the in-memory hold is cleared; `last_unsafe` stays, so a later `START` waits again if needed |

- Holds are entered only while `started`. While the scheduler is stopped, only `last_unsafe` is tracked.
- `enter_hold()` resets `idle_since = 0`, `idle_hook_done = false` and `idle_hook_reported = false`, because the hold is not an idle period. `end_hold()` resets them again.

## 5. Hold entry: effect on what is running

`enter_hold()` is called from `process()` right after `evaluate_safety()`, when `started && hold == HOLD_NONE && unsafe_now`. It sends the message `Unsafe conditions, the scheduler holds` (`No safety information, treated as unsafe, the scheduler holds` when `unsafe_missing`), sets `hold = HOLD_RUN` with `unsafe_hook_done = hand_abort_done = false`, and then acts on the current phase:

| Phase and kind | Action |
|---|---|
| `RUN_IDLE` | nothing; the hold branch of `process_idle()` takes over |
| `RUN_STARTING`/`RUN_RUNNING`, `RUN_TASK` (normal or urgent, stored or one-off) | `begin_stop(STOP_UNSAFE)` |
| same, `RUN_STARTUP_HOOK`, `RUN_NIGHT_START_HOOK`, `RUN_IDLE_HOOK`, `RUN_RESUME_HOOK`, `RUN_SAFE_HOOK` | `begin_stop(STOP_UNSAFE)` |
| same, `RUN_SHUTDOWN_HOOK`, `RUN_NIGHT_END_HOOK` | not touched; it finishes (owner decision) |
| same, `RUN_UNSAFE_HOOK` | cannot happen in `HOLD_NONE` |
| `RUN_STOPPING`, `STOP_PREEMPT` | `stop = STOP_UNSAFE`. The preempting urgent task cannot start during the hold, and the run is recorded as unsafe instead of preempted, so `@preempted fail` does not apply. |
| `RUN_STOPPING`, `STOP_ABORT`/`STOP_DELETE`/`STOP_FAILURE` | the reason is kept; the hold continues after the stop |

The abort uses the existing path (`send_abort()`, wait for `SEQUENCE_STATE` and the agents, `STOP_TIMEOUT`). A `STOP_TIMEOUT` stops the scheduler with Alert, as now.

**New stop reason `STOP_UNSAFE` in `finish_stop()`:**

- For a task: `add_history(RESULT_UNSAFE, "unsafe conditions")`, `state = TASK_RETRY`, `delay_until = 0`. `runs`, `retries_used` and `measured` stay unchanged, and `@preempted` is ignored. The message is `Task '<name>' aborted by unsafe conditions, it will be retried`. A one-off urgent task stays in the list in RETRY instead of being removed, and runs after the hold, subject to its window (Q2). A stored task gets `apply_pending_edit()` as now.
- For a hook: `finish_hook(RESULT_UNSAFE, "unsafe conditions")`, with the message `Hook '<name>' aborted by unsafe conditions`.
- For `current < 0` (the sequence started by hand): no history entry, as for an urgent preemption.

**New result `RESULT_UNSAFE`:** name `unsafe`, label `UNSAFE`, Busy light (the `default` branch of `history_light()`). `restore_history()` must loop over the whole names array instead of the literal `4`. Older versions read `unsafe` as `failed`, which only matters after a downgrade.

**`finish_hook()` with `RESULT_UNSAFE`** is an interruption, not a failure:

| Kind | Behaviour |
|---|---|
| `RUN_STARTUP_HOOK` | no `stop_scheduler()`; `startup_pending = true`, so the startup hook runs again after the hold (Q6) |
| `RUN_NIGHT_START_HOOK`, `RUN_RESUME_HOOK` | no `request_stop()`; standby is unchanged. The night flag stays set, so `NIGHT_START` does not run again tonight; `SAFE` or `RESUME` prepares the equipment instead. |
| `RUN_IDLE_HOOK` | `idle_hook_done = true`, as now |
| `RUN_SAFE_HOOK` | no stop; the new hold runs `UNSAFE` again |

`chain_slot = slot` and `chain_ok = false`, as for any non-success, so a following hook naming the same task runs again.

**Sequence started by hand (occupied path).** In the hold branch of `process_idle()`, when `occupied && !hand_abort_done`: set `hand_abort_done = true`, `current = -1`, `kind = RUN_TASK`, and call `begin_stop(STOP_UNSAFE)`, with the message `A sequence started by hand was aborted by unsafe conditions`. Only a sequence that is running when the hold begins is aborted. One started by hand later in the hold is left alone (it may be the operator closing the roof, Q3); while it runs, the `UNSAFE` hook waits with the reason `another sequence is running`.

## 6. Decisions during the hold (`process_idle()`)

Order inside `process_idle()`:

1. The stop path (`stop_requested`) stays first and unchanged. `STOP`, `ABORT`, the automatic stop and a hook failure therefore run `SHUTDOWN` during the hold as usual. Once the stop path runs, `UNSAFE` is no longer started.
2. The check-interval gate and the Scripting Agent and Sequencer.js checks, unchanged.
3. **New hold branch** when `hold != HOLD_NONE`:
   1. The hand-started sequence, as in §5.
   2. When `hold == HOLD_RUN && !unsafe_hook_done`: `unsafe_hook_done = true`, then run the `UNSAFE` hook through a helper like `start_night_hook()`. An empty item does nothing. A missing or disabled task gets `send_hook_notice("unsafe", ...)` and the hold continues; this is not a failure. When `slot == chain_slot && chain_ok` (the task has just run as `NIGHT_END` or `SHUTDOWN`), the task does not run again. Otherwise `start_task(slot, RUN_UNSAFE_HOOK)` and return.
   3. **Night end during the hold.** When `night_state.handled && night_state.started`, call `indigo_scheduler_night_decide()` once with `has_work = false`. If it returns `SCHEDULER_NIGHT_END`, run `start_night_hook(HOOKS_NIGHT_END_ITEM, RUN_NIGHT_END_HOOK)`. A night that has not started is never decided during the hold, so no `NIGHT_START` or `SKIP` happens and its start stays pending (Q4).
   4. When `hold_safe(now)`, call `end_hold()` (§7). If it started the `SAFE` hook, return; otherwise fall through to the normal decision in the same pass.
   5. Otherwise set the reason (§11) and `chain_slot = -1` (a decision without a hook ends a chain), and return. No task, urgent task, `STARTUP`, `NIGHT_START`, `RESUME` or `IDLE` starts, and `select_task()` is not called.
4. `startup_pending`, `all_tasks_finished()`, selection, `process_night()`, `RESUME`, `IDLE` — unchanged.

`all_tasks_finished()` is not checked during the hold. A run that ended only for the hold is RETRY, so it cannot finish the list. Tasks that expire during the hold leave it to the first decision after the hold, which then makes the automatic stop.

Estimates (`process_estimation()`), the library load, `update_task_states()` (expiry, RETRY→QUEUED) and planning keep running during the hold.

## 7. Hold end (`end_hold()`)

```
kind = hold; hold = HOLD_NONE; reset idle period; schedule_save()
if startup_pending:                       // postponed start, or startup aborted by the hold
    message "Safe conditions, the scheduler starts"
    return false                          // normal decision: STARTUP, then night start / selection
if kind == HOLD_RUN:
    slot  = select_task(now)
    night = nights.valid ? indigo_scheduler_night_at(&nights, now) : NULL
    in_night = !nights.valid || night != NULL
    has_work = slot >= 0 || (night && plan_first_run > 0 && plan_first_run < night->end)
    if in_night && has_work && SAFE names an enabled task:
        message "Safe conditions, the unsafe hold ends"
        start_task(safe_slot, RUN_SAFE_HOOK); return true
    if SAFE is named but has no task: send_hook_notice("safe", ...)
    standby = true
    message "Safe conditions, the unsafe hold ends, the scheduler continues in standby"
return false
```

- When the site is unknown there is no scheduler night (`nights.valid` is false). The whole time then counts as night, so `SAFE` runs whenever a task is eligible.
- **`SAFE` succeeds:** `standby = false` (like `RESUME`) and normal selection follows. **`SAFE` fails:** `request_stop(ALERT, "The safe hook failed, the scheduler stopped")`, so `SHUTDOWN` runs and the scheduler stops with Alert.
- **Lifted into standby:** later preparation comes from the existing rules. `NIGHT_START` runs at the next night with work, and `RESUME` runs before a task in standby when it is named. Without `RESUME`, a task in standby starts directly, as documented today.
- **A night start that passed during the hold:** if the hold ends in that night with work, `SAFE` runs, and at the next decision `process_night()` runs the pending `NIGHT_START` (the night flag is still clear). When both items name one task, the task runs once, through `chain_slot` (Q5).
- **Night boundaries during a `HOLD_START`:** `night_state` was reset by `START`, and nothing is decided until the startup hook has run. This matches a normal `START` in the night.

## 8. Controls during the hold

| Control | Behaviour |
|---|---|
| `STOP` | As now: `request_stop(OK)`. A running `UNSAFE`, `SHUTDOWN` or `NIGHT_END` hook finishes first; then `SHUTDOWN` runs (once per the chaining rule if `UNSAFE` names the same task and succeeded) and the scheduler stops Ok. `last_unsafe` stays. |
| `ABORT` | As now: aborts the running hook, including `UNSAFE`, then runs `SHUTDOWN` and stops with Alert. |
| `START` (already started) | As now: clears a pending stop; the hold stays. `START` during the `SHUTDOWN` hook continues with `startup_pending`, and the startup waits for the hold. |
| `START` (stopped) | §4: `HOLD_START` when `!hold_safe`. |

`STATE` stays Busy during the hold. It becomes Alert after a failed `UNSAFE` or `SAFE` hook, which is added to the documented list of Alert causes.

## 9. Hook failure paths

| Hook | Result | Consequence |
|---|---|---|
| `UNSAFE` | failed | `request_stop(ALERT, "The unsafe hook failed, the scheduler stopped")` → `SHUTDOWN` → Alert |
| `UNSAFE` | aborted by `ABORT`/delete | as for any hook; `ABORT` stops anyway |
| `UNSAFE` | names no task / disabled | notice, the hold continues, no stop |
| `SAFE` | failed | `request_stop(ALERT, "The safe hook failed, the scheduler stopped")` → `SHUTDOWN` → Alert |
| `SAFE` | aborted by a new hold | `RESULT_UNSAFE`, new `HOLD_RUN`, `UNSAFE` again |
| `STARTUP`, `NIGHT_START`, `RESUME`, `IDLE` | aborted by the hold | `RESULT_UNSAFE`, no stop, §5 |
| `SHUTDOWN`, `NIGHT_END` | running at hold entry | finish; their own failure rules apply as now |

## 10. Persistence and restart

- `AGENT_SCHEDULER_SAFETY_MISSING`, the new `HOOKS` items and `SAFE_DELAY` are saved with their properties.
- The safety state is saved as one meta line at the start of the `ENTRIES` of the saved `AGENT_SCHEDULER_RESTORE_HISTORY`, the same way as the existing `#urgent` line (Q10):
  - `#safety\tunsafe\t<time>`: the effective value was unsafe at the save; `<time>` is informational;
  - `#safety\tsafe\t<last_unsafe>`: safe at the save, with the time of the latest unsafe report.

  The line has 3 fields. A version without it skips the line, because an entry needs at least 5 fields. It is written when `last_unsafe != 0`, and `CLEAR_HISTORY` does not clear it.
- Saves happen on each change of `unsafe_now` and at hold entry and end. Saving is not tied to the tick, so a long unsafe period does not rewrite the file every two seconds.
- **Restore** (`restore_history()`): for `unsafe`, set `last_unsafe = time(NULL)` at load, because the server was down and the conditions are unknown. For `safe`, use the saved time. `unsafe_now` starts false and is re-evaluated at the first tick from the live property.
- **Restart while unsafe:** the scheduler starts stopped, as it always does. `START` then makes a start hold until the safety has been OK for `SAFE_DELAY`, counted from the later of the restore time and the last unsafe report. This holds even when the Auxiliary Agent reports OK immediately, or is absent and missing counts as safe. When the hold ends, `STARTUP` runs and then `NIGHT_START` during a night with work, so no `SAFE` hook is needed. The hold kind, `unsafe_hook_done` and standby are not saved.

## 11. Reason and message texts

`CURRENT.REASON` during the hold, set by the hold branch (Busy and running hooks keep the empty reason, as now):

| Condition | Reason |
|---|---|
| unsafe, `SAFE` ALERT | `unsafe conditions` |
| unsafe because the information is missing | `no safety information, treated as unsafe` |
| safe, delay running | `unsafe hold ends in <n> min` (rounded up) |
| a sequence started by hand blocks `UNSAFE` | `another sequence is running` (existing) |

Messages, all through `indigo_send_message()` except the `STATE` one:

- `STATE` message on a postponed `START`: `Unsafe conditions, the start waits for safe conditions`
- `Unsafe conditions, the scheduler holds` / `No safety information, treated as unsafe, the scheduler holds`
- `Task '<name>' aborted by unsafe conditions, it will be retried`
- `Hook '<name>' aborted by unsafe conditions`
- `A sequence started by hand was aborted by unsafe conditions`
- `The unsafe hook '<name>' names no task` / `... is disabled, skipped` (and the same with `safe`), through `send_hook_notice()`
- `Safe conditions, the unsafe hold ends` / `..., the scheduler continues in standby` / `Safe conditions, the scheduler starts`
- `The unsafe hook failed, the scheduler stopped` / `The safe hook failed, the scheduler stopped` (STATE Alert)
- History reason: `unsafe conditions`, result `UNSAFE`.

## 12. Plan preview and edge cases

**Plan during the hold.** `update_plan()` clamps every task's `available_from` to the earliest possible hold end: `last_unsafe + SAFE_DELAY*60` when safe, `now + SAFE_DELAY*60` while unsafe. No run is then planned inside the hold. A running `UNSAFE` hook appears as the Busy first item, through the existing running-hook path. `SAFE` and `UNSAFE` are not forecast, like `IDLE`, `RESUME` and `STARTUP`. No extra plan item is added (Q11). A change of `unsafe_now` sets `plan_dirty`, and the periodic plan refresh follows the moving delay.

**Edge cases:**

| Case | Behaviour |
|---|---|
| Safety flapping | Every unsafe report, including an ALERT shorter than a tick (`safety_alerts`), sets `last_unsafe`. The hold ends only after `SAFE_DELAY` of continuous safe readings. No second `UNSAFE` runs in one hold. A flap after the hold ended (for example during `SAFE`) starts a new hold with a new `UNSAFE`. |
| Unsafe while a stop for another reason runs | §5: `PREEMPT` becomes `UNSAFE`; `ABORT`, `DELETE` and `FAILURE` keep their reason and history; the hold continues after it. If `stop_requested` is set (STOP, ABORT, auto stop), the stop path runs `SHUTDOWN`, and `UNSAFE` is skipped. |
| Unsafe while `UNSAFE` runs | It continues; only `last_unsafe` moves. |
| `UNSAFE` names a missing task | Notice once per hold (`unsafe_hook_done`); the hold continues. |
| `SAFE`/`UNSAFE` name another hook's task | Allowed. The run kind comes from the caller, not from `hook_kind()`. Back-to-back runs of one task are merged by `chain_slot`/`chain_ok`, for example `NIGHT_END`→`UNSAFE`, `UNSAFE`→`SHUTDOWN` on `STOP`, or `SAFE`→`NIGHT_START`. The label shows the first role in the documented order. |
| Delete of the running `UNSAFE` task | `STOP_DELETE` as for any hook; `unsafe_hook_done` stays true. |
| `SAFE_DELAY` or `SAFETY_MISSING` changed during the hold | Takes effect at the next decision. |
| Auxiliary Agent absent with missing = safe | Behaves exactly as today; all existing tests stay valid. |
| Remote "Auxiliary Agent @ host" | Not matched; only the exact local name is, like the other agents (phase 4 will add selection). |

## 13. Open questions with recommendations

| # | Question | Recommendation |
|---|---|---|
| Q1 | Postponed `START`: does `SAFE_DELAY` apply, and does `UNSAFE` run? | **`SAFE_DELAY` applies.** `START` waits until conditions have been safe for `SAFE_DELAY` (counted from `last_unsafe`, also when that report came while stopped or before a restart), so a flapping sensor cannot start and then abort the startup hook. **`UNSAFE` does not run**, because this start has prepared nothing, and at the end the startup hook runs instead of `SAFE`. Order after the hold: `STARTUP`, then `NIGHT_START` (night with work), then tasks. |
| Q2 | A one-off urgent task (`AGENT_SCHEDULER_URGENT_TASK`) aborted by the hold: keep or drop? Today `ABORT` drops it. | **Keep it** in RETRY so it runs after the hold, matching "the task returns to RETRY". It is still lost on restart, as now. |
| Q3 | Abort only the hand-started sequence running at hold entry, or every one started during the hold? | **Only at entry.** One started later may be the operator closing the roof by hand. |
| Q4 | Does `NIGHT_END` run at its boundary during the hold? | **Yes.** It puts equipment away, and the owner's list of hooks blocked during the hold does not include it. A night start during the hold stays pending. |
| Q5 | A night start passed during the hold, and the hold ends in that night with work. | **`SAFE` then `NIGHT_START`** (the night was never prepared); it runs once when both name one task. |
| Q6 | Startup hook aborted by the hold. | **Rerun `STARTUP` after the hold** (`startup_pending = true`), without `SAFE`. `UNSAFE` still runs for this hold, since the startup hook may have half-prepared the equipment. |
| Q7 | `SAFE_DELAY` default, range, step. | **Default 10 min, 0–1440 min, step 0.1.** The 0.1 step lets the tests use 6 s; 0 means the hold ends at the first safe decision. |
| Q8 | Which value counts: the item light or also the property state? What about BUSY? | **Item `SAFE` light only.** OK = safe, ALERT = unsafe, IDLE, BUSY or an absent item = missing. |
| Q9 | Include `indigo_agent_auxiliary.h` or define the strings? | **Define locally**; the test uses the header names to catch a mismatch. |
| Q10 | Where to save the safety state. | **A `#safety` meta line in the saved `RESTORE_HISTORY` entries** (precedent `#urgent`; no new property; older versions ignore it). Alternative: a new restore-only property `AGENT_SCHEDULER_RESTORE_SAFETY`. |
| Q11 | Plan during the hold. | **Clamp the task starts to the earliest hold end; no extra plan item.** The hold is visible in `CURRENT.REASON` and the messages. |
| Q12 | Name of the missing-property switch. | `AGENT_SCHEDULER_SAFETY_MISSING` with `SAFE`/`UNSAFE`. |
| Q13 | Publish a read-only safety status property (for example the effective value and the remaining delay)? | **No**, not requested; `REASON` covers it. It can be added later. |
| Q14 | `SAFE` empty or naming no task when the hold ends at night with work. | **Lift into standby** (with a notice for a missing task), so `RESUME` prepares if named. |

## 14. Test plan

File `indigo_test/integration/test_agent_scheduler.c`. New cases are added to `cases[]`; the default timeout of 150 s is enough.

**Stand-in Auxiliary Agent.** `static indigo_device aux_standin = INDIGO_DEVICE_INITIALIZER(AUX_AGENT_NAME, NULL, aux_enumerate, NULL, NULL, NULL);` and `safety_property = indigo_init_light_property(NULL, AUX_AGENT_NAME, AGENT_AUX_SAFETY_PROPERTY_NAME, "Safety", "Safety", INDIGO_OK_STATE, 1)` with the item `AGENT_AUX_SAFETY_SAFE_ITEM_NAME`, created in `setup()` but **not attached**, so every existing case runs without it, which is missing treated as safe. Helpers:

- `aux_attach(state)` / `aux_detach()`;
- `set_safety(state)`: sets the item light and calls `indigo_update_property()`;
- `safety_blip()`: ALERT then OK back to back, faster than the 0.5 s tick;
- `delete_safety()` / `define_safety()`: delete or redefine the property while the device stays attached;
- `set_missing(bool unsafe)`, `set_safety_hooks(unsafe, safe)`.

`cleanup()` detaches the stand-in and releases the property.

**Timing.** `fast_settings()` stays. Cases that do not test the delay set `SAFE_DELAY = 0`. Delay cases use `0.1` (6 s) and measure with `time(NULL)`, allowing ±1.5 s for the 1 s check interval and the tick. Night cases reuse `night_longitude()`, `start_in_night()` and `set_site()`. Bodies use `BODY_WAIT(n)`; failing hooks throw in the body, as the existing failed-hook cases do.

| Ticket scenario | Case | Checks |
|---|---|---|
| Normal task aborted | `unsafe_hold_aborts_a_running_task` | RUNNING task with `@retry 1` and `@preempted fail`: ALERT → history `UNSAFE` reason `unsafe conditions`, task QUEUED with `retries left 1`, not FAILED; `UNSAFE` hook runs once; reason `unsafe conditions`; no `Starting task` during the hold; OK → hold ends, task runs again |
| Urgent task aborted | `unsafe_hold_aborts_an_urgent_task` | One-off urgent task running → `UNSAFE`, kept in RETRY, runs after the hold |
| Manual sequence aborted | `unsafe_hold_aborts_a_hand_started_sequence` | Hand-started 30 s sequence aborted with the message; `UNSAFE` runs after it stops; a sequence started by hand during the hold is not aborted |
| Hooks that finish | `unsafe_hold_lets_shutdown_finish`, `unsafe_hold_lets_night_end_finish` | `SHUTDOWN` (after `STOP`) and `NIGHT_END` (`start_in_night`) running at ALERT end `SUCCEEDED`; after `NIGHT_END` the `UNSAFE` hook runs |
| Hooks that are aborted | `unsafe_hold_aborts_the_startup_hook`, `unsafe_hold_aborts_the_idle_hook`, `unsafe_hold_aborts_the_night_start_hook`, `unsafe_hold_aborts_the_resume_hook`, `unsafe_hold_aborts_the_safe_hook` | History `UNSAFE` marked hook; no stop; startup reruns after the hold; an aborted `SAFE` leads to a second `UNSAFE` |
| Night end during the hold | `night_end_runs_during_the_hold` | Hold before dawn; `NIGHT_END` runs at dawn; no `NIGHT_START` during a hold across dusk |
| `SAFE_DELAY` reset | `safe_delay_restarts_after_a_short_alert` | `SAFE_DELAY 0.1`; OK, then `safety_blip()` after 3 s → the hold ends no earlier than 6 s after the blip; reason `unsafe hold ends in 1 min` |
| `SAFE` only at night with work | `safe_hook_runs_at_night_with_work`, `safe_hook_is_skipped_by_day`, `safe_hook_is_skipped_without_work` | Night and eligible task: `SAFE` then the task without `RESUME`. Day: no `SAFE`, standby message; `NIGHT_START` runs at dusk. Night without work: standby; a task uploaded later runs `RESUME` first |
| Hook failures | `failed_unsafe_hook_stops_the_scheduler`, `failed_safe_hook_stops_the_scheduler`, `missing_unsafe_hook_is_reported` | `SHUTDOWN` runs, STATE Alert with the message; a missing `UNSAFE` task gives the notice and no stop |
| Missing switch, safe (default) | `missing_safety_counts_as_safe_by_default` | No stand-in, then the stand-in IDLE, then property deleted: tasks run, no hold |
| Missing switch, unsafe | `missing_safety_counts_as_unsafe_when_selected` | `UNSAFE`: no stand-in → `START` postponed with reason `no safety information, treated as unsafe`; IDLE still holds; OK → starts; `delete_safety()` while running → hold and `UNSAFE` |
| Postponed start | `start_waits_for_safe_conditions` | ALERT, `START` → STATE message, no `STARTUP` and no `UNSAFE`; OK with `SAFE_DELAY 0.1` → `STARTUP` about 6 s later, then the task |
| Controls | `stop_during_the_hold_runs_the_shutdown_hook`, `abort_during_the_unsafe_hook` | Ok stop with `SHUTDOWN`; `ABORT` aborts `UNSAFE`, `SHUTDOWN`, Alert |
| Persistence | `unsafe_hold_survives_a_restart` | ALERT during the run, restart the scheduler (`stop_scheduler()`/`start_scheduler()`), stand-in reports OK at once, `SAFE_DELAY 0.1`: `START` is postponed and `STARTUP` runs only after the delay; `SAFETY_MISSING`, `SAFE_DELAY`, `UNSAFE` and `SAFE` are restored; a saved `#safety safe` older than the delay does not postpone |
| Labels | `safety_hooks_are_labelled` | Task list priority `hook unsafe` / `hook safe`; neither task is selected as an ordinary task |

The narrowest check is the scheduler integration binary filtered by `unsafe` / `safe` / `safety` (`SCHEDULER_TEST_FILTER`). The whole scheduler integration suite is then run once to confirm that the existing cases are unaffected.

## 15. Documentation

- `indigo_docs/PROPERTIES.md`, Scheduler agent table: the `AGENT_SCHEDULER_HOOKS` items and description, `SAFE_DELAY` in `AGENT_SCHEDULER_SETTINGS`, a new row for `AGENT_SCHEDULER_SAFETY_MISSING`, the `UNSAFE` result (Busy) in `AGENT_SCHEDULER_HISTORY`, the new Alert causes in `AGENT_SCHEDULER_STATE`, and a note that the agent observes `AGENT_AUX_SAFETY` of the Auxiliary Agent.
- `indigo_drivers/agent_scheduler/doc/SCHEDULER.md`:
  - survey row (line 33) and adopted list (line 50): describe the delivered behaviour;
  - Agent dependencies: a new "Auxiliary Agent" row, replacing the sentence at line 97;
  - property and settings tables;
  - Scheduler control: `START` while unsafe;
  - Hooks: the new roles and the label order;
  - a new section **Unsafe hold** after Night hooks, covering §3–§12;
  - Urgent preemption: line 797 becomes a statement of the implemented rule;
  - Aborting a sequence: the hold added to the list;
  - Run history: the `unsafe` result;
  - Persistence: the `#safety` line;
  - Night plan preview: the clamp;
  - Implementation phases: phase 6 rewritten as implemented (line 869);
  - Testing: the new cases.
- `indigo_docs/INDIGO_SCHEDULER_GUIDE.md`: an "Unsafe conditions" subsection under Hooks and the night, plus Hook rules, Settings, Scheduler state, The history, Messages, Reasons, What happens when the server restarts, Agents the scheduler depends on, and Property reference.
- No `README.md` changes.

## 16. Version and files

- `DRIVER_VERSION` 0x0300001F → **0x03000020**. Remove the stale `TODO: add weather safety checks` comment at line 22.

| File | Change |
|---|---|
| `indigo_libs/indigo/indigo_names.h` | 6 new name macros (§2) |
| `indigo_drivers/agent_scheduler/indigo_agent_scheduler.c` | everything in §2–§12, version |
| `indigo_test/integration/test_agent_scheduler.c` | stand-in, helpers, cases (§14) |
| `indigo_docs/PROPERTIES.md` | §15 |
| `indigo_drivers/agent_scheduler/doc/SCHEDULER.md` | §15 |
| `indigo_docs/INDIGO_SCHEDULER_GUIDE.md` | §15 |

`indigo_agent_scheduler_night.c` and `.h` are not changed; the hold uses `indigo_scheduler_night_decide()` as it is. No new files, so no Xcode or Visual Studio project changes.

Build dependency: the test needs `indigo_agent_auxiliary.h` with the safety names from edf38e12. If this ticket lands first, the test defines the same three strings under `#ifndef` until the header provides them.
