# INDIGO Scheduler

Revision: 19.11.2026

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

This document is the design of the INDIGO Scheduler Agent. The agent decides which imaging task runs next, when it runs and under which sky conditions, and it can interrupt the running task for an urgent one. It is a design document. The agent is implemented as described here, except for the parts marked as planned, see [Implementation phases](#implementation-phases). How to use the scheduler is described in the [Scheduler User Guide](../../../indigo_docs/INDIGO_SCHEDULER_GUIDE.md).

## Motivation

INDIGO already runs imaging sequences. `Sequencer.js` executes a sequence inside the Scripting Agent (`indigo_drivers/agent_scripting/library/Sequencer.js`). It handles device selection, slewing, centering, focusing, guiding, capture and meridian flips. Only one sequence runs at a time, and the client starts it by hand.

The following are missing:

- a list of tasks that a client can create, store, download and edit;
- automatic selection of the next task from sky constraints and priorities;
- a way to stop the running task for an urgent target such as a supernova, a GRB alert or a new comet.

## Survey of existing schedulers

The last column shows the INDIGO scheduler for comparison.

| Feature | Ekos | SGP | N.I.N.A. Target Scheduler | Voyager RoboTarget | ACP Scheduler | INDIGO |
|---|---|---|---|---|---|---|
| Job unit | target + `.esq` sequence, job list `.esl` | target with events, sequence file | project → target → exposure plan (database) | set → target → shots | project → plan → observation (plan text files) | task text: `// @key` header + `Sequencer.js` body, stored in agent properties |
| Selection | greedy, re-planned every second | list order | weighted score, re-planned per step | priority order | weighted efficiency score | score, selected only between tasks; a switch between COMPACT (greedy, start the best task now) and VISIBILITY (a night plan that places the runs nearer their culmination without planning fewer runs than COMPACT) |
| Priority | row order | none | one weighted rule | Very Low … First | per-user, normalized | urgent, high, normal, low class + level 0–100 |
| Constraints | altitude, horizon, moon separation, twilight, weather | start/end time or altitude | altitude, horizon, meridian window, moon (Lorentzian), twilight per filter, minimum time span | altitude, HA window, moon phase and distance, Lorentzian | air mass, horizon, moon, dark time, checked at start and end | altitude, airmass, HA, moon separation (Lorentzian), moon illumination, twilight, time window, checked over the whole duration |
| Completion | repeat N, until a time, loop | events done or end time | accepted frames reached | shots done | plan done, "best efforts" | sequence ends Ok; repeat N with interval |
| Preempts a running job | yes (greedy) | no | no (at boundaries only) | no | by priority no; Target of Opportunity (ToO) API yes | urgent tasks only; the preempted task is retried or failed |
| Weather and safety | warning suspends, alert parks | safety monitor and recovery | N.I.N.A. triggers | yes | weather script, failed plan re-queued | unsafe hold from the Auxiliary Agent's safety: everything is aborted, the `UNSAFE` hook runs, the `SAFE` hook prepares again after a safe delay |
| Night start and end | startup and shutdown procedures; a per-job twilight limit aborts jobs at dawn; preemptive shutdown when the next job is hours away, startup again before it | end-of-sequence options (warm up, park); no nightly cycle | start and end areas of a sequence run each night, waits until dusk or dawn; Before Wait / After Wait instructions around idle waits | DragScript waits for dusk or astronomical night and loops until the night ends | `StartupObs` every evening, `ShutdownObs` every morning, at a Sun-down altitude (−12° by default); runs 24/7 | startup, shutdown and idle hooks; night start, night end and resume hooks at a scheduler-wide twilight limit, see [Night hooks](#night-hooks) |
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
- night start, night end and resume hooks for unattended runs over several nights, run every evening and morning at a twilight limit (ACP model);
- an unsafe hold driven by the safety of the Auxiliary Agent, with `UNSAFE` and `SAFE` hooks and a hysteresis before it ends.

It deliberately leaves out continuous greedy preemption, anti-thrashing settings and frame-level resume. Image grading and mosaics are possible later extensions.

## Design principles

- **Tasks are not preemptible, except by urgent tasks.** Only an urgent task interrupts a running non-urgent task. All other scheduling decisions are made between tasks.
- **A preempted task is failed or re-queued.** The task's header chooses which. A re-queued task reruns its sequence from the beginning.
- **A task is one human-readable text.** It consists of a header with the scheduling fields followed by an ordinary `Sequencer.js` body.
- **The agent stores tasks like the Scripting Agent stores scripts.** Tasks live in agent properties and in the agent's INDIGO configuration. The agent never reads or writes task files; clients load, save and edit files and transfer them through properties.
- **Device control stays in `Sequencer.js`.** The scheduler does not duplicate any device logic. It requires three small additions to `Sequencer.js`: a load that does not replace a running sequence, see [Running a task](#running-a-task), the `estimate()` method, see [Task duration](#task-duration), and the `SEQUENCE_FAILURE` property, which gives the reason of a failed run, see [Running a task](#running-a-task). The only device actions the scheduler performs itself are aborts and the labelling of the saved data, see [Observation data](#observation-data).

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
  - `indigocat_sun_equatorial_coords()` and `indigocat_moon_equatorial_coords_coarse()` (`indigocat/indigocat_solar_system.h`) give the Sun and Moon positions in J2000, which the agent precesses to the equinox of the evaluated time with `indigo_precess()`. The coarse Moon call truncates the ELP2000/82 series at 1e-6 rad per term; it stays within about 3.5 arcsec of the full theory and is about 50 times faster.

  No moon phase helper exists yet. The moon illumination and the number of days from full moon are derived from the Sun–Moon elongation. The elongation does not distinguish a waxing from a waning moon, which the constraints do not need, because the Lorentzian limit is symmetric about full moon.
- Any client can submit an urgent task with a single property change. This includes the INDIGO clients, MCP clients and a future GCN/VOEvent bridge.

The Scheduler Agent is a device with its own client, like the Scripting Agent. It does not select hardware devices, so it is not a filter agent.

### Agent dependencies

The Scheduler Agent depends directly on these agents, found by their exact device names:

| Agent | Needed | Properties the scheduler uses | Without it |
|---|---|---|---|
| "Scripting Agent" | always | changes `AGENT_SCRIPTING_RUN_SCRIPT` to load `Sequencer.js`, estimate durations and run tasks; observes `SEQUENCE_STATE`, `SEQUENCE_FAILURE`, and the `SCHEDULER_RESULT` and `SCHEDULER_ESTIMATE` properties that the wrapper script defines there; changes `AGENT_ABORT_PROCESS` to abort a sequence | no task starts and no duration is estimated; `REASON` is `waiting for the Scripting Agent` |
| "Mount Agent" | for tasks with `@target` or `@twilight` | observes `GEOGRAPHIC_COORDINATES` for the site; observes `AGENT_START_PROCESS` to wait until the agent is idle after an abort | these tasks wait with `site coordinates unknown`; tasks with only time windows still run |
| "Guider Agent" | optional | changes `AGENT_ABORT_PROCESS` on `ABORT` and preemption, because the sequencer's abort leaves guiding running; observes `AGENT_START_PROCESS` | nothing to stop |
| "Imager Agent", "Astrometry Agent", "Platesolver Agent" | optional | observe `AGENT_START_PROCESS`, to wait until they are idle after an abort before the next task starts | nothing to wait for |
| "Auxiliary Agent" | for the weather and safety: the unsafe hold and the `UNSAFE` and `SAFE` hooks | observes the light `AGENT_AUX_SAFETY`, item `SAFE`: Ok is safe, Alert is unsafe, Idle is no information | no safety information, which counts as safe by default or as unsafe with `AGENT_SCHEDULER_SAFETY_MISSING.UNSAFE`, see [Unsafe hold](#unsafe-hold) |

An unconfigured Mount Agent reports 0° latitude and 0° longitude, which the scheduler cannot tell from a real site, so the site must be configured in the Mount Agent: set directly with the default `AGENT_SITE_DATA_SOURCE` `HOST`, or taken from the mount, dome or GPS selected there. The sky tables are rebuilt with a new site at the next selection check.

The task bodies depend on more: the agents and devices their `Sequencer.js` steps use, by default the "Imager Agent", "Mount Agent" and "Guider Agent" and a plate solver agent selected for the Imager Agent. The scheduler does not check these; a missing one makes the run fail. Weather and other safety conditions reach the scheduler only through the Auxiliary Agent's `AGENT_AUX_SAFETY`, see [Unsafe hold](#unsafe-hold).

For the native Platesolver Agent used by a sequence, see the [native INDIGO solver reference](../../agent_platesolver/doc/PLATESOLVER.md).

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

String values are JSON-encoded, so that quotes or backslashes in a task name cannot break or alter the script. The body uses these values, so the name and the coordinates are written only once, in the header. Naming the sequence after the task also shows in clients which task the running sequence belongs to; the scheduler itself does not read the sequence name. A task run by hand outside the scheduler must define `scheduler_task` itself or use literal values.

### Lexical rules

- The text is UTF-8. A leading byte-order mark is ignored, and CRLF line endings are accepted.
- A **header line** is `//`, optional spaces or tabs, `@`, a key, then at least one space or tab and the value. Trailing whitespace is ignored. A header line is at most 255 bytes long; trailing whitespace and the CR of a CRLF line ending count.
- An **empty comment line** (`//` with only whitespace after it) is allowed inside the header.
- The header starts at the first line and ends at the first line that is neither a header line nor an empty comment line. A blank line ends the header.
- `// @key` lines after the end of the header belong to the body and are ignored by the scheduler.
- Keys and keywords are case-insensitive. Every key except `@task` is optional, and each key appears at most once.
- Comparison operators are mandatory where shown: `>` or `>=` for a minimum and `<` or `<=` for a maximum. The two forms mean the same.

### Header keys

| Key | Syntax | Meaning | Default |
|---|---|---|---|
| `@task` | `name` | Display name, 1–40 bytes, no control characters and no `\|`, unique among stored and urgent tasks, compared case-insensitively | required |
| `@priority` | `urgent` \| `high` \| `normal` \| `low` [`level`] \| `hook` | Priority class and level 0–100 within the class. The level is ignored for `urgent`. `hook` takes no level; it marks a hook-only task that is never selected as an ordinary task, and rejects `@repeat`, `@retry`, `@enabled`, `@window`, `@altitude`, `@airmass`, `@ha`, `@moon`, `@illumination`, `@twilight` and `@preempted` | `normal 50` |
| `@target` | `ra dec` | J2000 coordinates. RA in hours (`hh:mm:ss.s` or decimal) and Dec in degrees (`±dd:mm:ss` or decimal) | none |
| `@altitude` | `> degrees` | Minimum target altitude, 0–90 | off |
| `@airmass` | `< value` | Maximum target airmass, 1–100 | off |
| `@ha` | `from .. to` | Allowed hour-angle range in hours, −12 to +12. `from` greater than `to` wraps across 12h | off |
| `@moon` | `> degrees` [`width days`] | Minimum moon separation, 0–180, with a width of 0.5–30 days, see [Lorentzian moon avoidance](#lorentzian-moon-avoidance); ignored while the moon is below the horizon | off |
| `@illumination` | `< percent` | Maximum moon illumination, 0–100; ignored while the moon is below the horizon | off |
| `@twilight` | `none` \| `civil` \| `nautical` \| `astronomical` | No limit, or the Sun must be below −6°, −12° or −18° | `none` |
| `@window` | `[start] .. [end]` | UTC time window, see below | open |
| `@duration` | `minutes` | Expected run time, 1–1440; all constraints must hold for this whole interval before the task starts | estimated, see [Task duration](#task-duration) |
| `@repeat` | `count` [`every minutes`] | Number of successful runs, 1–10000, with an optional pause of 0–100000 minutes between them | `1` |
| `@retry` | `count` [`after minutes`] | Number of reruns after a failure, 0–100, with an optional delay of 0–100000 minutes | `0` |
| `@preempted` | `retry` \| `fail` | What happens to the task when an urgent task preempts it or the [unsafe hold](#unsafe-hold) aborts it | `retry` |
| `@enabled` | `yes` \| `no` | A disabled task is kept but never selected | `yes` |
| `@folder` | `name` \| `none` | The folder under the data base folder where the frames of the task are saved, 1-40 bytes: `name` forces it, also when `FOLDER` of `AGENT_SCHEDULER_DATA` is off, and lets several tasks share it; `none` disables the folder of the task, which leaves `DIR` of the Imager Agent untouched, see [Observation data](#observation-data) | by `FOLDER` |
| `@observer` | text | Observer name, written as the FITS header `OBSERVER` of the frames of the task when `FITS` of `AGENT_SCHEDULER_DATA` is on, 1-56 characters without control characters, a quote counts as two (it is doubled in the FITS value), see [Observation data](#observation-data) | no header |
| `@pi` | text | Principal investigator, written as the FITS header `PI_NAME` under the same rules as `@observer` | no header |
| `@project` | text | Project or proposal identifier, written as the FITS header `PROPID` under the same rules as `@observer` | no header |

The constraints `@altitude`, `@airmass`, `@ha`, `@moon` and `@illumination` require `@target`. A header that uses them without `@target` is invalid. A task with `@target` also requires the target above the horizon, with or without `@altitude`, and within the global `MIN_ALTITUDE` and `MAX_ALTITUDE` of `AGENT_SCHEDULER_SETTINGS` (reasons `target below the altitude limit` and `target above the altitude limit`).

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

**Estimation.** The agent estimates a task when it is uploaded, edited, reset after a script error or loaded from the configuration. Estimates run one at a time, also while another task runs, because they never start a sequence. The agent loads `Sequencer.js` for them even when the scheduler is stopped. A task is not selected while its estimate is pending, so that script errors are found before the first run and the constraints are checked against the right duration. While any selectable task waits for its estimate, no other ordinary task starts either, because the waiting task may have the higher priority; only an eligible urgent task starts. The estimate timeout bounds this wait.

1. The agent sends the body to the Scripting Agent in estimation mode, inside the wrapper script described in [Running a task](#running-a-task). In this mode, the wrapper temporarily replaces `Sequence.prototype.start()` with a function that calls `this.estimate()` and publishes the result instead of starting the sequence, and restores `start()` afterwards.
2. The result is published in the `SCHEDULER_ESTIMATE` property of the Scripting Agent. Its items are `TOKEN`, `SUPPORTED`, `EXPOSURE`, `FRAMES`, `OPERATIONS`, `WAITS` and `UNKNOWN`. `SUPPORTED` is 0 when the loaded `Sequencer.js` has no `estimate()`, and the other counts are then 0. An error in the body is reported in `SCHEDULER_RESULT` instead.
3. The agent matches the token and computes the duration:

   ```
   duration = exposure + frames * FRAME_OVERHEAD + operations * OPERATION_OVERHEAD + waits
   ```

   `FRAME_OVERHEAD` covers download, saving, dithering and settling of one frame. `OPERATION_OVERHEAD` is a typical time for one equipment step. The agent keeps the computed duration, so a change of `FRAME_OVERHEAD` or `OPERATION_OVERHEAD` does not recompute existing estimates; it applies to the estimates made after it, for example after an upload or an edit.

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
| RETRY | Failed, preempted or aborted by the unsafe hold; returns to QUEUED after the retry delay, at once after an abort by the unsafe hold |
| REPEAT | Completed a run with more runs to go; returns to QUEUED after the repeat interval |
| DONE | All runs completed |
| FAILED | Failed with no retries left, or preempted or aborted by the unsafe hold with `@preempted fail` |
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
   any state -- DISABLE --> DISABLED -- ENABLE --> previous state
   any state but RUNNING -- RESET --> QUEUED, counters cleared
   RUNNING -- unsafe hold --> as preempted, RETRY without a retry delay
```

Further rules:

- `DISABLE` of a RUNNING task does not stop it: the run finishes and its result is applied, and the task is not selected again until it is enabled.
- A preemption or an abort by the unsafe hold does not consume a retry, because the task did not fail on its own.
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
| `AGENT_SCHEDULER_TASK_STATE` | light | one per task | Idle = QUEUED, WAITING, RETRY, REPEAT or DISABLED; Busy = RUNNING; Ok = DONE; Alert = FAILED or EXPIRED. A task with a script error is Alert, and otherwise a task named as a hook or of class `@priority hook` is Idle, also while the hook runs. The label has six columns joined by ` \| `: name, state, priority, runs, retries and duration, see [Label format](../../../indigo_docs/INDIGO_SCHEDULER_GUIDE.md#label-format) |
| `AGENT_SCHEDULER_TASK_NAME` | text | `NAME` | Target of `AGENT_SCHEDULER_TASK_COMMAND` |
| `AGENT_SCHEDULER_TASK_COMMAND` | switch | `RESET`, `ENABLE`, `DISABLE`, `MOVE_UP`, `MOVE_DOWN`, `RESET_ALL`, `DELETE_ALL` | The first five apply to the task named in `AGENT_SCHEDULER_TASK_NAME`; `RESET_ALL` and `DELETE_ALL` apply to all tasks, see [Commands for all tasks](#commands-for-all-tasks). List order breaks score ties |
| `AGENT_SCHEDULER_CONTROL` | switch | `START`, `STOP`, `ABORT` | Scheduler control, see [Scheduler control](#scheduler-control) |
| `AGENT_SCHEDULER_STATE` | number | `DONE`, `FAILED`, `URGENT_DONE`, `URGENT_FAILED`, `TOTAL` | Task counters: `DONE` and `FAILED` (failed or expired) count the tasks of the list; `URGENT_DONE` and `URGENT_FAILED` count the one-off urgent tasks, which leave the list when they end; a deleted or aborted one-off task is not counted. The urgent counters are saved with the history and reset with it. `TOTAL` counts the tasks of the list. The property state is Idle before the first `START`, Busy while scheduling, Ok after `STOP` and the automatic stop, and Alert after `ABORT` (also while stopped), a failed `STARTUP`, `NIGHT_START`, `RESUME`, `UNSAFE`, `SAFE` or `SHUTDOWN` hook, or a `STOP_TIMEOUT` stop |
| `AGENT_SCHEDULER_CURRENT` | text | `CURRENT`, `REASON` | The running task or hook, and why nothing starts when the scheduler waits: why nothing is eligible, or which task the [VISIBILITY](#strategies) plan starts next and when. What comes next is in the [night plan](#night-plan-preview) |
| `AGENT_SCHEDULER_PLAN` | light | one per planned run | The forecast of the rest of the night in expected run order, see [Night plan preview](#night-plan-preview) |
| `AGENT_SCHEDULER_HISTORY` | light | one per finished run | The record of finished runs, oldest first, newest last, see [Run history](#run-history) |
| `AGENT_SCHEDULER_CLEAR_HISTORY` | switch | `CLEAR` | Remove all entries from `AGENT_SCHEDULER_HISTORY` |
| `AGENT_SCHEDULER_RESTORE_HISTORY` | text | `ENTRIES` | Used only to restore the run history from the configuration; published with an empty value, and changes from clients are rejected |
| `AGENT_SCHEDULER_SETTINGS` | number | see below | Scheduler settings |
| `AGENT_SCHEDULER_DATA` | switch | `OBJECT`, `FITS`, `FOLDER` | What the agent sets on the Imager Agent for a run, any of many, see [Observation data](#observation-data) |
| `AGENT_SCHEDULER_DATA_DIR` | text | `BASE` | Parent folder of the task folders, empty for the folder the Imager Agent had when the scheduler started |
| `AGENT_SCHEDULER_HOOKS` | text | `STARTUP`, `SHUTDOWN`, `NIGHT_START`, `NIGHT_END`, `IDLE`, `RESUME`, `UNSAFE`, `SAFE` | Names of stored tasks used as hooks, see [Hooks](#hooks), [Night hooks](#night-hooks) and [Unsafe hold](#unsafe-hold) |
| `AGENT_SCHEDULER_NIGHT` | switch | `CIVIL`, `NAUTICAL`, `ASTRONOMICAL` | The Sun altitude that bounds the scheduler's night for the night hooks |
| `AGENT_SCHEDULER_STRATEGY` | switch | `COMPACT`, `VISIBILITY` | How the next task is chosen: `COMPACT` (default) starts the best eligible task at once, `VISIBILITY` follows a night plan that places the runs nearer their culmination, see [Strategies](#strategies) |
| `AGENT_SCHEDULER_SAFETY_MISSING` | switch | `SAFE`, `UNSAFE` | How missing safety information counts, one of many: as safe (`SAFE`, default) or as unsafe, see [Unsafe hold](#unsafe-hold) |

`AGENT_SCHEDULER_SETTINGS` items:

| Item | Unit | Default | Meaning |
|---|---|---|---|
| `IDLE_CHECK_INTERVAL` | s | 30 | Interval of constraint checks while waiting, and of urgent-task checks while a task runs |
| `IDLE_HOOK_DELAY` | min | 30 | Time without an eligible task before the `IDLE` hook runs |
| `STOP_TIMEOUT` | s | 60 | Time allowed for an aborted sequence and its agents to stop |
| `START_TIMEOUT` | s | 10 | Time allowed for a started task's sequence to become Busy |
| `SETTING_WEIGHT` | — | 1 | Weight of the setting-soonest score term, 0–4 |
| `ALTITUDE_WEIGHT` | — | 1 | Weight of the altitude score term (the altitude relative to the target's culmination), 0–4 |
| `VISIBILITY_FLIP_PENALTY` | min | 10 | Best visibility only: cost of one meridian flip in minutes of full-quality imaging, 0–120, 0 = off; ignored under COMPACT and while `MERIDIAN_FLIP.ENABLED` is 0 or missing, see [Meridian flips](#meridian-flips) |
| `FRAME_OVERHEAD` | s | 10 | Time added per captured frame in the duration estimate |
| `OPERATION_OVERHEAD` | s | 120 | Time added per slew, centering, focus, guiding start or similar step in the duration estimate |
| `NIGHT_START_OFFSET` | min | 0 | Shift of the night start, −240 to 240, negative is earlier |
| `NIGHT_END_OFFSET` | min | 0 | Shift of the night end, −240 to 240, negative is earlier |
| `SAFE_DELAY` | min | 10 | Time the conditions must have been safe without interruption before the [unsafe hold](#unsafe-hold) ends or a postponed `START` begins, 0–1440 in steps of 0.1 |
| `MIN_ALTITUDE` | ° | 20 | Global minimum altitude of every task with `@target`, 0–90; the effective minimum is the larger of it and the task's `@altitude` |
| `MAX_ALTITUDE` | ° | 90 | Global maximum altitude of every task with `@target`, 0–90, 90 = no ceiling; a change with `MIN_ALTITUDE` at or above it is rejected with Alert and the old values are kept. A task whose target can never stand within the limits (`MAX_ALTITUDE` not above `@altitude`, upper culmination `90 − \|latitude − declination\|` below the effective minimum, lower culmination `\|latitude + declination\| − 90` above `MAX_ALTITUDE`) is `UNREACHABLE` in the task list (Alert) and the night plan (a hook task never is), is skipped, and is reported once by the message `Task 'NAME' can never run: ...` when it becomes unreachable or its reason changes |

**Requests.** Every change of a writable property is queued. The agent's handler acknowledges it with the property in the Busy state and completes it with Ok or Alert. Only the handler changes the properties; client enumerations receive copies. INDIGO does not publish an update that changes neither values nor state, so without the Busy state a client, and the configuration restore, would not see the result.

`AGENT_SCHEDULER_TASK_NAME` and `AGENT_SCHEDULER_TASK_COMMAND` form a two-step command, which is a common INDIGO idiom. Two clients that issue commands at the same moment can interfere. If both properties are changed in one client action, the command applies to the name sent with it.

**Labels and list properties.** INDIGO item labels are limited to 511 bytes, and clients show them in narrow lists. Task names are therefore limited to 40 bytes. The labels of `AGENT_SCHEDULER_TASK_STATE`, `AGENT_SCHEDULER_PLAN` and `AGENT_SCHEDULER_HISTORY` are cut to fit without splitting a UTF-8 character; only the last column, the reason, is cut. These properties are defined and updated without a message; a full failure reason reaches the client in the agent's own message, such as `Task '<name>' failed: <reason>`. The item names are internal identifiers (`TASK_<n>`, `PLAN_<n>`, `RUN_<n>`), not task names.

Changing the number of items of a list property requires deleting and defining it again, which clients see as the list being rebuilt. INDIGO cannot change an item label with an update either. The agent therefore redefines `AGENT_SCHEDULER_TASK_STATE` and `AGENT_SCHEDULER_PLAN` only when their items or labels change, and updates them in place when only the lights change. `AGENT_SCHEDULER_HISTORY` is redefined whenever it changes.

### Editing tasks

- An edit is validated like an upload. An invalid edit leaves the stored task unchanged.
- An edit may change `@task`, which renames the task, as long as the new name is unique.
- An edit keeps the task's state and counters. To run a completed, failed or expired task again, the client uses `RESET`.
- An edit of a running task takes effect after the task ends.
- A hook that names a deleted or renamed task is ignored, and the agent sends a warning message when the hook is due.

### Commands for all tasks

`RESET_ALL` and `DELETE_ALL` in `AGENT_SCHEDULER_TASK_COMMAND` ignore `AGENT_SCHEDULER_TASK_NAME`. Both items carry a `warn_on_set` hint, so clients that follow the hints ask for a confirmation before sending them. The hint is advisory; the agent does not wait for a confirmation.

- `RESET_ALL` resets every task, stored or urgent, hooks included, as `RESET` does: state QUEUED, run and retry counters cleared, script errors cleared. A running task is left alone, and the update message gives the number of reset tasks and the name of the running one. Unlike `RESET`, it keeps the relative order of pending urgent tasks. The disabled flag is kept.
- `DELETE_ALL` deletes every stored and urgent task except the tasks named in `AGENT_SCHEDULER_HOOKS` and the tasks with `@priority hook`, so that the preparation and the shutdown of the equipment stay configured. It is rejected with Alert unless the scheduler is stopped and no task or hook runs. Deleted tasks get no history entry. The update message gives the number of deleted and kept tasks.

The other schedulers keep start and end actions apart from targets (SGP sequence events, N.I.N.A. Target Scheduler instructions, ACP startup and shutdown scripts) and also offer resetting all targets at once.

### Enabling and disabling

A task is selectable only when its header has `@enabled yes` and it is not disabled by the `DISABLE` command. `ENABLE` clears only the command flag. A task disabled in its header stays disabled until the header is edited.

### Urgent tasks

A task submitted through `AGENT_SCHEDULER_URGENT_TASK` is not stored:

- It appears in `AGENT_SCHEDULER_TASK_STATE` while it is pending or running.
- It can be cancelled by name with `AGENT_SCHEDULER_DELETE_TASK`.
- It is removed when it ends or expires, and it is not persisted, so a server restart drops it. It is removed after its first run whatever the result, so `@repeat` and `@retry` have no effect on it.

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
- a change of `SETTINGS`, `HOOKS`, `NIGHT` or `SAFETY_MISSING`;
- a task state transition or a new history entry;
- a change between safe and unsafe conditions, and the start and end of an [unsafe hold](#unsafe-hold);
- a history clear.

Changes that come in quick succession are written once, two seconds after the last of them. Nothing is saved while the configuration is being restored. The file contains, in this order:

1. one `AGENT_SCHEDULER_ADD_TASK` change per stored task, in list order. The agent saves a temporary copy of the property with two items:
   - `TASK`, the task text;
   - `STATUS`, the runtime status in a compact form, for example `state=DONE runs=2 retries=0 disabled=0 measured=3540`.
2. `AGENT_SCHEDULER_SETTINGS`, `AGENT_SCHEDULER_DATA`, `AGENT_SCHEDULER_DATA_DIR`, `AGENT_SCHEDULER_HOOKS`, `AGENT_SCHEDULER_NIGHT`, `AGENT_SCHEDULER_STRATEGY` and `AGENT_SCHEDULER_SAFETY_MISSING`;
3. one `AGENT_SCHEDULER_RESTORE_HISTORY` change. The agent saves a temporary copy whose `ENTRIES` item holds all history entries, one line per entry, oldest first, preceded by a line `#urgent`, `URGENT_DONE`, `URGENT_FAILED` when either counter is not zero, and by a line `#safety`, `unsafe` or `safe`, time when the conditions have ever been unsafe: `unsafe` when they were unsafe at the save, `safe` with the time of the latest unsafe report otherwise; both lines have too few fields to be read as an entry by a version without them, and `AGENT_SCHEDULER_CLEAR_HISTORY` does not remove the `#safety` line. Each line holds the task name, run number, start, end, result, a mark (`H` for a hook, `U` for an urgent task, `-` for neither) and the reason, separated by tabs. The configuration reader trims trailing whitespace, so no field is saved as a space, and entries whose trailing empty fields were trimmed are still restored.

With a task text of at most 16 KiB and 128 tasks, the file stays below about 2 MiB.

**Loading.** The agent makes no load call itself. Its attach callback initialises the task store and defines its properties; right after the attach callback returns, the bus (`indigo_attach_device()`) sends the device a `CONFIG.LOAD` change, which the agent passes to `indigo_device_change_property()`. That starts the standard configuration restore of `indigo_driver.c`, which parses the file and replays the saved changes in file order. Each saved change is replayed through the agent's change handler, with the configuration reader as the client:

- An `AGENT_SCHEDULER_ADD_TASK` change validates the header and creates the task, as an upload does. A non-empty `STATUS` restores the task's state and counters. `STATUS` from any other client is ignored.
- A replayed task that fails validation, for example after a grammar change in a newer version, is skipped. The agent sends a message with the validation error, `The saved task '<name>' was skipped: <error>`, which names the task when its `@task` line could be read before the error, and `A saved task was skipped: <error>` otherwise. The rest of the configuration still loads.
- An `AGENT_SCHEDULER_RESTORE_HISTORY` change rebuilds the history.
- The duration estimates of the restored tasks are requested when the Scripting Agent appears.

**After a restart:**

- Finished work is not repeated: DONE, FAILED and EXPIRED tasks keep their state, and the counters are kept.
- A task that was RUNNING when the server stopped is restored as RETRY without consuming a retry.
- Repeat and retry delays are not saved: a RETRY or REPEAT task returns to QUEUED without waiting for the rest of its delay.
- WAITING tasks are restored as QUEUED and re-evaluated.
- Pending urgent tasks submitted through `AGENT_SCHEDULER_URGENT_TASK` are lost. A stored task with `@priority urgent` is kept.
- The scheduler itself always starts stopped. Scheduling resumes only after `START`.
- A save made while the conditions were unsafe counts as an unsafe report at the time of the restore, because the conditions while the server was down are unknown; a `safe` line keeps the time of the latest unsafe report. `START` therefore waits until the conditions have been safe for `SAFE_DELAY` after it, see [Unsafe hold](#unsafe-hold). The hold itself and the standby are not saved.

**Moving tasks between systems.** Clients export a task by reading `AGENT_SCHEDULER_TASK_<n>.TASK` and saving the text to a file, and import one by sending a file's text to `AGENT_SCHEDULER_ADD_TASK`. The configuration file is internal to the agent and is not an exchange format.

## Scheduling

All scheduler work runs on the agent's handler queue, so it is serialised without a lock. Property changes are copied and queued, and the agent never sends a request to another agent while holding a lock. This matters because the Scripting Agent calls into other agents while holding its JavaScript lock. Updates from other agents, such as `SEQUENCE_STATE`, arrive on client threads and are recorded under a small mutex of their own.

### Ephemeris grid

Eligibility checks over whole task durations, the setting-soonest score and the night plan all evaluate the sky at many times. Computing the Moon's position at every one of them would be too slow on small computers such as a Raspberry Pi.

The constraint engine therefore works on a grid:

- At every refresh, it computes the Sun and Moon altitudes, the moon illumination and the local sidereal time for the next 48 hours at 5-minute steps (577 points). The 48 hours cover a 24-hour task duration and the 12-hour look-ahead of the score.
- The Sun and Moon positions themselves are computed once per hour and interpolated linearly between. The Moon moves about half a degree per hour, so the interpolation error is far below any limit. The Moon position comes from the coarse ELP2000/82 call, so the Sun and Moon samples take a few milliseconds per refresh instead of most of the build time.
- For each task with `@target`, it computes the target's altitude, airmass and hour angle on the same grid. These values only change when the task is edited, apart from the precession to the date, which is negligible within a night.
- For each task, it also records for every grid point the first point at or after it where a sky constraint fails, with the reason. An interval check is then a single lookup, and the search for the next start skips over a failing stretch in one step instead of testing every start time. The time windows are checked separately, because a nightly window does not depend on the grid.
- The table also records for every grid point the first passing point at or after it, so the search for the next start jumps over a whole failing stretch at once.
- Eligibility, the setting-soonest term and the night plan are lookups in these tables. The time until the constraints of a task stop holding, which the setting-soonest term needs, is a single lookup too: for a task without a time window in the table of failing points, for a task with a window in a table of the first failing five-minute step at or after every grid point, which is built with the other tables.
- The night plan remembers for every task the stretch of start times in which it was found unable to start, up to its next possible start, and its nested simulations share it, so a task is not searched again at every simulated decision. A task with a nightly window is searched every time, because its window may open at any minute. The search for the next start in the night plan ends at the end of the plan.
- These shortcuts give exactly the same results as the step-by-step evaluation; the unit test `points_match_reference` in `test_agent_scheduler_sky` compares the next-start search and the time to set, windows included, with the step-by-step reference.

The grid and the task tables are rebuilt when the site coordinates change and every hour. A task's tables are also rebuilt when it is edited.


### Scheduler control

- `START` runs the `STARTUP` hook, then begins scheduling. If the hook fails, the scheduler stops and its state becomes Alert. While the conditions are unsafe, or have been safe for less than `SAFE_DELAY`, the start is postponed, see [Unsafe hold](#unsafe-hold).
- `STOP` lets the running task finish, then runs the `SHUTDOWN` hook and stops.
- `ABORT` aborts the running task as described in [Aborting a sequence](#aborting-a-sequence) and marks it RETRY without consuming a retry. It then runs the `SHUTDOWN` hook and stops. `ABORT` while the scheduler is stopped sets its state to Alert.
- When there is at least one task and every task is DONE, FAILED, EXPIRED, DISABLED or has a script error, the scheduler runs the `SHUTDOWN` hook and stops. With no tasks at all it keeps running, ready for urgent tasks. A task added or reset before the shutdown hook of this automatic stop starts cancels the stop.

### Hooks

The `STARTUP`, `SHUTDOWN` and `IDLE` hooks name stored tasks.

- A hook task runs outside the state machine. Its constraints, state and counters are not evaluated or changed, so it can run every night. The task list shows it with the state `HOOK` (`DISABLED` while the task is disabled) and a priority such as `hook startup`, `hook shutdown` or `hook idle`; a task with `@priority hook` that no hook names shows the priority `hook`.
- A task named as a hook, or with `@priority hook`, is never selected as an ordinary task and does not count when deciding that all tasks have finished. It is not planned in the night plan, not even as `NOT TONIGHT`, and `DELETE_ALL` keeps it. A task uploaded for use as a hook therefore does not run at once as an ordinary task before it is named in `AGENT_SCHEDULER_HOOKS`; with `@priority hook` it also stays hook-only when it is removed from the hooks. The header does not name the hook role, only `AGENT_SCHEDULER_HOOKS` does. An urgent submission with `@priority hook` is rejected with Alert.
- Hook runs appear in the run history, marked as hooks.
- A hook run does not label the data: no object name, no FITS headers and no folder from `FOLDER`; only `@folder <name>` sets the folder, see [Observation data](#observation-data).
- A hook that fails is reported by a message. A failed `STARTUP` hook stops the scheduler with Alert; a failed `SHUTDOWN` hook still stops it, with Alert.
- The `SHUTDOWN` hook runs once for every stop: `STOP` after the running task, `ABORT` after the running task or startup hook was aborted, and the automatic stop when all tasks finished. It does not run after a `STOP_TIMEOUT`, because the old sequence still occupies the Scripting Agent. When it cannot run because a sequence started by hand occupies the Scripting Agent or `Sequencer.js` is missing, the agent says so in a message and stops without it.
- `START` while the `SHUTDOWN` hook runs cancels the stop: after the hook, the `STARTUP` hook runs again and scheduling continues.
- A hook runs whatever the state of its task; an edit of a hook while it runs takes effect after it ends.
- The `IDLE` hook runs once per idle period, `IDLE_HOOK_DELAY` minutes after the scheduler last found nothing to start. Waiting for the next run of a [VISIBILITY](#strategies) plan is idle time like waiting for a constraint, but the hook does not start while that run starts within `IDLE_HOOK_DELAY` minutes. A task that starts begins a new idle period.
- A hook that names no task, or a disabled task (`DISABLE` or `@enabled no`), is reported by a message (`The idle hook 'Park' is disabled, skipped`) and skipped, and scheduling continues as without that hook. The disabled hook is not planned in the night plan, and `ENABLE` makes it run again from the next occasion.

The `STARTUP`, `SHUTDOWN` and `IDLE` hooks follow `START`, the stop and idle periods, not the night. The [night hooks](#night-hooks) run at the start and the end of every night.

### Night hooks

**Motivation.** With the startup, shutdown and idle hooks alone, a scheduler that runs over several nights has no point at which the equipment is prepared for the night or put away in the morning. At dawn the `IDLE` hook usually parks the equipment. The Mount Agent unparks the mount before the next slew, and the dome when dome slaving is enabled, but nothing restores what else a hook switched off, such as the camera cooler, a flat cover or the dome shutter. ACP Scheduler solves this with a startup script every evening and a shutdown script every morning, and N.I.N.A.'s Target Scheduler with instructions run before and after each wait. The night hooks combine both.

**Night limit.** The scheduler gets one night for all tasks. A new switch property `AGENT_SCHEDULER_NIGHT` selects the Sun altitude that bounds it:

| Item | Sun altitude |
|---|---|
| `CIVIL` | below −6° |
| `NAUTICAL` | below −12° (default) |
| `ASTRONOMICAL` | below −18° |

Two new `AGENT_SCHEDULER_SETTINGS` items move its ends:

| Item | Unit | Default | Range | Meaning |
|---|---|---|---|---|
| `NIGHT_START_OFFSET` | min | 0 | −240–240 | Shift of the night start; negative is earlier, for example −30 to start cooling the camera half an hour before the night |
| `NIGHT_END_OFFSET` | min | 0 | −240–240 | Shift of the night end; negative is earlier |

- A night lasts from the time the Sun goes below the limit, plus `NIGHT_START_OFFSET`, until the time it rises above it, plus `NIGHT_END_OFFSET`. The agent samples the Sun's altitude every five minutes from 36 hours before to 36 hours after now, from Sun positions computed once per hour, and interpolates the crossings. It computes the nights again every hour and after a change of the site, the limit or the offsets. A night recomputed with slightly different ends, or for a new site, counts as the same night when the two overlap.
- When the offsets make one night end at or after the start of the next, the two are one night.
- When the Sun does not go below the limit, for example in summer at high latitude with `ASTRONOMICAL`, or the offsets leave no time, there is no night. When the Sun does not rise above the limit, as in polar winter, the night does not end. When a night hook is named, the agent says so in a message after `START`, after a hook is named and whenever this changes, and the night plan ends with an Alert item when no night starts within a day.
- The night limit times only the night hooks. It does not constrain tasks: every task keeps its own `@twilight`, and a task without one may also run outside the scheduler's night.

**New hooks.** `AGENT_SCHEDULER_HOOKS` gets three more items. Like the current hooks, they name stored tasks:

| Item | When the task runs | Example |
|---|---|---|
| `NIGHT_START` | When the night starts, if the night plan has work for the night; once per night and `START` | cool the camera, open the dome, open the flat cover |
| `NIGHT_END` | When the night ends, if `NIGHT_START` ran in that night; once per night and `START` | park, close the dome, warm up the camera |
| `RESUME` | Before a task that starts while the equipment is in standby, see below | unpark and cool the camera again after the `IDLE` hook |

**Night state and standby.** The agent keeps two flags, which are not visible as properties:

- *Night* says whether `NIGHT_START` has run in the current night. A night start runs `NIGHT_START` and sets the flag, unless the night has no work, see below. A night end runs `NIGHT_END` only when the flag is set, and clears it. On `START` during the night, the night start is handled right after the `STARTUP` hook. A `STOP` and `START` within one night therefore runs `NIGHT_START` again, and `NIGHT_END` always follows a `NIGHT_START` of the same night.
- *Standby* says that a hook has put the equipment away and nothing has prepared it since. A named `IDLE` or `NIGHT_END` hook sets it when it runs, whatever its result. A named `RESUME` or `NIGHT_START` hook clears it when it succeeds. Any task start also clears it; the resume hook, when it is named, runs before that start. `START` clears it.

**Rules.**

- **Night hooks never interrupt a task.** They run only when no task runs, at the next decision after the boundary: when the running task ends, or at the next `IDLE_CHECK_INTERVAL` check while the scheduler waits. A task that must stop at dawn does so through its own `@twilight` or a checkpoint in its body, such as `break_if_solar_altitude_above()`.
- **Nights without work.** At the night start, the agent looks at the [night plan](#night-plan-preview). When it has no task run that starts before the end of the night, `NIGHT_START` does not run, the night flag stays clear, and the agent says so in a message when a night hook is named, also when it is named later in that night: `No task can run tonight, the night start hook is skipped`. The equipment is not prepared, and `NIGHT_END` does not run at the end of that night. When the plan gains a run later in the night, `NIGHT_START` runs at the next decision, and at the latest before that run starts. This covers an uploaded or reset task, an urgent task, or a constraint that clears earlier than forecast. The equipment then gets less lead time than the offset gives, for example for cooling.
- **Order.** At one decision, the agent first runs `NIGHT_END`, then `NIGHT_START`, when they are due. It then selects a task. If it starts one in standby, `RESUME` runs first. If nothing is eligible, the `IDLE` rules apply. A task that runs from before the end of one night until after the start of the next is followed by both night hooks.
- **Idle periods.** `NIGHT_START` and `NIGHT_END` begin a new idle period when they run, as a task start does. `IDLE` does not run in standby. So after `NIGHT_START`, `IDLE` can put the equipment away again while the scheduler waits for the first target. After `NIGHT_END` it does not run during the day.
- **One task, several hooks.** One task may serve several hooks, for example a Park task as `SHUTDOWN`, `NIGHT_END` and `IDLE`. When two hooks that name the same task are due one right after the other, the task runs once. This includes `STARTUP` and `NIGHT_START` on `START` during the night, and `NIGHT_END` and `SHUTDOWN` when `STOP` arrives while the night end hook runs. The task list shows the first hook in the order `STARTUP`, `SHUTDOWN`, `NIGHT_START`, `NIGHT_END`, `IDLE`, `RESUME`, `UNSAFE`, `SAFE`.
- **Unnamed hooks.** A night hook item that is empty, or that names no stored task, is skipped; a missing task is reported by a message, as for the current hooks. The night flag changes as if the hook had run, but the standby does not. An empty `NIGHT_START` therefore leaves a standby set by `IDLE` to `RESUME`. Without `RESUME`, a task in standby starts directly. A configuration without the night hooks behaves exactly as before them.
- **Failures.** A failed `NIGHT_START` or `RESUME` leaves the equipment half prepared, for example with the roof open, and the next task would run on equipment that is not ready. The agent therefore runs the `SHUTDOWN` hook and stops with Alert. This differs from a failed `STARTUP`, which stops without `SHUTDOWN` because the scheduler has not started. A failed `NIGHT_END` or `IDLE` is reported by a message and scheduling continues.
- **Stopping.** `STOP`, `ABORT` and the automatic stop run only the `SHUTDOWN` hook, also during the night; `NIGHT_END` does not run then.
- **Night plan.** The [night plan](#night-plan-preview) ends at the later of the next sunrise and the end of the scheduler's night. `NIGHT_START` and `NIGHT_END` appear in it at their expected times, marked as hooks; on a night without work, neither appears. The simulation reserves their [duration](#task-duration) like a run. A night hook that names the task of the running hook and is due when that hook is expected to end is not planned again, because the task runs once, see **One task, several hooks**. `RESUME` depends on idle periods that the plan does not forecast, so it is shown only while it runs, like the other hooks that the plan does not forecast (`STARTUP`, `SHUTDOWN` and `IDLE`). The plan is also recomputed after a change of `AGENT_SCHEDULER_NIGHT` or `AGENT_SCHEDULER_HOOKS`.
- **History.** Night hook runs are recorded in the [run history](#run-history) and marked as hooks, like the current hooks.
- **Persistence.** `AGENT_SCHEDULER_NIGHT`, the offsets and the hook items are saved, see [Persistence](#persistence). The night and standby flags are not saved: the scheduler starts stopped after a restart, and `START` sets them again.

**Example.** An observatory that cools the camera and opens the roof half an hour before nautical dusk, parks when clouds come and closes at dawn:

```
AGENT_SCHEDULER_NIGHT.NAUTICAL = ON
AGENT_SCHEDULER_SETTINGS.NIGHT_START_OFFSET = -30
AGENT_SCHEDULER_HOOKS: NIGHT_START = Evening, RESUME = Evening, IDLE = Park, NIGHT_END = Morning, SHUTDOWN = Morning
```

`Evening` cools the camera, opens the roof and unparks; `Park` parks the mount; `Morning` parks, closes the roof and warms the camera up.

### Unsafe hold

**Safety.** The agent observes the light property `AGENT_AUX_SAFETY` of the "Auxiliary Agent" all the time, also while it is stopped. Only the light of its item `SAFE` counts, not the property state: Ok is safe and Alert is unsafe. The information is missing when the Auxiliary Agent or the property is absent, or when the light is Idle (no information) or Busy. The switch `AGENT_SCHEDULER_SAFETY_MISSING` decides how missing information counts: as safe (`SAFE`, the default, so a setup without the Auxiliary Agent behaves as before) or as unsafe (`UNSAFE`). The Auxiliary Agent computes `AGENT_AUX_SAFETY` from `AUX_WEATHER_SAFETY` of the aux devices selected in its `FILTER_AUX_1_LIST` to `FILTER_AUX_4_LIST`, see the Auxiliary agent section of [PROPERTIES.md](../../../indigo_docs/PROPERTIES.md#auxiliary-agent); until a weather source is selected, and again after every source is lost, its light is Idle, so `AGENT_SCHEDULER_SAFETY_MISSING` decides, and with `UNSAFE` the scheduler holds until a source reports safe conditions. The agent keeps the time of the latest unsafe report. Every update with an Alert light counts, also one that reverts to Ok before the agent's next evaluation half a second later.

**Hooks.** `AGENT_SCHEDULER_HOOKS` gets two more items, which name stored tasks like the other hooks:

| Item | When the task runs | Example |
|---|---|---|
| `UNSAFE` | Once per hold, after everything that ran has been aborted | park, close the roof, close the flat cover |
| `SAFE` | When the hold ends in the night with work to do | open the roof, unpark, cool the camera again |

**Entering the hold.** When the conditions become unsafe while the scheduler runs, it enters the unsafe hold at once, without debounce, and says so (`Unsafe conditions, the scheduler holds`, or `No safety information, treated as unsafe, the scheduler holds`):

- A running task is aborted as described in [Aborting a sequence](#aborting-a-sequence), also an urgent task. Its history entry has the result `unsafe` and the reason `unsafe conditions`. Its `@preempted` key applies as after a preemption: with `retry`, the default, it returns to RETRY without consuming a retry and without a retry delay, and a one-off urgent task stays in the list and runs after the hold, subject to its window; with `fail` it becomes FAILED (`Task '<name>' aborted by unsafe conditions and failed`), and a one-off urgent task leaves the list and is counted in `URGENT_FAILED`. A preemption in progress is recorded as unsafe instead of preempted, with the same `@preempted` rules.
- A running `STARTUP`, `NIGHT_START`, `IDLE`, `RESUME` or `SAFE` hook is aborted, with the result `unsafe`; this is not a hook failure. An interrupted `STARTUP` hook runs again after the hold, an interrupted `NIGHT_START` hook does not, because the night counts as started, and the `SAFE` or `RESUME` hook prepares the equipment instead. A running `SHUTDOWN`, `NIGHT_END` or `UNSAFE` hook finishes.
- A sequence started by hand that runs when the hold begins is aborted (`A sequence started by hand was aborted by unsafe conditions`), without a history entry. A sequence started by hand during the hold is left alone, since it may be the operator closing the roof; the `UNSAFE` hook waits for it with the reason `another sequence is running`.
- Then the `UNSAFE` hook runs, once per hold. When it names a task that has just run successfully as the previous hook, for example `NIGHT_END`, the task does not run again.

**During the hold** no task, urgent task, `STARTUP`, `NIGHT_START`, `RESUME` or `IDLE` hook starts, and the automatic stop waits for the end of the hold. The `NIGHT_END` hook still runs at the night end when the night has started; a night that starts during the hold is decided after it. Duration estimates, expiry and the night plan continue. `AGENT_SCHEDULER_CURRENT.REASON` shows `unsafe conditions`, `no safety information, treated as unsafe`, or, once the conditions are safe again, `unsafe hold ends in <n> min`. `STOP`, `ABORT` and the automatic stop work as usual and run the `SHUTDOWN` hook; the `UNSAFE` hook is then not started. Every unsafe report, including a short one, restarts the delay; a report during the `UNSAFE` hook only moves the delay.

**Ending the hold.** The hold ends at a decision once the conditions have been safe for `SAFE_DELAY` minutes without interruption:

- During the scheduler's night with work to do (an eligible task, or a planned run before the night ends; without a known site the whole time counts as night), the `SAFE` hook runs (`Safe conditions, the unsafe hold ends`). When it succeeds, the standby ends and normal selection resumes; a night start that passed during the hold runs its `NIGHT_START` hook next, once when both items name one task.
- Otherwise, outside the night, without work, or with `SAFE` empty or naming no task (reported by a message), the `SAFE` hook does not run. The hold is lifted into standby (`Safe conditions, the unsafe hold ends, the scheduler continues in standby`), so the `NIGHT_START` or `RESUME` hook prepares the equipment when it is needed; without `RESUME`, a task in standby starts directly.
- When the `STARTUP` hook has not run yet, `Safe conditions, the scheduler starts` is said, and the `STARTUP` hook prepares instead of the `SAFE` hook.

**Postponed start.** `START` while the conditions are unsafe, or have been safe for less than `SAFE_DELAY`, starts the scheduler in a hold: its state is Busy with the message `Unsafe conditions, the start waits for safe conditions`, and the `STARTUP` hook and the first tasks wait until the conditions have been safe for `SAFE_DELAY`. No `UNSAFE` hook runs for it, because this start has prepared nothing. The time of the latest unsafe report is kept while the scheduler is stopped and across a restart, see [Persistence](#persistence).

**Failures.** A failed `UNSAFE` hook stops the scheduler with Alert (`The unsafe hook failed, the scheduler stopped`), after the `SHUTDOWN` hook. A failed `SAFE` hook does the same (`The safe hook failed, the scheduler stopped`), as a failed `RESUME` hook does. An `UNSAFE` or `SAFE` item that names no task or a disabled task is reported by a message and skipped, which is not a failure. A `SAFE` hook interrupted by a new unsafe report starts a new hold with its own `UNSAFE` hook.

**Night plan.** During the hold, no task run is planned before the earliest end of the hold: `SAFE_DELAY` after the latest unsafe report, or after now while the conditions are unsafe. A running `UNSAFE` hook is the Busy first item; `UNSAFE` and `SAFE` are not forecast otherwise.

**Remote agents.** Only the local "Auxiliary Agent" is observed; one on another server appears with a host suffix and is not used.

### When a decision is made

While no task is running, the agent selects the next task:

- after the previous task ends;
- after a task is uploaded, edited, enabled or reset;
- on `START`;
- every `IDLE_CHECK_INTERVAL` while it waits for constraints.

While a task is running, the agent checks every `IDLE_CHECK_INTERVAL` only whether an urgent task is pending and eligible; see [Urgent preemption](#urgent-preemption). It makes no other scheduling decisions. Limits inside a task, such as the target setting below the horizon, are the task body's job, through `Sequencer.js` steps like `break_if_target_altitude_below()` and `break_at()`.

### Selection

At every decision the agent goes through the following steps. The [night plan](#night-plan-preview) applies the same steps at every simulated decision, through the same functions in `indigo_agent_scheduler_plan.c`.

1. **Delays.** RETRY and REPEAT tasks whose delay has elapsed return to QUEUED.
2. **Expiry.** Tasks whose absolute `@window` end has passed become EXPIRED, also when some of their `@repeat` runs completed. The task list and the run history show how many runs succeeded.
3. **Eligibility.** A QUEUED or WAITING task is eligible when all its constraints hold over the whole interval from now to now + `@duration`. The interval is sampled every five minutes and at its end. A task that is not eligible becomes WAITING. Eligibility alone decides whether a task can run; the score below only orders the eligible tasks.
4. **Urgent tasks first.** Eligible urgent tasks run in the order they were submitted. They never wait for a planned run.
5. **Estimates.** While the duration of a task is still being estimated, no other task without urgency starts, because the waiting task may outrank it.
6. **Score.** Other eligible tasks are ranked by:

   ```
   score = class_base + level + SETTING_WEIGHT * setting + ALTITUDE_WEIGHT * altitude
   ```

   - `class_base` is 2000 for `high`, 1000 for `normal` and 0 for `low`. With weights of at most 4, the other terms add at most 900. Any high task therefore outranks any normal task.
   - `level` is the `@priority` level, 0–100.
   - `setting` is `100 * (1 − min(T, 12 h) / 12 h)`, where T is the time until the task's constraints stop holding. It favours tasks that become unobservable soonest.
   - `altitude` is `100 * max(0, alt_now) / alt_transit`, clamped to 0–100, where `alt_now` is the target altitude at the current time and `alt_transit = 90 − |latitude − declination|` is its altitude at upper culmination. It is 100 at transit and lower on either side, so every target scores best near its own transit, whatever its declination. It is 0 without `@target` or when the target never rises. A 0 only means no bonus: whether a task can run is decided by its constraints (eligibility), so an eligible task with a 0 altitude term, such as one without `@target`, still runs when it scores highest or is the only eligible task.

   Ties go to the task higher in the list.
7. **Strategy.** With `COMPACT`, the default, the best task starts. The selection is then greedy: it never leaves the equipment idle while a task is eligible, and the score only compares the tasks that are eligible at the same moment. A lone task starts as soon as it is eligible, however far it is from its culmination. With `VISIBILITY` the first run of the night plan starts at its planned time, see [Strategies](#strategies); until then the tasks stay QUEUED.
8. **Run.** The agent starts the selected task and waits until it ends.
   - Success: the run counter increases. The task becomes DONE or REPEAT.
   - Failure: the task becomes RETRY while retries remain, otherwise FAILED.
9. **Nothing to start.** When nothing is eligible, or the VISIBILITY plan waits for its next run, the agent publishes `REASON`. After `IDLE_HOOK_DELAY` minutes it runs the `IDLE` hook, for example to park the mount and close the dome, unless the equipment is already in [standby](#night-hooks).

### Strategies

`AGENT_SCHEDULER_STRATEGY` selects how the selection of step 7 chooses among the eligible tasks. It is a switch, one of many, saved with the configuration. A change takes effect at the next decision and does not stop a running task.

**Motivation.** The greedy selection fills the night from dusk. When the queue is shorter than the night, every task still runs back to back from dusk, often hours before its culmination, and the equipment is idle for the rest of the night. VISIBILITY plans the whole night first and moves the runs to the times where their targets stand highest, waiting where that gains more than it costs.

**COMPACT** (the default) is the greedy selection: the best eligible task starts at once, and the night plan is the greedy simulation of the [night plan preview](#night-plan-preview). It never leaves the equipment idle while a task can run.

**VISIBILITY** computes a night plan with `indigo_scheduler_plan_visibility()` and follows it: at a decision, the first planned task run that is not running starts at its planned time.

**Quality of a run.** The quality `q(X, s)` of task X started at time s is the mean relative altitude over the run: the mean of `100 * max(0, alt) / alt_transit`, each clamped to 0–100, over the grid points s, s + 5 min, … up to s + `@duration` (`indigo_scheduler_run_quality()`). A task without `@target`, or with a target that never rises, has quality 0.

**Objective.** The plan maximises the summed quality weighted by the run duration, `J = Σ q(X, s) × d / 3600` (percent × hours), over the planned runs that are not running, less the cost of the [meridian flips](#meridian-flips) of the runs that are not urgent. Between two plans, more task runs is better, then a higher `J`; on a tie (a difference of at most 10⁻⁶) the earlier candidate of the search wins, the previous plan first.

**Constraints.** A plan is a run order with a start on a whole UTC minute for every run, within `[t0, end)`, where `t0` is now or the end of the running task or hook, rounded up to the minute, and `end` is the end of the night plan. It is feasible when:

1. every run is eligible over its whole duration (`indigo_scheduler_points_ok()`: `@window`, nightly windows, `@ha`, `@twilight`, `@altitude`, the Moon) and starts before `end`;
2. runs do not overlap; a hook due after the start of a run and up to the start of the next one runs after the first one ends and delays the next one, as in the greedy simulation, so a run delays a hook and is never blocked by it;
3. the first run of a task starts at or after its repeat or retry delay, and after the running run of the same task;
4. two runs of a task keep the repeat spacing `@repeat … every`, also when other runs lie between them;
5. runs of urgent tasks keep their COMPACT start, rounded up to the minute;
6. the plan has in total at least as many task runs as the COMPACT plan, and per task at most the runs left. It starts from the runs of the COMPACT plan, which priority decides, rearranges them and adds the runs COMPACT leaves out. Only the seed from the previous plan (see the algorithm) may leave out a COMPACT run, and only one whose priority class is not above the class of a run it keeps; a run of a higher class, or an urgent run, is never dropped for it.

**Algorithm.**

1. The COMPACT plan is computed first. With more than `SCHEDULER_VISIBILITY_MAX_RUNS` = 64 task runs, or a plan longer than 24 hours, it is the plan (one message per night).
2. For every task, whether its run may start and its weighted quality are tabulated for every minute of the plan.
3. Three seeds are searched, in this order: the rest of the previous VISIBILITY plan with the COMPACT runs it lacks inserted where they fit best; the COMPACT order; and the COMPACT runs ordered by the minute of their best quality. A run the previous plan lacks that fits nowhere is left out when its priority class is not above the class of a run of the previous plan, so the runs a plan promised are kept; otherwise that seed is dropped.
4. In each seed the runs COMPACT left out are inserted, one at a time, each where `J` is highest, as long as one fits. Then a local search moves one run to another position, first improvement in a fixed scan order, and keeps a move when the order is feasible and `J` rises; urgent runs are not moved.
5. The best seed wins by the precedence above. When it is infeasible, has fewer runs than COMPACT or the same runs and a `J` not above COMPACT's, the COMPACT plan is the plan. VISIBILITY therefore never plans fewer runs than COMPACT and never a lower `J` at the same runs. Hooks due after the last run follow it, as in COMPACT.

**Start times for a fixed order.** A dynamic programme over the start minute of each run gives the exact best starts: `F_k(g)` is the best `J` of the runs 0 to k with run k at minute g, the maximum over the feasible starts of run k − 1 plus the value of run k at g. The predecessors of a start form one range per stretch between hook minutes, so a run costs time linear in the minutes of the plan. Two adjacent runs of a task need a minimal gap. When other runs separate two runs of a task, the gaps of that span must add up to the spacing the runs between do not fill; the state of the runs of the span is extended by the remaining spacing r, which falls by every gap, and the second run accepts only r = 0. Maxima along the diagonals start + r keep each transition constant in time. Spacings above 120 minutes are counted in units of several minutes, rounded to the safe side. Orders with two overlapping spans or a gap within a span, or with more than 2 000 000 states, are not arranged; an arrangement with a hook due within a span is checked in seconds afterwards and refused. The arrangement of every order is checked against all constraints in seconds before it is used, so a plan never breaks one.

**Budget.** The search counts the cell updates of the dynamic programme and stops at `SCHEDULER_VISIBILITY_BUDGET` = 20 000 000 per plan: the previous plan gets a third, the COMPACT order half of what remains, the last seed the rest. The work is counted, not timed, so a plan is the same on every machine, and there is no randomness.

**At run time.**

- At a decision without an urgent task, when the best task is eligible, the plan is computed if something changed and its first task run that is not running is taken. Before its planned start nothing starts, `REASON` is `<task> starts at HH:MM UTC, planned for better visibility`, and the next check comes at the planned start, so the run starts on time. At its start the task starts. When it cannot start then (a constraint changed, the start was missed), the plan is computed once more; when its first run still cannot start, the best task starts, as in COMPACT. Without a planned task run the best task starts.
- An urgent task starts at once, also during a wait, and preempts a running task as in COMPACT.
- The plan is computed again after every run, seeded with the rest of the previous plan, and after every change (a task uploaded, edited, deleted, reset, enabled or disabled, a window that expires, a change of the settings, the hooks, the night limit, the site or the strategy). While nothing changes, the periodic refresh keeps the plan as long as its first planned run starts at or after now and after the expected end of the running task or hook; the hourly rebuild of the sky and the nights does not count as a change. A long wait therefore costs one plan, not one per check.
- Waiting is idle time. The `IDLE` hook does not start while the next planned run starts within `IDLE_HOOK_DELAY` minutes; when the idle time is reached and the next run is further away, `IDLE` runs, and `RESUME` runs before the planned run.
- `STOP`, `ABORT` and the automatic stop are unchanged. During a wait nothing runs, so `STOP` runs `SHUTDOWN` at once. A planned task is QUEUED, not finished, so it keeps the scheduler running.
- The first plan of a decision, which tells whether the night has work for the [night hooks](#night-hooks), stays a COMPACT plan; when VISIBILITY moves a run into a night without COMPACT work, the night hooks are added and the plan is computed once more.
- Priority classes decide which runs are in the plan; within the plan a high-priority run may come after a normal one when that raises `J`. COMPACT keeps its priority order.
- The debug log has one line per VISIBILITY plan with its result, the runs and `J` of both plans, the work and the time taken.

**Cost.** Measured on an Intel Core i5-9400 with `bench_scheduler_plan` over 1000 random nights of 3 to 10 tasks, planning again after every run: about 3 ms per plan on average, 27 ms at the 99th percentile and 39 ms at most; the largest work of one plan is the budget. A Raspberry Pi 4 is estimated, not measured, at four to ten times slower, about 0.5 s at most.

**Results.** On the same 1000 nights COMPACT makes 5626 runs with a mean quality of 68.7 % (69.7 % weighted by the run duration), VISIBILITY 5879 runs with 79.6 % weighted; no night has fewer runs than COMPACT and no executed run breaks a constraint. The worked example is the night of 19 November 2026 at 42° N, 23° E with six tasks, see the [user guide](../../../indigo_docs/INDIGO_SCHEDULER_GUIDE.md#choosing-a-strategy): COMPACT runs them from dusk at a mean quality of 73.1 %, VISIBILITY at 97.3 %.

#### Meridian flips

**Input.** `Sequencer.js` publishes the meridian flip as `MERIDIAN_FLIP` on the Scripting Agent (`ENABLED` 0 or 1, `USE_SOLVER`, `OFFSET` in hours, −2 to 2), defined when the library is loaded and updated by `set_meridian_flip()`, the last step of `enable_meridian_flip()` and `disable_meridian_flip()`. The agent's client caches `ENABLED` and `OFFSET` (clamped to −2..2) on define and update; before the first one and after the property or the device is deleted the flip counts as disabled. A change of either value increments a generation, and a plan made with an older generation is replaced at the next pass; an update with the same values changes nothing. Every plan copies the cached values once and assumes they hold for the rest of the night, so the flip should be set once, in the `STARTUP` hook, not by the tasks. `USE_SOLVER` is not used because durations stay fixed.

**Rule.** The Imager Agent pauses the batch before the first exposure that would end at an hour angle of `OFFSET` or more, and the flipper flips once the Mount Agent reports `FLIP_REQUIRED` at hour angle 0; a target reached west of the meridian is not paused. A run therefore flips if and only if its hour angle at the start, `HA_start` (from the target grid, advanced at the sidereal rate and normalised to −12..12), is below 0 and `HA_start + d × 1.0027379 / 3600 ≥ OFFSET` (`indigo_scheduler_run_flips()`). With `OFFSET < 0` the capture also idles from `max(HA_start, OFFSET)` until the meridian. Exposure granularity is ignored.

**Cost of a flip.** For a run that flips, `indigo_scheduler_flip_loss()` returns `VISIBILITY_FLIP_PENALTY / 60` hours plus, for `OFFSET < 0`, the wait `−max(HA_start, OFFSET)` hours; it returns 0 without a flip or with the penalty 0, which switches the whole cost off, the wait included. `indigo_scheduler_plan_visibility()` takes it through its `scheduler_flip` argument (`enabled`, `offset` in hours, `penalty` in hours; NULL = none) and subtracts `100 × loss` from the value of every run that is not urgent: an hour lost counts as an hour of full-quality imaging. The same cost is subtracted from the `J` of the COMPACT plan that VISIBILITY compares itself with, so VISIBILITY is not measured against a COMPACT plan that flips for free; the COMPACT plan and the COMPACT selection are unchanged. Urgent runs keep their COMPACT start and are not charged. Run durations do not include the flip; the measured duration of a task already includes past flips.

**Mark.** Under both strategies, while `MERIDIAN_FLIP.ENABLED` is 1, a task run of the plan that flips has `meridian flip` as its reason in [`AGENT_SCHEDULER_PLAN`](#night-plan-preview); the running run is evaluated with its actual start, hooks are never marked. The mark does not depend on the penalty.

**Results.** On the 1000 random nights of `bench_scheduler_plan` with the flip enabled at `OFFSET` 0, the executed runs flip 645 times under COMPACT and 1436 times under VISIBILITY; with the default penalty of 10 minutes VISIBILITY flips 269 times (−81 %), at 78.9 % instead of 79.6 % weighted quality and the same 5879 runs. In the worked example of the user guide the penalty removes all three flips of VISIBILITY for 1.3 points of weighted quality.

### Night plan preview

`AGENT_SCHEDULER_TASK_STATE` lists the tasks in list order, which only breaks score ties. `AGENT_SCHEDULER_CURRENT` shows only the running task. To show the user the expected order of the whole night, the agent publishes a forecast in `AGENT_SCHEDULER_PLAN`.

**Computation.** The agent simulates scheduling from now until the end of the night:

- At night, the night ends at the next sunrise. During the day, it ends at the sunrise that follows the next sunset, so that the plan shows the coming night. It ends 24 hours from now if the sun does not rise or set within the grid. When the scheduler's night for the [night hooks](#night-hooks) ends later, the plan lasts until its end.
- The simulation starts with the running task or [hook](#hooks), assumed to end at its start time plus its [duration](#task-duration), but not earlier than one minute from now. The runs that follow are simulated from that end.
- At each simulated time, the agent applies the [selection](#selection) rules of COMPACT exactly as at run time: delays, expiry, eligibility over the task duration, urgent tasks first, then score. A task queued again after a repeat or retry delay waits behind the urgent tasks queued before it. With `VISIBILITY` this simulation is the COMPACT plan from which the plan of the [strategy](#strategies) is computed, and the plan shows the VISIBILITY plan.
- The selected task is assumed to run for its duration and succeed. Its simulated run counter increases, and `@repeat` intervals are applied.
- When nothing starts, the simulation advances to the time the next task becomes eligible or a hook is due.
- A run that starts before the end of the night is planned even when it lasts beyond it.

The simulation works on a copy of the runtime status and never changes the real task states. It uses only ephemeris arithmetic and runs on the agent's handler queue.

**Items.** Each planned run is one item, in expected run order:

- The label has six columns joined by ` | `: when (`YYYY-MM-DD HH:MM-HH:MM`, UTC), name, run, priority, state and reason, for example `2026-11-12 20:15-23:15 | M31 LRGB | run 1/1 | normal 50 | PLANNED | -`. A task that repeats appears once per planned run, with the run number. The reason of a planned run is `-`, or `meridian flip` for a run that is expected to [flip](#meridian-flips); the gaps of a VISIBILITY plan show as idle time between the runs. A hook has `-` as the run and `hook startup`, `hook shutdown`, `hook night start`, `hook night end`, `hook idle` or `hook resume` as the priority.
- The light is Busy for the running task or hook, with the state `RUNNING`, and Idle for future runs. A running hook is the first item and disappears when the hook ends. A running night hook replaces its planned item, so it is never listed twice.
- Tasks that cannot run before the end of the night follow the planned runs as Alert items with `-` as the time, the state `NOT TONIGHT` and the reason, for example `- | M42 | run 0/1 | normal 50 | NOT TONIGHT | target below the altitude limit`, with the reason `disabled`, `script error` or `no time left tonight`, the last for a task that could run but finds no room after the planned runs. A task whose target can never stand within the global altitude limits has the state `UNREACHABLE` instead of `NOT TONIGHT`, with a reason carrying the numbers: `never rises above the horizon`, `culminates at 15.3°, below the minimum altitude 20°` (or `below the task's @altitude 40°`), `never below 50.2°, above the maximum altitude 45°` or `maximum altitude 30° is not above the task's @altitude 40°`. DONE, FAILED and EXPIRED tasks are not listed.

**Updates.** The plan is recomputed:

- whenever a task or a hook starts or ends;
- after a task is uploaded, edited or deleted, a `TASK_COMMAND`, an urgent submission and a change of `SETTINGS`, `HOOKS` or `NIGHT`;
- when a night hook starts or ends a night;
- on `START`, `STOP` and `ABORT`;
- when the site coordinates change;
- every `IDLE_CHECK_INTERVAL`, so that the times follow the clock, except with a valid [VISIBILITY](#strategies) plan, which is kept while nothing changes.

It is also published while the scheduler is stopped, as the plan that `START` would follow.

**Limits.** The plan is a forecast, not a commitment:

- Failures, retries, tasks that end earlier or later than their duration, urgent tasks and the [unsafe hold](#unsafe-hold) change the real order. The plan is recomputed when they happen. During the hold no run is planned before its earliest end.
- The forecast assumes that the constraints and the site do not change.
- Urgent preemption is not simulated: an urgent task that becomes eligible during a planned run is listed after that run, although it will preempt it.
- A task whose duration is still being estimated is planned with the duration known so far, although the scheduler does not start it before the estimate is known.
- With COMPACT, while nothing runs, the first run starts now, so the labels change every minute and the property is redefined with them. A kept VISIBILITY plan does not change. INDIGO cannot change an item label with an update.

### Run history

`AGENT_SCHEDULER_TASK_STATE` shows only the current state of each task. To show what actually happened, the agent keeps a record of finished runs in `AGENT_SCHEDULER_HISTORY`.

**Entries.** An entry is added whenever a run ends, including hook runs and urgent tasks:

| Result | Light | Meaning |
|---|---|---|
| `succeeded` | Ok | The sequence ended Ok |
| `failed` | Alert | The sequence ended Alert, or the task failed to start |
| `preempted` | Busy | An urgent task interrupted the run |
| `aborted` | Busy | `ABORT` or a delete of the running task |
| `unsafe` | Busy | The [unsafe hold](#unsafe-hold) aborted the run |

The Busy light is used for interrupted runs, so that they stand out from both successes and failures.

A run that does not stop within `STOP_TIMEOUT` keeps the result of what caused the stop: `preempted`, `aborted` or `failed`. The reason of a task run preempted or aborted by `ABORT` is then `did not stop in time`; a deleted task keeps `deleted` and a failed run its failure reason.

The item label has six columns joined by ` | `, `-` for an empty one:

- the UTC start and end times, `YYYY-MM-DD HH:MM-HH:MM`;
- the task name;
- the run number, `run <n>`;
- the kind, `task`, `urgent` or `hook`;
- the result in capitals: `SUCCEEDED`, `FAILED`, `PREEMPTED`, `ABORTED` or `UNSAFE`;
- the reason, the last column; for a failure it is, for a sequence that ended Alert, `SEQUENCE_FAILURE.MESSAGE` of the Scripting Agent, which is the failure message of the step when a step failed and empty when the sequence was aborted by hand outside the scheduler, see [Running a task](#running-a-task); the script error message, for a failed start `the sequence was not started` or `no result from the Scripting Agent`, or `the Scripting Agent or Sequencer.js disappeared` when either disappears during the run.

For example: `2026-11-12 20:15-21:02 | M31 LRGB | run 1 | task | PREEMPTED | by SN 2026abc`.

Entries are oldest first, the newest is last. Expiry is not a run, so an expired task gets no entry; its state remains visible in `AGENT_SCHEDULER_TASK_STATE`.

**Limits.**

- The history keeps the last 200 entries. Older entries are dropped.
- It survives a restart: entries are saved in the configuration and replayed on load, see [Persistence](#persistence).
- Deleting or resetting a task does not remove its entries.
- `AGENT_SCHEDULER_CLEAR_HISTORY` removes all entries and resets the urgent counters of `AGENT_SCHEDULER_STATE`.

### State between tasks

The scheduler does not reset the agents between tasks. Settings made by one task body persist into the next one. Examples are the meridian flip, which stays enabled after a sequence ends, HA limits, filter offsets and the solver filter.

The exceptions are the object name, the folder and the FITS keywords that the agent labels a run with, see [Observation data](#observation-data). Each task body must therefore set up the state it relies on. The meridian flip is the exception: set it once with `enable_meridian_flip()` or `disable_meridian_flip()` in the `STARTUP` hook and leave it unchanged in the tasks, because the night plan uses the current `MERIDIAN_FLIP` for all runs, see [Meridian flips](#meridian-flips). Tasks that change it make the plan approximate, and every change makes the agent plan the night again.

## Urgent preemption

An urgent task is one submitted through `AGENT_SCHEDULER_URGENT_TASK`, or a stored task with `@priority urgent`.

1. The agent queues a handler immediately on submission, and checks pending urgent tasks every `IDLE_CHECK_INTERVAL` while a task runs.
2. An urgent task preempts only if it is **eligible now** over its whole `@duration`. Otherwise it waits like any other task, and the running task is not touched.
3. If no task is running, or the running task is itself urgent or a hook, the eligible urgent task waits at the head of the queue. Urgent tasks never preempt each other, and they never preempt a running hook.
4. If a non-urgent task is running, the agent aborts it as described in [Aborting a sequence](#aborting-a-sequence).
5. The agent marks the preempted task RETRY or FAILED according to its `@preempted` key.
6. The agent starts the urgent task. The urgent task performs its own slew, centering, guiding and capture.
7. When the urgent task ends, normal selection resumes.

An urgent task never bypasses its own constraints. An urgent task below `@altitude` waits until it rises or its window ends.

Frames that the preempted task saved stay on disk. The scheduler does not count them, and a rerun captures its full sequence again.

The [unsafe hold](#unsafe-hold) outranks urgent tasks: it aborts a running urgent task too, and no urgent task starts during the hold.

### Aborting a sequence

Preemption, `ABORT`, deleting a running task and the [unsafe hold](#unsafe-hold) all stop the sequence the same way:

1. Set the Scripting Agent's `AGENT_ABORT_PROCESS.ABORT`. The sequencer aborts the Imager, Mount and solver agents.
2. Abort the related Guider Agent explicitly. The sequencer's abort intentionally leaves active guiding running.
3. Wait, for at most `STOP_TIMEOUT` seconds, until:
   - `SEQUENCE_STATE` is no longer Busy;
   - the `AGENT_START_PROCESS` property of the related Imager, Mount, Guider and solver agents is no longer Busy.
4. If the sequence or an agent is still Busy after `STOP_TIMEOUT`, the agent sends an alert message. It does not start another task: the Scripting Agent refuses a new sequence while the old one runs. The scheduler stops, and its state becomes Alert.

### Observation data

The agent relates the frames to the task that produced them by changing `CCD_LOCAL_MODE` and the FITS headers of the "Imager Agent", the same properties `set_object_name()`, `set_directory()` and `set_fits_header()` of `Sequencer.js` change. It does so itself, not in the wrapper script, because `Sequence.start()` returns immediately: nothing in the script could restore the state after a failure or an abort, while the agent observes the bus and knows when a run ends.

- The agent observes `DIR` and `OBJECT` of `CCD_LOCAL_MODE` of the Imager Agent. When the scheduler starts, it keeps them as the original. The base of the task folders is `AGENT_SCHEDULER_DATA_DIR.BASE`, or the original `DIR` when it is empty, so folders never nest.
- Before a task is sent to the Scripting Agent, in this order and on the same client connection: `OBJECT` is set to the task name (`OBJECT` item on), `DIR` to `<base>/<name>/` (`FOLDER` item on, or `@folder`; `@folder none` suppresses it and leaves `DIR` untouched; the base applies only when a folder is used), and the FITS headers `SCHTASK` (task name), `SCHRUN` (run number) and `SCHNIGHT` (date of the session night, as `%N` of the file name template) are set (`FITS` item on). The name of the folder is the `@folder` value or the task name, with the characters that the CCD driver does not allow in a file name replaced by `_`. If the task has `@observer`, `@pi` or `@project`, the FITS headers `OBSERVER`, `PI_NAME` and `PROPID` are set with them in the same step (`FITS` item on); a key the task does not give sets no header and leaves an existing header of that name alone. After the run the scheduler removes exactly the headers it set. String values carry the FITS quotes, a quote in the value is doubled; the limit of 56 characters keeps the quoted value within the 58 characters that the CCD driver accepts.
- A hook is not labelled: it gets no `OBJECT`, no folder from the `FOLDER` item and no FITS headers, whatever `AGENT_SCHEDULER_DATA` and its header keys say. A hook may deselect or disconnect the camera, for example a shutdown hook with `select_imager_camera("NONE")`. That removes `CCD_LOCAL_MODE` from the Imager Agent while the camera keeps its values, so a restore at the end of the hook could not reach it, and the next start would take the folder of the hook as the original. Only an explicit `@folder <name>` in the header of the hook task sets `DIR` to `<base>/<name>/` for the hook run, and it is restored after the run as for a task; such a hook must not deselect or disconnect the camera. `@folder none` and a hook without `@folder` leave `DIR` alone.
- Steps of the script (`set_object_name()`, `set_directory()`, `set_fits_header()`) run later and win.
- After every task run, whatever its outcome (succeeded, failed, preempted or aborted), including a stop that aborts it, `DIR` and `OBJECT` return to the original and `SCHTASK`, `SCHRUN`, `SCHNIGHT` and the headers set from `@observer`, `@pi` and `@project` are removed with `CCD_REMOVE_FITS_HEADER`. A directory set by a script therefore does not outlive the run. A hook run with `@folder <name>` has `DIR` and `OBJECT` returned in the same way; any other hook run sets nothing, so nothing is restored.
- Nothing is set or restored when the Imager Agent or its `CCD_LOCAL_MODE` is not known. The duration estimate never sets anything. A script that works with another imager agent gets no labels, as for the busy check on the "Imager Agent".
- `AGENT_SCHEDULER_DATA` and `AGENT_SCHEDULER_DATA_DIR` are saved with the other settings.

## Running a task

`Sequencer.js` is compiled into the Scheduler Agent from `indigo_drivers/agent_scripting/library/Sequencer.js`. The build converts it into `Sequencer.js.dat` with `indigo_hexdump`, as the Scripting Agent's `boot.js`: the make rule in `Makefile.inc` whenever the file is missing or older than `Sequencer.js`, the Xcode build phase and the pre-build step in `indigo_agent_scheduler.vcxproj` on every build. The generated file is not checked in. See [MAKEFILES.md](../../../indigo_docs/MAKEFILES.md#embedded-files).

The scheduler starts no task until the Scripting Agent is present. When the Scripting Agent does not yet define `SEQUENCE_STATE`, the scheduler loads the library into it through `AGENT_SCRIPTING_RUN_SCRIPT`. A library that a client loaded already, possibly of another version, is used as it is.

**Library reloads.** Some clients send the whole library again before every script they run. A reload keeps the `indigo_sequencer` object while it has a sequence running, and the flipper while a flip is enabled; only an idle sequencer is replaced. A client that runs a script during a scheduled task therefore does not disturb it.

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
- Busy → Alert means failure, unless the agent itself aborted the sequence. The reason of the failure is `SEQUENCE_FAILURE.MESSAGE`, which `Sequencer.js` publishes before that update: the failure message of the step that ended the sequence, or empty after an abort by hand. The agent forgets the value at the start of a task and at every transition to Busy, so a reason from an earlier run is never reused.

A failed start counts as a task failure.

A sequence started by hand, outside the scheduler, keeps `SEQUENCE_STATE` Busy. The scheduler does not start a task while such a sequence runs. An eligible urgent task aborts it like a preempted task, and the [unsafe hold](#unsafe-hold) aborts one that runs when the hold begins.

## Implementation phases

Phases 1, 2, 3, 5 and 6 are implemented. Phases 4 and 7 are planned.

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
5. Night hooks: the night limit, the `NIGHT_START`, `NIGHT_END` and `RESUME` hooks and the standby state in the module `indigo_agent_scheduler_night.c`, as described in [Night hooks](#night-hooks).
6. Safety: the unsafe hold driven by `AGENT_AUX_SAFETY` of the Auxiliary Agent, with the `UNSAFE` and `SAFE` hooks, `SAFE_DELAY` and `AGENT_SCHEDULER_SAFETY_MISSING`, as described in [Unsafe hold](#unsafe-hold). It replaces the earlier plan of a warning and an alert level with debounce and grace period.
7. Optional extensions:
   - `@timeout minutes`: a hard limit on the length of one run, independent of `@duration`, which stays a planning value. When a run exceeds it, the agent aborts the run the same way as `ABORT` (the sequence, then the Guider Agent, waiting at most `STOP_TIMEOUT`) and counts it as a failure: the history records `timed out`, and `@retry` decides whether the task runs again. Hook tasks and urgent tasks obey it too. Without the key, runs are not limited, as now;
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
- Unit tests of the global altitude limits (`test_agent_scheduler_sky`): the global minimum beating a lower `@altitude` and the reverse, the maximum rejecting points, tasks without `@target` unaffected, and the reachability cases (horizon, empty range, upper culmination below the effective minimum, lower culmination above the maximum) in both hemispheres and without a site.
- Unit tests compare the per-task constraint tables with a direct evaluation of the constraints for random tasks and times, including the failure reasons.
- Unit tests for the night plan simulation cover the score, the order and times, ties, urgent tasks first, the running task first, repeats with intervals, unschedulable tasks, windows, the end of the night and the run limit.
- Unit tests of the [strategies](#strategies) (`test_agent_scheduler_plan`): the run quality; the COMPACT plan equal, run for run, to an independent greedy reference on random queues of 4 to 128 tasks; the owner's night of 19 November 2026 planned for visibility with all six runs, each at least 90 % and a mean of at least 95 %, and Jellyfish centred on its transit within half an hour; 250 seeded random queues (200 of 3 to 10 tasks, 50 crowded ones of 12 to 20) never planning fewer runs per task than COMPACT nor a lower `J` at the same runs; on the same queues every run eligible over its whole duration, after its delay, without overlap and with the repeat spacing; the dynamic programme equal to the enumeration of all start minutes for an implied spacing, adjacent runs, a span across another run and a hook within a gap; urgent runs at their COMPACT starts; an absolute window, a nightly window and `@ha` bounding the starts; night hooks and a running hook; the running task first and its repeat delay; a task without `@target` at its earliest start in its place; on 60 seeded queues a plan computed again after every planned run keeping the rest of the plan unless it finds more runs or a higher `J`; the same plan for the same inputs and the previous plan winning a tie; more than 64 runs giving the COMPACT plan and an order with two overlapping spans not arranged; and 64 targets of 5 minutes with and without windows, 128 tasks and the random queues within the work budget and two seconds. A unit test of the night computation checks that a rebuild with the same site, limit and offsets counts as no change, which keeps a VISIBILITY plan over the hourly rebuild.
- The benchmark `bench_scheduler_plan` (`make -C indigo_test benchmark`) executes 1000 seeded random nights with both strategies, planning again after every run, and reports the runs, the mean and weighted quality, the nights with fewer runs than COMPACT, the constraint violations of the executed runs, the plan changes and the plan time.
- The [night hooks](#night-hooks) are tested in three layers:
  - Unit tests of the night computation (`test_agent_scheduler_night`) on fixed dates and sites: night ends that lie on the civil, nautical and astronomical limits; the three limits nested; both offsets, negative and positive and clamped at ±240 minutes; offsets that leave no night and offsets that merge nights; a summer without astronomical night, polar day and polar night; and the lookups.
  - Unit tests of the hook decisions over synthetic nights: `NIGHT_START` and `NIGHT_END` once per night over three nights; `START` during the day and the night, and `STOP` and `START` within one night; night boundaries passing while a task runs, a task spanning the end of one night and the start of the next, and a task covering a whole night; a night without work; work that appears late; a night that loses its work; nights recomputed with moved ends or for a new site; the polar night; and three real nights with hourly recomputation, where every hook falls within a degree of the limit.
  - A unit test of the plan simulation with hook runs: a hook before a run delays it, a hook due during a run follows it, a hook at the end is planned and a hook after it is not.
  - Integration tests move the stand-in site's longitude so that dusk or dawn falls a few seconds ahead, using the agent's own night computation. They cover: the night hooks at dusk and dawn with their order around the task, the night hooks in the plan, `IDLE` before the night, again after the task and not after `NIGHT_END`; a night without work skipping both hooks with the message, and a task uploaded later in that night running `NIGHT_START` first and `NIGHT_END` at dawn; a night without work and without night hooks staying silent; `RESUME` after `IDLE` and only in standby; `IDLE` without `RESUME` running once per idle period as before; a failed `NIGHT_START` and a failed `RESUME` running `SHUTDOWN` and stopping with Alert; `STARTUP` and `NIGHT_START` naming one task that runs once; a night hook naming a missing task; a day without astronomical night near the pole with the message and the plan item; a running startup hook and a running night start hook shown as the Busy first item of the plan, without a duplicate planned item and with the following run starting at the hook's expected end; a running startup hook that shares its task with the night start hook keeping tonight's planned night start item during the day, and listed once when started at night; a running night end hook without a planned duplicate; and the night limit, offsets and hook items across a restart.
  - The server acceptance test also runs device tasks: an LRGB sequence on the wheel of the simulated camera with named filters, checking the frames per filter and their `FILTER`, `IMAGETYP`, `EXPTIME` and mount position (`OBJCTRA`, `OBJCTDEC`) keywords, a dark and a bias frame; two targets uploaded in the reverse of their priority order, run in priority order with each frame pointing at its own target and a repeated task capturing in every run; and a missing filter and a missing camera failing their tasks, with one retry, no frames and the next task running.
  - The server acceptance test runs the [unsafe hold](#unsafe-hold) with the Weather Station Simulator of the aux simulator selected in the Auxiliary Agent: wind above the `AUX_WIND_THRESHOLDS` `STRONG` threshold during a capture turns `AUX_WEATHER_SAFETY` and `AGENT_AUX_SAFETY` Alert with the wind reason, the scheduler holds, the capture is aborted with an `UNSAFE` history entry and the `UNSAFE` hook runs once; after calm wind and a gust shorter than the evaluation interval, which restarts the delay, the hold ends no earlier than `SAFE_DELAY`, the `SAFE` hook runs, and the task runs again without a used retry and completes, with consistent counters and history.
  - The server acceptance test adds one night with the simulators: a night without work does not run `NIGHT_START`; a task uploaded in the night makes `NIGHT_START` turn the CCD cooler on at 20 °C and unpark the mount; moving the site to noon ends the night, and `NIGHT_END` turns the cooler off and parks the mount.
- A performance test computes the night plan for 128 tasks and checks that it completes within one second on a Raspberry Pi 4.
- Integration tests run the Scheduler Agent with the Scripting Agent and stand-in Mount and Guider Agents; a server acceptance test runs it in `indigo_server` with the Imager, Mount, Guider and Auxiliary Agents and the CCD, mount and aux simulators. Together they cover:
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
  - labels cut to 511 bytes, and list properties redefined only when their items or labels change;
  - a body that fails only when it runs, and a body without `start()`, counting as failures;
  - a `STOP_TIMEOUT` expiry stopping the scheduler without starting another task;
  - `RESET_ALL` and `DELETE_ALL`: their hints, the item order, a running task left alone, the order of pending urgent tasks kept, `DELETE_ALL` rejected while the scheduler runs, the hook tasks kept, and both results across a restart;
  - `RESET_ALL` on finished work: repeat and retry counters cleared, a script error checked again, a disabled task kept disabled, the task name ignored, an empty list, and the work run again after `START`;
  - `DELETE_ALL` rejected while a stop waits for the running task and while the shutdown hook runs, a hook name in a different case protecting its task, the deleted task's property removed, the history unchanged, and the name and slot free again;
  - eligibility end to end, each constraint with a task that waits with its reason and one that runs, and the waiting task starting by itself when the stand-in site is moved: `@altitude` and `@airmass` with targets placed at a computed altitude, `@ha` including a range wrapped across 12h, `@twilight` at noon and midnight, `@moon` with and without `width` and `@illumination` with the site under the moon, both ignored with the moon below the horizon, nightly windows outside, too short and across midnight, a `@duration` that does not fit before the target sets, an unknown site blocking only tasks with sky constraints, and `@enabled no` not overridden by `ENABLE`;
  - the score: a setting target overtaking a rising one of a higher level, the level deciding without the setting term, the list order deciding without both terms, and the altitude term preferring the target closer to its own culmination;
  - the strategies, with a target two hours before its culmination: `COMPACT` by default starting it at once and the settings without `DEFER`; `VISIBILITY` keeping it QUEUED with the reason `… starts at HH:MM UTC, planned for better visibility` and the plan listing it at that start near the culmination, unchanged when the clock passes the next minute; a run planned at the next point of the sky grid starting within two seconds of it; the `IDLE` hook running during a wait longer than `IDLE_HOOK_DELAY` and `RESUME` before the planned task, and not running when the planned run is due within `IDLE_HOOK_DELAY`; an urgent task running at once during the wait and a task without `@target` running as planned; `STOP` during the wait running `SHUTDOWN`; switching back to `COMPACT` starting the task; a failed run planned again after its retry delay while the other runs keep their starts; and the strategy across a restart;
  - the global altitude limits: `WAITING` with `target below the altitude limit` and `target above the altitude limit` and the run after relaxing them, `NOT TONIGHT` and the planned run after relaxing, a minimum at or above the maximum refused with the old values kept, the limits saved and restored, and `UNREACHABLE` for each case with its message told once, re-told on an edit, skipped by `START`, cleared by a change of the limits or the site, and never for a hook task;
  - the limits through the agent: a 16 KiB task text stored, restored and run, one byte more rejected, 128 tasks and the 129th upload and an urgent task rejected;
  - the pauses of `@repeat … every` and `@retry … after`, each start seen when it happens;
  - `START` right after uploads starting the task of the highest priority;
  - the [unsafe hold](#unsafe-hold), with a stand-in Auxiliary Agent that defines `AGENT_AUX_SAFETY` and is attached only by these cases: a running task, a one-off urgent task and a sequence started by hand aborted, the task kept in RETRY without a used retry with `@preempted retry`, the urgent task kept and run after the hold, a task and a one-off urgent task with `@preempted fail` failed and not run after the hold, the urgent one removed and counted in `URGENT_FAILED`, and a sequence started by hand during the hold left alone; a running `SHUTDOWN` and `NIGHT_END` hook finishing; a running `STARTUP`, `IDLE`, `NIGHT_START`, `RESUME` and `SAFE` hook aborted with the result `UNSAFE` and without a stop, the startup hook run again and an aborted `SAFE` hook followed by a new hold with its own `UNSAFE` hook; `NIGHT_END` at dawn during the hold and `NIGHT_START` of a night that started during the hold waiting for its end; `SAFE_DELAY` restarted by an ALERT shorter than the evaluation interval; the `SAFE` hook only in the night with work, standby by day with `NIGHT_START` at dusk, and standby without work with `RESUME` before a later task; failed `UNSAFE` and `SAFE` hooks running `SHUTDOWN` and stopping with Alert, and a missing `UNSAFE` task reported without a stop; missing safety information counted as safe by default and as unsafe when selected (no agent, Idle light, deleted property); a postponed `START` without `UNSAFE` and `STARTUP` after `SAFE_DELAY`; `STOP` and `ABORT` during the hold; the hold across a restart with the restored settings, and a saved safe state older than the delay not postponing `START`; and the labels `hook unsafe` and `hook safe`.
