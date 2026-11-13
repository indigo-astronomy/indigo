# INDIGO Scheduler

Revision: 12.11.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

This document is the design of the INDIGO Scheduler Agent. The agent decides which imaging task runs next, when it runs and under which sky conditions, and it can interrupt the running task for an urgent one. It is a design document. Property names, limits and defaults are subject to change until the agent is implemented. How to use the scheduler is described in the [Scheduler User Guide](../../../indigo_docs/INDIGO_SCHEDULER_GUIDE.md).

## Motivation

INDIGO already runs imaging sequences. `Sequencer.js` executes a sequence inside the Scripting Agent (`indigo_drivers/agent_scripting/library/Sequencer.js`). It handles device selection, slewing, centering, focusing, guiding, capture and meridian flips. Only one sequence runs at a time, and the client starts it by hand.

The following are missing:

- a list of tasks that a client can create, store, download and edit;
- automatic selection of the next task from sky constraints and priorities;
- a way to stop the running task for an urgent target such as a supernova, a GRB alert or a new comet.

## Survey of existing schedulers

The last column shows the proposed INDIGO scheduler for comparison.

| Feature | Ekos | SGP | N.I.N.A. Target Scheduler | Voyager RoboTarget | ACP Scheduler | INDIGO (proposed) |
|---|---|---|---|---|---|---|
| Job unit | target + `.esq` sequence, job list `.esl` | target with events, sequence file | project → target → exposure plan (database) | set → target → shots | project → plan → observation (plan text files) | task text: `// @key` header + `Sequencer.js` body, stored in agent properties |
| Selection | greedy, re-planned every second | list order | weighted score, re-planned per step | priority order | weighted efficiency score | score, selected only between tasks |
| Priority | row order | none | one weighted rule | Very Low … First | per-user, normalized | urgent, high, normal, low class + level 0–100 |
| Constraints | altitude, horizon, moon separation, twilight, weather | start/end time or altitude | altitude, horizon, meridian window, moon (Lorentzian), twilight per filter, minimum time span | altitude, HA window, moon phase and distance, Lorentzian | air mass, horizon, moon, dark time, checked at start and end | altitude, airmass, HA, moon separation (Lorentzian), moon illumination, twilight, time window, checked over the whole duration |
| Completion | repeat N, until a time, loop | events done or end time | accepted frames reached | shots done | plan done, "best efforts" | sequence ends Ok; repeat N with interval |
| Preempts a running job | yes (greedy) | no | no (at boundaries only) | no | by priority no; Target of Opportunity (ToO) API yes | urgent tasks only; the preempted task is retried or failed |
| Weather and safety | warning suspends, alert parks | safety monitor and recovery | N.I.N.A. triggers | yes | weather script, failed plan re-queued | planned: warning blocks new tasks, alert aborts and runs the shutdown hook |
| Extras | startup and shutdown queues, mosaic, retry policy | recovery mode | image grading | maximum time slot | retry bonus, VOEvent receiver | startup, shutdown and idle hooks, retry policy, night plan preview, run history |

Professional systems handle urgent observations as an interrupt that is separate from normal priorities. Las Cumbres Observatory rapid-response requests abort the observation in progress. ACP's ToO interface halts acquisition, fails the running plan and re-queues it.

ACP plan files and Ekos `.esl`/`.esq` files show that tasks are most useful as editable, shareable text documents.

The INDIGO scheduler adopts:

- an urgent interrupt that is separate from normal priorities, with the preempted task failed or re-queued (ACP ToO model);
- scoring between tasks;
- a constraint check over the whole expected task duration, so that a task is started only if it can finish;
- altitude, airmass, hour angle, moon separation (optionally Lorentzian), moon illumination, twilight and time-window constraints;
- repeat and retry;
- startup, shutdown and idle hooks;
- safety handling.

It deliberately leaves out continuous greedy preemption, anti-thrashing settings and frame-level resume. Image grading and mosaics are possible later extensions.

## Design principles

- **Tasks are not preemptible, except by urgent tasks.** Only an urgent task interrupts a running non-urgent task. All other scheduling decisions are made between tasks.
- **A preempted task is failed or re-queued.** The task's header chooses which. A re-queued task reruns its sequence from the beginning.
- **A task is one human-readable text.** It consists of a header with the scheduling fields followed by an ordinary `Sequencer.js` body.
- **The agent stores tasks like the Scripting Agent stores scripts.** Tasks live in agent properties and in the agent's INDIGO configuration. The agent never reads or writes task files; clients load, save and edit files and transfer them through properties.
- **Device control stays in `Sequencer.js`.** The scheduler does not duplicate any device logic. It requires two small additions to `Sequencer.js`: a load that does not replace a running sequence, see [Running a task](#running-a-task), and the `estimate()` method, see [Task duration](#task-duration). The only device actions the scheduler performs itself are aborts.

## Architecture

| Concern | Owner |
|---|---|
| Task store and persistence, header parsing, constraints, selection, urgent preemption, ephemeris | Scheduler Agent (`indigo_agent_scheduler`, C) |
| Device control inside a task: slew, center, focus, guide, capture, meridian flip | the task body, executed by `Sequencer.js` in the Scripting Agent |

The scheduler is a separate C agent rather than a JavaScript library, for these reasons:

- The Scripting Agent has one shared Duktape heap, and the sequencer is a singleton. A C agent watches and aborts the sequence from outside it.
- The agent keeps its task list in its own configuration and is not affected by script errors.
- Constraints use the C ephemeris functions directly:
  - `indigo_j2k_to_jnow()` converts the J2000 `@target` to the equinox of date;
  - `indigo_radec_to_altaz()` and `indigo_airmass()` give the altitude and airmass;
  - `indigo_raise_set()` gives the rise, transit and set times;
  - `indigocat_sun_equatorial_coords()` and `indigocat_moon_equatorial_coords()` (`indigocat/indigocat_solar_system.h`) give the Sun and Moon positions.

  No moon phase helper exists yet. The moon illumination and the number of days from full moon are derived from the Sun–Moon elongation. The elongation does not distinguish a waxing from a waning moon, which the constraints do not need, because the Lorentzian limit is symmetric about full moon.
- Any client can submit an urgent task with a single property change. This includes AIN, the web UI, MCP clients and a future GCN/VOEvent bridge.

The Scheduler Agent is a device with its own client, like the Scripting Agent. It does not select hardware devices, so it is not a filter agent.

### Agent dependencies

The Scheduler Agent depends directly on these agents, found by their exact device names:

| Agent | Needed | Properties the scheduler uses | Without it |
|---|---|---|---|
| "Scripting Agent" | always | changes `AGENT_SCRIPTING_RUN_SCRIPT` to load `Sequencer.js`, estimate durations and run tasks; observes `SEQUENCE_STATE` and `SEQUENCE_NAME`, and the `SCHEDULER_RESULT` and `SCHEDULER_ESTIMATE` properties that the wrapper script defines there; changes `AGENT_ABORT_PROCESS` to abort a sequence | no task starts and no duration is estimated; `REASON` is `waiting for the Scripting Agent` |
| "Mount Agent" | for tasks with `@target` or `@twilight` | observes `GEOGRAPHIC_COORDINATES` for the site; observes `AGENT_START_PROCESS` to wait until the agent is idle after an abort | these tasks wait with `site coordinates unknown`; tasks with only time windows still run |
| "Guider Agent" | optional | changes `AGENT_ABORT_PROCESS` on `ABORT` and preemption, because the sequencer's abort leaves guiding running; observes `AGENT_START_PROCESS` | nothing to stop |
| "Imager Agent", "Astrometry Agent", "Platesolver Agent", "ASTAP Agent" | optional | observe `AGENT_START_PROCESS`, to wait until they are idle after an abort before the next task starts | nothing to wait for |

An unconfigured Mount Agent reports 0° latitude and 0° longitude, which the scheduler cannot tell from a real site, so the site must be configured in the Mount Agent.

The task bodies depend on more: the agents and devices their `Sequencer.js` steps use, by default the "Imager Agent", "Mount Agent" and "Guider Agent" and a plate solver agent selected for the Imager Agent. The scheduler does not check these; a missing one makes the run fail. A safety or weather device is added in a later phase.

The agent names are currently fixed, so the Scheduler Agent must run on the same server as these agents: an agent on another server appears with a host suffix, such as `Scripting Agent @ obs2`, and is not used. A later phase lets the user select the Scripting Agent, see [Implementation phases](#implementation-phases).

## Task format

A task is a JavaScript text. It starts with a header of `// @key value` comment lines, followed by the body. Because the header consists only of comments, the whole task is valid JavaScript, and existing sequence editors can open it.

```js
// @task       M31 LRGB
// @priority   normal 50
// @target     00:42:44.3 +41:16:09
// @altitude   > 30
// @airmass    < 2.0
// @moon       > 40 width 7
// @twilight   astronomical
// @window     2026-11-12 20:00 .. 2026-11-13 04:00
// @repeat     1
// @retry      2 after 15
// @preempted  retry

var sequence = new Sequence(scheduler_task.name);
sequence.enable_meridian_flip(true, 0);
sequence.precise_goto(5, scheduler_task.ra, scheduler_task.dec);
sequence.start_guiding();
sequence.select_filter("L");
sequence.capture_batch(20, 300);
sequence.stop_guiding();
sequence.start();
```

An urgent task has the same form:

```js
// @task       SN 2026abc
// @priority   urgent
// @target     13:29:52.7 +47:11:43
// @altitude   > 20
// @duration   30

var sequence = new Sequence(scheduler_task.name);
sequence.disable_meridian_flip();
sequence.precise_goto(5, scheduler_task.ra, scheduler_task.dec);
sequence.select_filter("L");
sequence.capture_batch(10, 60);
sequence.start();
```

### The `scheduler_task` object

The wrapper script that runs a task (see [Running a task](#running-a-task)) defines a `scheduler_task` object from the validated header before it evaluates the task text:

```js
var scheduler_task = { name: "M31 LRGB", ra: 0.712306, dec: 41.269167, run: 1, token: 17 };
```

- `ra` is in hours and `dec` in degrees, both J2000. They are defined only when `@target` is present.
- `run` is the number of the run, starting at 1.
- `token` identifies this run or estimate in the results the wrapper reports.
- `estimate` is defined only while the agent estimates the task duration, see [Task duration](#task-duration).

String values are JSON-encoded, so that quotes or backslashes in a task name cannot break or alter the script. The body uses these values, so the name and the coordinates are written only once, in the header. Naming the sequence after the task also lets the scheduler recognise its sequence in `SEQUENCE_NAME`. A task run by hand outside the scheduler must define `scheduler_task` itself or use literal values.

### Lexical rules

- The text is UTF-8. A leading byte-order mark is ignored, and CRLF line endings are accepted.
- A **header line** is `//`, optional spaces or tabs, `@`, a key, then at least one space or tab and the value. Trailing whitespace is ignored.
- An **empty comment line** (`//` with only whitespace after it) is allowed inside the header.
- The header starts at the first line and ends at the first line that is neither a header line nor an empty comment line. A blank line ends the header.
- `// @key` lines after the end of the header belong to the body and are ignored by the scheduler.
- Keys and keywords are case-insensitive. Every key except `@task` is optional, and each key appears at most once.
- Comparison operators are mandatory where shown: `>` or `>=` for a minimum and `<` or `<=` for a maximum. The two forms mean the same.

### Header keys

| Key | Syntax | Meaning | Default |
|---|---|---|---|
| `@task` | `name` | Display name, 1–40 bytes, no control characters, unique among stored and urgent tasks, compared case-insensitively | required |
| `@priority` | `urgent` \| `high` \| `normal` \| `low` [`level`] | Priority class and level 0–100 within the class. The level is ignored for `urgent` | `normal 50` |
| `@target` | `ra dec` | J2000 coordinates. RA in hours (`hh:mm:ss.s` or decimal) and Dec in degrees (`±dd:mm:ss` or decimal) | none |
| `@altitude` | `> degrees` | Minimum target altitude, 0–90 | off |
| `@airmass` | `< value` | Maximum target airmass, ≥ 1 | off |
| `@ha` | `from .. to` | Allowed hour-angle range in hours, −12 to +12. `from` greater than `to` wraps across 12h | off |
| `@moon` | `> degrees` [`width days`] | Minimum moon separation, see [Lorentzian moon avoidance](#lorentzian-moon-avoidance); ignored while the moon is below the horizon | off |
| `@illumination` | `< percent` | Maximum moon illumination, 0–100; ignored while the moon is below the horizon | off |
| `@twilight` | `none` \| `civil` \| `nautical` \| `astronomical` | No limit, or the Sun must be below −6°, −12° or −18° | `none` |
| `@window` | `[start] .. [end]` | UTC time window, see below | open |
| `@duration` | `minutes` | Expected run time, 1–1440; all constraints must hold for this whole interval before the task starts | estimated, see [Task duration](#task-duration) |
| `@repeat` | `count` [`every minutes`] | Number of successful runs, 1–10000, with an optional pause between them | `1` |
| `@retry` | `count` [`after minutes`] | Number of reruns after a failure, 0–100, with an optional delay | `0` |
| `@preempted` | `retry` \| `fail` | What happens to the task when an urgent task preempts it | `retry` |
| `@enabled` | `yes` \| `no` | A disabled task is kept but never selected | `yes` |

The constraints `@altitude`, `@airmass`, `@ha`, `@moon` and `@illumination` require `@target`. A header that uses them without `@target` is invalid. A task with `@target` also requires the target above the horizon, with or without `@altitude`.

`@window` takes two endpoints, and either may be empty:

- `YYYY-MM-DD HH:MM` is an absolute UTC time.
- `HH:MM` alone is a nightly window that repeats every day. If `end` is earlier than `start`, the window crosses midnight.
- Both endpoints must use the same form.
- A nightly window and an open end never expire. An absolute end expires the task.

### Lorentzian moon avoidance

Without `width`, `@moon > D` requires a separation of at least D degrees.

With `width W`, the required separation depends on the moon age A in days, from 0 at new moon to 29.53:

```
limit = D / (1 + ((A − 14.77) / W)²)
```

The full limit D applies at full moon and falls off towards new moon. W is the half-width in days: W days before or after full moon, the limit is D / 2.

A fixed separation is too strict near new moon, when a thin crescent hardly brightens the sky, and too lax near full moon. The Lorentzian limit follows the brightness of the moon. A small W suits narrowband filters, which tolerate moonlight; a large W keeps the limit high for most of the month, which suits broadband imaging.

Example for `@moon > 40 width 7`:

| Moon | Days from full moon | Required separation |
|---|---|---|
| Full | 0 | 40° |
| Gibbous | 3 | 34° |
| First or last quarter | 7 | 20° |
| Crescent | 11 | 12° |
| New | 14.8 | 7° |

With or without `width`, the separation limit is ignored while the moon is below the horizon, because it does not brighten the sky then.

### Validation

Task text comes from clients and is untrusted input. Only the header is validated; the body is arbitrary JavaScript with the same trust level as `AGENT_SCRIPTING_RUN_SCRIPT`.

The agent validates the header on every upload and edit:

- `@task` is required and must be unique.
- Unknown keys, repeated keys, malformed values and out-of-range values are errors.
- The body must not be empty.
- The task text is limited to 16 KiB.
- At most 128 tasks are stored, the same limit as the Scripting Agent's scripts.

An invalid task is rejected. The property goes to the Alert state and the message names the line and the problem, for example `line 4: @altitude: expected '> degrees'`. The stored task list is not changed.

After an upload or an edit, the agent runs the body once to estimate its duration, see [Task duration](#task-duration). The estimate runs after the property change is answered, because it needs the Scripting Agent. A body that throws an error during the estimate, including a syntax error, is marked `SCRIPT ERROR`: the message names the error, the task list shows it with an Alert light, and the task is not scheduled until it is edited or reset. A script error during a run counts as a task failure. A body can tell the two apart through `scheduler_task.estimate`.

Uploading therefore executes the body up to its `start()` call. This is consistent with the trust level above: anyone who can upload a task can also run a script.

### Task duration

The scheduler needs the duration of a run to check the constraints before a start and to build the night plan. The effective duration is the first of these that is available:

1. `@duration` from the header, when the user sets it;
2. the **measured** duration of the last successful run of the same task text;
3. the **estimated** duration computed from the sequence;
4. 60 minutes, while no estimate is available.

Elsewhere in this document, `@duration` means this effective duration. The label of the task in `AGENT_SCHEDULER_TASK_STATE` shows the duration and its source: `set`, `measured`, `estimated` or `default`.

The duration is a planning value, not a limit. The scheduler never stops a run because it exceeds its duration; limits inside a run are the body's job (`break_at()`, `break_at_ha()`, `break_if_target_altitude_below()`). A limit enforced by the agent is the planned `@timeout` key, see [Implementation phases](#implementation-phases).

**Estimate in `Sequencer.js`.** The sequencer gets one addition, `Sequence.prototype.estimate()`. While a sequence is built, the builder methods add to counters on the `Sequence` object, as `capture_batch()` and `capture_stream()` already add to the total exposure. `estimate()` returns these counters:

| Field | Meaning |
|---|---|
| `exposure` | Total exposure time in seconds of all `capture_batch()` and `capture_stream()` steps |
| `frames` | Number of frames these steps capture |
| `operations` | Number of steps that move or calibrate equipment: `slew()`, `precise_goto()`, `sync_center()`, `focus()`, `calibrate_guiding()`, `start_guiding()`, `park()`, `unpark()`, `home()`, `dome_slew()`, `dome_park()`, `dome_unpark()`, `dome_open()` and `dome_close()` |
| `waits` | Sum of fixed `wait()` delays in seconds |
| `unknown` | `true` when the sequence contains a step whose duration cannot be known in advance: `wait_until()`, `wait_until_solar_altitude_below()` or `wait_until_target_altitude_above()` |

Steps inside `repeat()` are counted once per iteration, because `repeat()` runs its block for every iteration. The method only reports counts. Converting them to a duration with the agent's overhead settings remains the scheduler's job. Every builder method that is added later must update the counters it affects, and the sequencer documentation and tests cover `estimate()` like any other part of the `Sequence` API.

**Estimation.** The agent estimates a task when it is uploaded, edited, reset after a script error or loaded from the configuration. Estimates run one at a time, also while another task runs, because they never start a sequence. The agent loads `Sequencer.js` for them even when the scheduler is stopped. A task is not selected while its estimate is pending, so that script errors are found before the first run and the constraints are checked against the right duration.

1. The agent sends the body to the Scripting Agent in estimation mode, inside the wrapper script described in [Running a task](#running-a-task). In this mode, the wrapper temporarily replaces `Sequence.prototype.start()` with a function that calls `this.estimate()` and publishes the result instead of starting the sequence, and restores `start()` afterwards.
2. The result is published in the `SCHEDULER_ESTIMATE` property of the Scripting Agent. Its items are `TOKEN`, `EXPOSURE`, `FRAMES`, `OPERATIONS`, `WAITS` and `UNKNOWN`. An error in the body is reported in `SCHEDULER_RESULT` instead.
3. The agent matches the token and computes the duration:

   ```
   duration = exposure + frames * FRAME_OVERHEAD + operations * OPERATION_OVERHEAD + waits
   ```

   `FRAME_OVERHEAD` covers download, saving, dithering and settling of one frame. `OPERATION_OVERHEAD` is a typical time for one equipment step.

An estimation can run while another sequence is executing, because nothing is started.

**When no estimate is possible.** A task is marked unknown when:

- `estimate()` reports `unknown`;
- the loaded `Sequencer.js` is an older version without `estimate()`, for example one loaded by a client.

Its upload succeeds with a warning that recommends an explicit `@duration`. The agent uses the measured duration once a run has succeeded, and the 60-minute default until then.

**Measurement.** After every successful run, the agent records the actual time from start to end as the measured duration. Meridian flips, autofocus retries and slow downloads are not in the model, but they are part of the measured duration, so the scheduler learns the real duration after the first run. An edit of the task text discards the measured duration. Failed and preempted runs do not change it.

**Body requirement.** For estimation, the body runs up to its `start()` call, on every upload and edit. It must only build the sequence before `start()`: it must not change properties, set timers or send messages directly. Everything that acts on devices belongs in sequence steps. The agent cannot enforce this rule.

## Task states

| State | Meaning |
|---|---|
| QUEUED | Ready for selection |
| WAITING | Its constraints do not hold for the whole `@duration` now; the agent computes when they will |
| RUNNING | The task's sequence is executing |
| RETRY | Failed or preempted; returns to QUEUED after the retry delay |
| REPEAT | Completed a run with more runs to go; returns to QUEUED after the repeat interval |
| DONE | All runs completed |
| FAILED | Failed with no retries left, or preempted with `@preempted fail` |
| EXPIRED | The end of its `@window` passed before it completed |
| DISABLED | `@enabled no` or disabled by command |

```
             upload / RESET
                   |
                   v
   +-------> QUEUED <------> WAITING
   |            |               |
   |            +------+--------+
   |                   | selected
   |                   v
   |               RUNNING ---- success, runs left ----> REPEAT --+
   |                |  |  \                                       |
   |                |  |   +-- success, no runs left --> DONE     |
   |                |  |                                          |
   |                |  +-- failure, retries left --> RETRY ----+  |
   |                |  +-- preempted, @preempted retry --> RETRY  |
   |                |                                          |  |
   |                +-- failure, no retries left --> FAILED    |  |
   |                +-- preempted, @preempted fail --> FAILED  |  |
   |                                                           |  |
   +---------------- delay elapsed ----------------------------+--+

   QUEUED, WAITING, RETRY, REPEAT -- window end passed --> EXPIRED
   any state but RUNNING -- DISABLE --> DISABLED -- ENABLE --> previous state
   any state but RUNNING -- RESET --> QUEUED, counters cleared
```

Further rules:

- A preemption does not consume a retry, because the task did not fail on its own.
- A sequence that ends Ok counts as a successful run, even if it ended early. This includes a `break_if_target_altitude_below()` or `break_at()` without a resume point, and failures skipped by `continue_on_failure()`. A body that must not count a shortened run as a success has to end with a failing step.
- RESET of a RUNNING task is rejected.

## Agent properties

All properties belong to the device "Scheduler Agent". Task storage follows the Scripting Agent's pattern of `AGENT_SCRIPTING_ADD_SCRIPT` and `AGENT_SCRIPTING_SCRIPT_<n>`. Agent properties are standard INDIGO properties: their names are added to `indigo_names.h` and documented in `PROPERTIES.md`.

| Property | Type | Items | Purpose |
|---|---|---|---|
| `AGENT_SCHEDULER_ADD_TASK` | text | `TASK`, `STATUS` | Upload a task. The name is taken from `@task`. Creates `AGENT_SCHEDULER_TASK_<n>`. Clients leave `STATUS` empty; it is used only when the configuration is restored, see [Persistence](#persistence) |
| `AGENT_SCHEDULER_TASK_<n>` | text | `TASK` | One stored task, group "Tasks", label equal to the task name. Clients download a task by reading it and edit it by writing it |
| `AGENT_SCHEDULER_DELETE_TASK` | text | `NAME` | Delete a stored or urgent task. A running task is aborted first, as with `ABORT` |
| `AGENT_SCHEDULER_URGENT_TASK` | text | `TASK` | Submit a task as urgent, regardless of its `@priority` |
| `AGENT_SCHEDULER_TASK_STATE` | light | one per task | Idle = QUEUED, WAITING, RETRY, REPEAT or DISABLED; Busy = RUNNING; Ok = DONE; Alert = FAILED or EXPIRED. The label shows the name, state, completed runs and retries left; urgent tasks are marked as urgent |
| `AGENT_SCHEDULER_TASK_NAME` | text | `NAME` | Target of `AGENT_SCHEDULER_TASK_COMMAND` |
| `AGENT_SCHEDULER_TASK_COMMAND` | switch | `RESET`, `ENABLE`, `DISABLE`, `MOVE_UP`, `MOVE_DOWN` | Applied to the task named in `AGENT_SCHEDULER_TASK_NAME`. List order breaks score ties |
| `AGENT_SCHEDULER_CONTROL` | switch | `START`, `STOP`, `ABORT` | Scheduler control, see [Scheduler control](#scheduler-control) |
| `AGENT_SCHEDULER_STATE` | number | `DONE`, `FAILED`, `TOTAL` | Task counters. The property state is Idle when stopped, Busy while scheduling and Alert after `ABORT` or a failed `STARTUP` hook |
| `AGENT_SCHEDULER_CURRENT` | text | `CURRENT`, `NEXT`, `NEXT_START`, `REASON` | The running task, the task expected next, the UTC time it becomes eligible (`YYYY-MM-DDTHH:MM:SS`), and why nothing is eligible when the scheduler waits |
| `AGENT_SCHEDULER_PLAN` | light | one per planned run | The forecast of the rest of the night in expected run order, see [Night plan preview](#night-plan-preview) |
| `AGENT_SCHEDULER_HISTORY` | light | one per finished run | The record of finished runs, newest first, see [Run history](#run-history) |
| `AGENT_SCHEDULER_CLEAR_HISTORY` | switch | `CLEAR` | Remove all entries from `AGENT_SCHEDULER_HISTORY` |
| `AGENT_SCHEDULER_RESTORE_HISTORY` | text | `ENTRIES` | Used only to restore the run history from the configuration; published with an empty value, and changes from clients are rejected |
| `AGENT_SCHEDULER_SETTINGS` | number | see below | Scheduler settings |
| `AGENT_SCHEDULER_HOOKS` | text | `STARTUP`, `SHUTDOWN`, `IDLE` | Names of stored tasks used as hooks |

`AGENT_SCHEDULER_SETTINGS` items:

| Item | Unit | Default | Meaning |
|---|---|---|---|
| `IDLE_CHECK_INTERVAL` | s | 30 | Interval of constraint checks while waiting, and of urgent-task checks while a task runs |
| `IDLE_PARK_AFTER` | min | 30 | Time without an eligible task before the `IDLE` hook runs |
| `STOP_TIMEOUT` | s | 60 | Time allowed for an aborted sequence and its agents to stop |
| `START_TIMEOUT` | s | 10 | Time allowed for a started task's sequence to become Busy |
| `SETTING_WEIGHT` | — | 1 | Weight of the setting-soonest score term, 0–4 |
| `ALTITUDE_WEIGHT` | — | 1 | Weight of the altitude score term, 0–4 |
| `FRAME_OVERHEAD` | s | 10 | Time added per captured frame in the duration estimate |
| `OPERATION_OVERHEAD` | s | 120 | Time added per slew, centering, focus, guiding start or similar step in the duration estimate |

**Requests.** Every change of a writable property is queued. The agent's handler acknowledges it with the property in the Busy state and completes it with Ok or Alert. Only the handler changes the properties; client enumerations receive copies. INDIGO does not publish an update that changes neither values nor state, so without the Busy state a client, and the configuration restore, would not see the result.

`AGENT_SCHEDULER_TASK_NAME` and `AGENT_SCHEDULER_TASK_COMMAND` form a two-step command, which is a common INDIGO idiom. Two clients that issue commands at the same moment can interfere. If both properties are changed in one client action, the command applies to the name sent with it.

**Labels and list properties.** INDIGO item labels are limited to 511 bytes, and clients show them in narrow lists. Task names are therefore limited to 40 bytes. The labels of `AGENT_SCHEDULER_TASK_STATE`, `AGENT_SCHEDULER_PLAN` and `AGENT_SCHEDULER_HISTORY` are cut to fit without splitting a UTF-8 character, and the full text, such as a long failure reason, is sent as the message of the update. The item names are internal identifiers (`TASK_<n>`, `PLAN_<n>`, `RUN_<n>`), not task names.

Changing the number of items of a list property requires deleting and defining it again, which clients see as the list being rebuilt. The agent therefore redefines a list only when its content has changed, and updates it in place when only states or labels change.

### Editing tasks

- An edit is validated like an upload. An invalid edit leaves the stored task unchanged.
- An edit may change `@task`, which renames the task, as long as the new name is unique.
- An edit keeps the task's state and counters. To run a completed, failed or expired task again, the client uses `RESET`.
- An edit of a running task takes effect after the task ends.
- A hook that names a deleted or renamed task is ignored, and the agent sends a warning message when the hook is due.

### Enabling and disabling

A task is selectable only when its header has `@enabled yes` and it is not disabled by the `DISABLE` command. `ENABLE` clears only the command flag. A task disabled in its header stays disabled until the header is edited.

### Urgent tasks

A task submitted through `AGENT_SCHEDULER_URGENT_TASK` is not stored:

- It appears in `AGENT_SCHEDULER_TASK_STATE` while it is pending or running.
- It can be cancelled by name with `AGENT_SCHEDULER_DELETE_TASK`.
- It is removed when it ends or expires, and it is not persisted, so a server restart drops it.

A stored task with `@priority urgent` behaves the same way while it is pending, but it remains stored and follows the normal state rules.

An urgent task submitted while the scheduler is stopped is accepted and queued. It runs when the scheduler is started. The agent sends a message that the scheduler is not running.

### Task store

In memory, the agent keeps a table of up to 128 task slots. A slot holds:

- the task text exactly as uploaded;
- the parsed and validated header;
- the runtime status: state, completed runs, retries used, the command-disabled flag, the time of the next possible start, and the submission order of urgent tasks.

Each stored task occupies slot `<n>` and is published as the property `AGENT_SCHEDULER_TASK_<n>`, whose `TASK` item holds the text. The slot number is only a handle: it is reused after a delete and is not stable across restarts. Clients identify tasks by name.

The list order used for tie-breaking and for `MOVE_UP` and `MOVE_DOWN` is a separate ordered list of slots. It is also the order of the items in `AGENT_SCHEDULER_TASK_STATE`.

Urgent tasks submitted through `AGENT_SCHEDULER_URGENT_TASK` use the same slots, but they get no `AGENT_SCHEDULER_TASK_<n>` property and are never saved.

### Persistence

The agent persists its tasks in its own INDIGO configuration file, the same way as the Scripting Agent persists its scripts. It uses the standard `indigo_save_property()` mechanism and does not open any other file.

The configuration file is `Scheduler_Agent.config` in the INDIGO configuration folder: `~/.indigo` on Linux and macOS, and the INDIGO folder under `%APPDATA%` on Windows. The usual profile and port suffixes apply.

Two rules of the INDIGO configuration restore shape the format:

- `indigo_save_property()` does not save hidden or read-only properties, and light properties are never restored.
- The restore replays only properties the device defines. It looks each saved property up first and skips it if the device does not define it, and it waits for the device to answer each replayed change with an update.

Everything the agent restores therefore goes through a defined, writable text property, and every replayed change is answered with an update.

**Saving.** The agent rewrites the whole file after a change that affects persisted data:

- a task added, edited or deleted;
- a `TASK_COMMAND`;
- a change of `SETTINGS` or `HOOKS`;
- a task state transition or a new history entry;
- a history clear.

Changes that come in quick succession are written once, two seconds after the last of them. Nothing is saved while the configuration is being restored. The file contains, in this order:

1. one `AGENT_SCHEDULER_ADD_TASK` change per stored task, in list order. The agent saves a temporary copy of the property with two items:
   - `TASK`, the task text;
   - `STATUS`, the runtime status in a compact form, for example `state=DONE runs=2 retries=0 disabled=0 measured=3540`.
2. `AGENT_SCHEDULER_SETTINGS` and `AGENT_SCHEDULER_HOOKS`;
3. one `AGENT_SCHEDULER_RESTORE_HISTORY` change. The agent saves a temporary copy whose `ENTRIES` item holds all history entries, one line per entry, oldest first. Each line holds the task name, run number, start, end, result, a mark (`H` for a hook, `U` for an urgent task, `-` for neither) and the reason, separated by tabs. The configuration reader trims trailing whitespace, so no field is saved as a space, and entries whose trailing empty fields were trimmed are still restored.

With a task text of at most 16 KiB and 128 tasks, the file stays below about 2 MiB.

**Loading.** The agent loads its configuration in its attach callback, after its task store is initialised, the same way as the Imager Agent does. Each saved change is replayed through the agent's change handler, with the configuration reader as the client:

- An `AGENT_SCHEDULER_ADD_TASK` change validates the header and creates the task, as an upload does. A non-empty `STATUS` restores the task's state and counters. `STATUS` from any other client is ignored.
- A replayed task that fails validation, for example after a grammar change in a newer version, is skipped. The agent sends a message naming the task, and the rest of the configuration still loads.
- An `AGENT_SCHEDULER_RESTORE_HISTORY` change rebuilds the history.
- The duration estimates of the restored tasks are requested when the Scripting Agent appears.

**After a restart:**

- Finished work is not repeated: DONE, FAILED and EXPIRED tasks keep their state, and the counters are kept.
- A task that was RUNNING when the server stopped is restored as RETRY without consuming a retry.
- WAITING tasks are restored as QUEUED and re-evaluated.
- Pending urgent tasks submitted through `AGENT_SCHEDULER_URGENT_TASK` are lost. A stored task with `@priority urgent` is kept.
- The scheduler itself always starts stopped. Scheduling resumes only after `START`.

**Moving tasks between systems.** Clients export a task by reading `AGENT_SCHEDULER_TASK_<n>.TASK` and saving the text to a file, and import one by sending a file's text to `AGENT_SCHEDULER_ADD_TASK`. The configuration file is internal to the agent and is not an exchange format.

## Scheduling

All scheduler work runs on the agent's handler queue, so it is serialised without a lock. Property changes are copied and queued, and the agent never sends a request to another agent while holding a lock. This matters because the Scripting Agent calls into other agents while holding its JavaScript lock. Updates from other agents, such as `SEQUENCE_STATE`, arrive on client threads and are recorded under a small mutex of their own.

### Ephemeris grid

Eligibility checks over whole task durations, the setting-soonest score and the night plan all evaluate the sky at many times. Computing the Moon's position at every one of them would be too slow on small computers such as a Raspberry Pi.

The constraint engine therefore works on a grid:

- At every refresh, it computes the Sun and Moon altitudes, the moon illumination and the local sidereal time for the next 48 hours at 5-minute steps (577 points). The 48 hours cover a 24-hour task duration and the 12-hour look-ahead of the score.
- The Sun and Moon positions themselves are computed once per hour and interpolated linearly between. The Moon moves about half a degree per hour, so the interpolation error is far below any limit, while the grid builds in less than 0.1 s.
- For each task with `@target`, it computes the target's altitude, airmass and hour angle on the same grid. These values only change when the task is edited, apart from the precession to the date, which is negligible within a night.
- For each task, it also records for every grid point the first point at or after it where a sky constraint fails, with the reason. An interval check is then a single lookup, and the search for the next start skips over a failing stretch in one step instead of testing every start time. The time windows are checked separately, because a nightly window does not depend on the grid.
- Eligibility, `NEXT_START`, the setting-soonest term and the night plan are lookups in these tables.

The grid and the task tables are rebuilt when the site coordinates change and every hour. A task's tables are also rebuilt when it is edited.


### Scheduler control

- `START` runs the `STARTUP` hook, then begins scheduling. If the hook fails, the scheduler stops and its state becomes Alert.
- `STOP` lets the running task finish, then runs the `SHUTDOWN` hook and stops.
- `ABORT` aborts the running task as described in [Aborting a sequence](#aborting-a-sequence) and marks it RETRY without consuming a retry. It then runs the `SHUTDOWN` hook and stops.
- When there is at least one task and every task is DONE, FAILED, EXPIRED, DISABLED or has a script error, the scheduler runs the `SHUTDOWN` hook and stops. With no tasks at all it keeps running, ready for urgent tasks. A task added or reset before the shutdown hook of this automatic stop starts cancels the stop.

### Hooks

The `STARTUP`, `SHUTDOWN` and `IDLE` hooks name stored tasks.

- A hook task runs outside the state machine. Its constraints, state and counters are not evaluated or changed, so it can run every night. The task list shows it as `STARTUP HOOK`, `SHUTDOWN HOOK` or `IDLE HOOK`.
- A task named as a hook is never selected as an ordinary task and does not count when deciding that all tasks have finished.
- Hook runs appear in the run history, marked as hooks.
- A hook that fails is reported by a message. A failed `STARTUP` hook stops the scheduler with Alert; a failed `SHUTDOWN` hook still stops it, with Alert.
- The `SHUTDOWN` hook runs once for every stop: `STOP` after the running task, `ABORT` after the running task or startup hook was aborted, and the automatic stop when all tasks finished. It does not run after a `STOP_TIMEOUT`, because the old sequence still occupies the Scripting Agent. When it cannot run because a sequence started by hand occupies the Scripting Agent or `Sequencer.js` is missing, the agent says so in a message and stops without it.
- `START` while the `SHUTDOWN` hook runs cancels the stop: after the hook, the `STARTUP` hook runs again and scheduling continues.
- A hook runs whatever the state of its task; an edit of a hook while it runs takes effect after it ends.
- The `IDLE` hook runs once per idle period, `IDLE_PARK_AFTER` minutes after the scheduler last found nothing eligible. A task that starts begins a new idle period.
- A hook that names no task is reported by a message and skipped.

### When a decision is made

While no task is running, the agent selects the next task:

- after the previous task ends;
- after a task is uploaded, edited, enabled or reset;
- on `START`;
- every `IDLE_CHECK_INTERVAL` while it waits for constraints.

While a task is running, the agent checks every `IDLE_CHECK_INTERVAL` only whether an urgent task is pending and eligible; see [Urgent preemption](#urgent-preemption). It makes no other scheduling decisions. Limits inside a task, such as the target setting below the horizon, are the task body's job, through `Sequencer.js` steps like `break_if_target_altitude_below()` and `break_at()`.

### Selection

1. **Delays.** RETRY and REPEAT tasks whose delay has elapsed return to QUEUED.
2. **Expiry.** Tasks whose absolute `@window` end has passed become EXPIRED, also when some of their `@repeat` runs completed. The task list and the run history show how many runs succeeded.
3. **Eligibility.** A QUEUED or WAITING task is eligible when all its constraints hold over the whole interval from now to now + `@duration`. The interval is sampled every five minutes and at its end. A task that is not eligible becomes WAITING, and the agent computes when it would become eligible for `NEXT_START`.
4. **Urgent tasks first.** Eligible urgent tasks run in the order they were submitted.
5. **Score.** Other eligible tasks are ranked by:

   ```
   score = class_base + level + SETTING_WEIGHT * setting + ALTITUDE_WEIGHT * altitude
   ```

   - `class_base` is 2000 for `high`, 1000 for `normal` and 0 for `low`. With weights of at most 4, the other terms add at most 900. Any high task therefore outranks any normal task.
   - `level` is the `@priority` level, 0–100.
   - `setting` is `100 * (1 − min(T, 12 h) / 12 h)`, where T is the time until the task's constraints stop holding. It favours tasks that become unobservable soonest.
   - `altitude` is `100 * altitude / 90` at the current time, or 0 without `@target`.

   Ties go to the task higher in the list.
6. **Run.** The agent starts the best task and waits until it ends.
   - Success: the run counter increases. The task becomes DONE or REPEAT.
   - Failure: the task becomes RETRY while retries remain, otherwise FAILED.
7. **Nothing eligible.** The agent publishes `NEXT_START` and `REASON`. After `IDLE_PARK_AFTER` minutes it runs the `IDLE` hook, for example to park the mount and close the dome.

### Night plan preview

`AGENT_SCHEDULER_TASK_STATE` lists the tasks in list order, which only breaks score ties. `AGENT_SCHEDULER_CURRENT` shows only the running and the next task. To show the user the expected order of the whole night, the agent publishes a forecast in `AGENT_SCHEDULER_PLAN`.

**Computation.** The agent simulates scheduling from now until the end of the night:

- At night, the night ends at the next sunrise. During the day, it ends at the sunrise that follows the next sunset, so that the plan shows the coming night. It ends 24 hours from now if the sun does not rise or set within the grid.
- The simulation starts with the running task, assumed to end at its start time plus its [duration](#task-duration), but not earlier than one minute from now.
- At each simulated time, the agent applies the [selection](#selection) rules exactly as at run time: delays, expiry, eligibility over the task duration, urgent tasks first, then score. A task queued again after a repeat or retry delay waits behind the urgent tasks queued before it.
- The selected task is assumed to run for its duration and succeed. Its simulated run counter increases, and `@repeat` intervals are applied.
- When nothing is eligible, the simulation advances to the time the next task becomes eligible.
- A run that starts before the end of the night is planned even when it lasts beyond it.

The simulation works on a copy of the runtime status and never changes the real task states. It uses only ephemeris arithmetic and runs on the agent's handler queue.

**Items.** Each planned run is one item, in expected run order:

- The label shows the expected UTC start and end, the task name and its priority, for example `20:15–23:15  M31 LRGB (normal 50)`. A task that repeats appears once per planned run, with the run number.
- The light is Busy for the running task and Idle for future runs.
- Tasks that cannot run before the end of the night follow the planned runs as Alert items, with the reason, for example `M42 — target below the altitude limit`, `NGC 7000 — disabled` or `M33 — no time left tonight`, the last for a task that could run but finds no room after the planned runs. DONE, FAILED and EXPIRED tasks are not listed.

**Updates.** The plan is recomputed:

- whenever a task starts or ends;
- after a task is uploaded, edited or deleted, a `TASK_COMMAND`, an urgent submission and a `SETTINGS` change;
- on `START`, `STOP` and `ABORT`;
- when the site coordinates change;
- every `IDLE_CHECK_INTERVAL`, so that the times follow the clock.

It is also published while the scheduler is stopped, as the plan that `START` would follow.

**Limits.** The plan is a forecast, not a commitment:

- Failures, retries, tasks that end earlier or later than their duration, urgent tasks and safety stops change the real order. The plan is recomputed when they happen.
- The forecast assumes that the constraints and the site do not change.
- Urgent preemption is not simulated: an urgent task that becomes eligible during a planned run is listed after that run, although it will preempt it.
- A task whose duration is still being estimated is planned with the duration known so far, although the scheduler does not start it before the estimate is known.
- While nothing runs, the first run starts now, so the labels change every minute and the property is redefined with them. INDIGO cannot change an item label with an update.

### Run history

`AGENT_SCHEDULER_TASK_STATE` shows only the current state of each task. To show what actually happened, the agent keeps a record of finished runs in `AGENT_SCHEDULER_HISTORY`.

**Entries.** An entry is added whenever a run ends, including hook runs and urgent tasks:

| Result | Light | Meaning |
|---|---|---|
| `succeeded` | Ok | The sequence ended Ok |
| `failed` | Alert | The sequence ended Alert, or the task failed to start |
| `preempted` | Busy | An urgent task interrupted the run |
| `aborted` | Busy | `ABORT`, a delete of the running task, or a `STOP_TIMEOUT` stop |

The Busy light is used for interrupted runs, so that they stand out from both successes and failures.

The item label shows:

- the UTC start and end times;
- the task name, with the run number and a hook or urgent mark where it applies;
- the result;
- for a failure, the reason: the last alert message of the sequence, the script error message, or `not started`.

For example: `2026-11-12 20:15–21:02  M31 LRGB #1  preempted: by SN 2026abc`.

Entries are newest first. Expiry is not a run, so an expired task gets no entry; its state remains visible in `AGENT_SCHEDULER_TASK_STATE`.

**Limits.**

- The history keeps the last 200 entries. Older entries are dropped.
- It survives a restart: entries are saved in the configuration and replayed on load, see [Persistence](#persistence).
- Deleting or resetting a task does not remove its entries.
- `AGENT_SCHEDULER_CLEAR_HISTORY` removes all entries.

### State between tasks

The scheduler does not reset the agents between tasks. Settings made by one task body persist into the next one. Examples are the meridian flip, which stays enabled after a sequence ends, HA limits, filter offsets and the solver filter.

Each task body must therefore set up the state it relies on. In particular, every task should call `enable_meridian_flip()` or `disable_meridian_flip()` explicitly, so that a flip enabled by a preempted task does not fire during an urgent task.

## Urgent preemption

An urgent task is one submitted through `AGENT_SCHEDULER_URGENT_TASK`, or a stored task with `@priority urgent`.

1. The agent queues a handler immediately on submission, and checks pending urgent tasks every `IDLE_CHECK_INTERVAL` while a task runs.
2. An urgent task preempts only if it is **eligible now** over its whole `@duration`. Otherwise it waits like any other task, and the running task is not touched.
3. If no task is running, or the running task is itself urgent, the eligible urgent task waits at the head of the queue. Urgent tasks never preempt each other.
4. If a non-urgent task is running, the agent aborts it as described in [Aborting a sequence](#aborting-a-sequence).
5. The agent marks the preempted task RETRY or FAILED according to its `@preempted` key.
6. The agent starts the urgent task. The urgent task performs its own slew, centering, guiding and capture.
7. When the urgent task ends, normal selection resumes.

An urgent task never bypasses its own constraints. An urgent task below `@altitude` waits until it rises or its window ends.

Frames that the preempted task saved stay on disk. The scheduler does not count them, and a rerun captures its full sequence again.

A future safety alert outranks urgent tasks: it aborts everything and runs the `SHUTDOWN` hook.

### Aborting a sequence

Preemption, `ABORT` and deleting a running task all stop the sequence the same way:

1. Set the Scripting Agent's `AGENT_ABORT_PROCESS.ABORT`. The sequencer aborts the Imager, Mount and solver agents.
2. Abort the related Guider Agent explicitly. The sequencer's abort intentionally leaves active guiding running.
3. Wait, for at most `STOP_TIMEOUT` seconds, until:
   - `SEQUENCE_STATE` is no longer Busy;
   - the `AGENT_START_PROCESS` property of the related Imager, Mount, Guider and solver agents is no longer Busy.
4. If the sequence or an agent is still Busy after `STOP_TIMEOUT`, the agent sends an alert message. It does not start another task: the Scripting Agent refuses a new sequence while the old one runs. The scheduler stops, and its state becomes Alert.

## Running a task

`Sequencer.js` is compiled into the Scheduler Agent from `indigo_drivers/agent_scripting/library/Sequencer.js`. It uses the same hexdump build rule as the Scripting Agent's `boot.js`. The generated `.dat` file is checked in and gets an Xcode build phase, as `boot.js.dat` does.

The scheduler starts no task until the Scripting Agent is present. When the Scripting Agent does not yet define `SEQUENCE_STATE`, the scheduler loads the library into it through `AGENT_SCRIPTING_RUN_SCRIPT`. A library that a client loaded already, possibly of another version, is used as it is.

**Library reloads.** Clients such as the web UI send the whole library again before every script they run. A reload keeps the `indigo_sequencer` object while it has a sequence running, and the flipper while a flip is enabled; only an idle sequencer is replaced. A client that runs a script during a scheduled task therefore does not disturb it.

**Wrapper script.** To estimate or run a task, the agent sends one wrapper script through `AGENT_SCRIPTING_RUN_SCRIPT`. The wrapper:

- defines `scheduler_task` with a unique token, see [The `scheduler_task` object](#the-scheduler_task-object);
- carries the task text as a JSON-encoded string, and evaluates it with `eval()` inside a function, so that the body's variables do not collide with other scripts in the shared interpreter;
- catches every error, including syntax errors;
- in estimation mode, replaces `Sequence.prototype.start()` as described in [Task duration](#task-duration);
- reports the outcome in the `SCHEDULER_RESULT` property of the Scripting Agent, with the items `TOKEN`, `RESULT` (`ok` or `error`) and `MESSAGE`.

The agent relies on its token, not on the state of `AGENT_SCRIPTING_RUN_SCRIPT`. That property is shared by all clients, so its state cannot tell whose script failed.

**Result of a run.** The agent watches:

- `SCHEDULER_RESULT` with its token: `error` means the task failed to start, and `MESSAGE` becomes the failure reason in the run history.
- An `ok` result together with a transition of `SEQUENCE_STATE` from not Busy to Busy after the wrapper was sent marks the start. The agent counts the transitions, so a sequence that ends quickly is still recognised. If no start is observed within `START_TIMEOUT` seconds, the start fails, for example because the body never calls `start()` or because another sequence is running.
- After the start, Busy → Ok means success.
- Busy → Alert means failure, unless the agent itself aborted the sequence.

A failed start counts as a task failure.

A sequence started by hand, outside the scheduler, keeps `SEQUENCE_STATE` Busy. The scheduler does not start a task while such a sequence runs. An eligible urgent task aborts it like a preempted task.

## Implementation phases

1. This design document.
2. Scheduler Agent core in `indigo_drivers/agent_scheduler/`, built in this order:
   1. header parser and validation;
   2. task store, properties and persistence;
   3. wrapper script, run, abort and urgent preemption, including the `Sequencer.js` change that keeps a running sequence on a library reload. This is the riskiest part and gets the most extensive tests;
   4. ephemeris grid and constraint engine;
   5. selection and scheduler control.

   The agent is registered in the server, the build and the project files, and documented in `PROPERTIES.md`.
3. Phase 3, in two parts:
   - 3a: startup, shutdown and idle hooks, run history, duration estimate and measurement, including `Sequence.prototype.estimate()` in `Sequencer.js` with its documentation in `Sequencer.md` and its tests. Scoring weights, repeat and retry and the Lorentzian moon limit came with phase 2.
   - 3b: night plan preview, with the per-task constraint tables that make the next-start search cheap.
4. Related Scripting Agent: the Scheduler Agent becomes a filter agent with `FILTER_RELATED_AGENT_LIST`, in which the user selects the Scripting Agent that runs the tasks, also one on another server. It is the only agent selected there. The default is the local "Scripting Agent", so existing setups do not change. Running tasks, estimating durations, loading `Sequencer.js`, aborting sequences and the `SEQUENCE_STATE` and result tracking use the selected agent. Changing the selection while a task runs is rejected.
5. Safety: a related AUX safety or weather device, selected through the filter agent's AUX device list. A warning prevents new tasks from starting. An alert aborts and runs the `SHUTDOWN` hook. Both have a debounce delay and a grace period.
6. Optional extensions:
   - `@timeout minutes`: a hard limit on the length of one run, independent of `@duration`, which stays a planning value. When a run exceeds it, the agent aborts the run the same way as `ABORT` (the sequence, then the Guider Agent, waiting at most `STOP_TIMEOUT`) and counts it as a failure: the history records `timed out`, and `@retry` decides whether the task runs again. Hook tasks and urgent tasks obey it too. Without the key, runs are not limited, as now;
   - task upload, download and editing in the web UI;
   - `@window` times in a local time zone;
   - image grading;
   - mosaics;
   - a GCN/VOEvent bridge that writes `AGENT_SCHEDULER_URGENT_TASK`.

## Testing

- Unit tests for the header parser cover:
  - every key and its defaults;
  - case-insensitivity, whitespace, CRLF and byte-order mark handling;
  - every invalid input.
- Unit tests for the constraint engine use fixed dates and sites with known sun, moon and target positions, including the Lorentzian limit, a polar site and nightly windows across midnight.
- Unit tests compare the per-task constraint tables with a direct evaluation of the constraints for random tasks and times, including the failure reasons.
- Unit tests for the night plan simulation cover the score, the order and times, ties, urgent tasks first, the running task first, repeats with intervals, unschedulable tasks, windows, the end of the night and the run limit.
- A performance test computes the night plan for 128 tasks and checks that it completes within one second on a Raspberry Pi 4.
- Integration tests run the Scheduler Agent with the Scripting Agent and stand-in Mount and Guider Agents; a server acceptance test runs it in `indigo_server` with the Imager, Mount and Guider Agents and the CCD and mount simulators. Together they cover:
  - upload, download, edit, rename and persistence across restarts, including restored counters, measured durations and history, and a saved task that no longer validates;
  - a body that throws or has a syntax error being marked `SCRIPT ERROR` by its estimate and never run;
  - a task name containing quotes and backslashes reaching the body unchanged;
  - a client loading `Sequencer.js` and running a script while a scheduled sequence runs, without disturbing it;
  - task ordering, and a task uploaded during a run not interrupting it;
  - a task waiting for its window, and a task not started when its duration does not fit;
  - urgent preemption, with the simulators for both `@preempted retry` and `@preempted fail`: the exposure is aborted, guiding stops, the agents become idle, the urgent task runs, and the preempted task becomes RETRY and later reruns, or becomes FAILED and never runs again;
  - an urgent task inserted while one task runs and another of higher priority, uploaded during the run, waits: the urgent task runs first and is shown running in the night plan; then the normal rules apply, so the waiting task runs before the retried one, and a failed task leaves the plan and never runs again; the preemption uses no retry;
  - an urgent task that is not yet eligible not preempting the running task;
  - a second urgent task waiting for the first;
  - an urgent task submitted while the scheduler is stopped;
  - expiry, including a repeating task with some runs completed;
  - the night plan preview: expected order and times of several tasks with different windows and priorities, repeats, unschedulable tasks with reasons, and recomputation after an upload, an urgent submission and a task ending early;
  - `Sequence.prototype.estimate()` in the sequencer tests: every counted builder, `repeat()` and the `unknown` flag;
  - the duration: estimates for bodies with `repeat()`, captures, operations and fixed waits; `UNKNOWN` for time and sky waits and for a loaded `Sequencer.js` without `estimate()`; a set `@duration` taking precedence; the measured duration replacing the estimate after a successful run and being discarded after an edit; no sequence started during estimation, also while another sequence runs;
  - the run history: one entry per finished run with the correct result and reason for success, failure, failed start, preemption and abort, the entry limit, persistence across restarts and clearing;
  - labels cut to 127 bytes, and list properties redefined only when their content changes;
  - a body that fails only when it runs, and a body without `start()`, counting as failures;
  - a `STOP_TIMEOUT` expiry stopping the scheduler without starting another task.
