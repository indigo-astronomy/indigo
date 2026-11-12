# INDIGO Scheduler

Revision: 12.11.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

This document is the design of the INDIGO Scheduler Agent. The agent decides which imaging task runs next, when it runs and under which sky conditions, and it can interrupt the running task for an urgent one. It is a design document. Property names, limits and defaults are subject to change until the agent is implemented.

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
| Completion | repeat N, until a time, loop | events done or end time | accepted frames reached | shots done | plan done, "best efforts" | sequence ends Ok; repeat N with interval, "best effort" |
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
- **Device control stays in `Sequencer.js`.** The scheduler does not duplicate any device logic and requires no change to `Sequencer.js`. The only device actions the scheduler performs itself are aborts.

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

  No moon phase helper exists yet. The moon age and illumination are derived from the Sun–Moon elongation.
- Any client can submit an urgent task with a single property change. This includes AIN, the web UI, MCP clients and a future GCN/VOEvent bridge.

The Scheduler Agent is a filter agent. Its related agents are:

- the Scripting Agent, which is required;
- the Mount Agent, which provides the site coordinates through `GEOGRAPHIC_COORDINATES`;
- the Imager, Guider and plate solver agents, which the scheduler aborts on preemption and abort.

The task bodies are expected to use the same agents, which is the default of `Sequence.start()`. A safety or weather device is added in a later phase.

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
// @duration   180
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

Before the task text, the scheduler sends one line that defines a `scheduler_task` object from the validated header:

```js
var scheduler_task = { name: "M31 LRGB", ra: 0.712306, dec: 41.269167, run: 1 };
```

- `ra` is in hours and `dec` in degrees, both J2000. They are defined only when `@target` is present.
- `run` is the number of the run, starting at 1.

The body uses these values, so the name and the coordinates are written only once, in the header. Naming the sequence after the task also lets the scheduler recognise its sequence in `SEQUENCE_NAME`. A task run by hand outside the scheduler must define `scheduler_task` itself or use literal values.

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
| `@task` | `name` | Display name, 1–127 characters, no control characters, unique among stored and urgent tasks, compared case-insensitively | required |
| `@priority` | `urgent` \| `high` \| `normal` \| `low` [`level`] | Priority class and level 0–100 within the class. The level is ignored for `urgent` | `normal 50` |
| `@target` | `ra dec` | J2000 coordinates. RA in hours (`hh:mm:ss.s` or decimal) and Dec in degrees (`±dd:mm:ss` or decimal) | none |
| `@altitude` | `> degrees` | Minimum target altitude, 0–90 | off |
| `@airmass` | `< value` | Maximum target airmass, ≥ 1 | off |
| `@ha` | `from .. to` | Allowed hour-angle range in hours, −12 to +12. `from` greater than `to` wraps across 12h | off |
| `@moon` | `> degrees` [`width days`] | Minimum moon separation, see [Lorentzian moon avoidance](#lorentzian-moon-avoidance) | off |
| `@illumination` | `< percent` | Maximum moon illumination, 0–100; ignored while the moon is below the horizon | off |
| `@twilight` | `none` \| `civil` \| `nautical` \| `astronomical` | Sun must be below 0°, −6°, −12° or −18° | `none` |
| `@window` | `[start] .. [end]` | UTC time window, see below | open |
| `@duration` | `minutes` | Expected run time, 1–1440; all constraints must hold for this whole interval before the task starts | `60` |
| `@repeat` | `count` [`every minutes`] | Number of successful runs, 1–10000, with an optional pause between them | `1` |
| `@retry` | `count` [`after minutes`] | Number of reruns after a failure, 0–100, with an optional delay | `0` |
| `@preempted` | `retry` \| `fail` | What happens to the task when an urgent task preempts it | `retry` |
| `@best_effort` | `yes` \| `no` | An expired task that completed at least one run becomes DONE instead of EXPIRED | `no` |
| `@enabled` | `yes` \| `no` | A disabled task is kept but never selected | `yes` |

The constraints `@altitude`, `@airmass`, `@ha`, `@moon` and `@illumination` require `@target`. A header that uses them without `@target` is invalid.

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

The full limit D applies at full moon and falls off towards new moon. W is the half-width in days.

### Validation

Task text comes from clients and is untrusted input. Only the header is validated; the body is arbitrary JavaScript with the same trust level as `AGENT_SCRIPTING_RUN_SCRIPT`.

The agent validates the header on every upload and edit:

- `@task` is required and must be unique.
- Unknown keys, repeated keys, malformed values and out-of-range values are errors.
- The body must not be empty.
- The task text is limited to 64 KiB.
- At most 128 tasks are stored, the same limit as the Scripting Agent's scripts.

An invalid task is rejected. The property goes to the Alert state and the message names the line and the problem, for example `line 4: @altitude: expected '> degrees'`. The stored task list is not changed.

Script errors in the body are reported by the Scripting Agent when the task runs and count as a task failure.

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

   QUEUED, WAITING, RETRY, REPEAT -- window end passed --> EXPIRED (or DONE with @best_effort)
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
| `AGENT_SCHEDULER_ADD_TASK` | text | `TASK` | Upload a task. The name is taken from `@task`. Creates `AGENT_SCHEDULER_TASK_<n>` |
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

The agent also publishes the standard filter-agent properties, including `FILTER_RELATED_AGENT_LIST`.

`AGENT_SCHEDULER_TASK_NAME` and `AGENT_SCHEDULER_TASK_COMMAND` form a two-step command, which is a common INDIGO idiom. Two clients that issue commands at the same moment can interfere. If both properties are changed in one client action, the command applies to the name sent with it.

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

The agent persists its tasks in its own INDIGO configuration file, exactly as the Scripting Agent persists its scripts. It uses the standard `indigo_save_property()` mechanism and does not open any other file.

The configuration file is `Scheduler_Agent.config` in the INDIGO configuration folder: `~/.indigo` on Linux and macOS, and the INDIGO folder under `%APPDATA%` on Windows. The usual profile and port suffixes apply.

**Saving.** The agent rewrites the whole file after every change that affects persisted data:

- a task added, edited or deleted;
- a `TASK_COMMAND`;
- a change of `SETTINGS` or `HOOKS`;
- every task state transition and every new history entry;
- a history clear.

It does not save while it is replaying its configuration. The file contains, in this order:

1. one `AGENT_SCHEDULER_ADD_TASK` change per stored task, in list order. Before saving, each `AGENT_SCHEDULER_TASK_<n>` property is temporarily renamed to `AGENT_SCHEDULER_ADD_TASK`, as `save_config()` does in the Scripting Agent;
2. one `AGENT_SCHEDULER_RESTORE_STATUS` change per stored task. Its items are `NAME`, `STATE`, `RUNS`, `RETRIES_USED` and `DISABLED`. The property exists only for this replay, is never defined for clients, and is accepted only from the configuration reader;
3. `AGENT_SCHEDULER_SETTINGS` and `AGENT_SCHEDULER_HOOKS`;
4. one `AGENT_SCHEDULER_RESTORE_HISTORY` change per history entry, oldest first. Its items are `TASK`, `RUN`, `START`, `END`, `RESULT` and `REASON`. Like `AGENT_SCHEDULER_RESTORE_STATUS`, it is accepted only from the configuration reader and never defined for clients.

**Loading.** The configuration is loaded when the agent is attached, the same way as for other filter agents. Each saved change is replayed through the agent's own change handler with the configuration reader as the client:

- An `AGENT_SCHEDULER_ADD_TASK` change validates the header and creates the task, as an upload does.
- A replayed task that fails validation, for example after a grammar change in a newer version, is skipped. The agent sends a message naming the task, and the rest of the configuration still loads.
- An `AGENT_SCHEDULER_RESTORE_STATUS` change sets the status of the task with that name. A status without a matching task is ignored.

**After a restart:**

- Finished work is not repeated: DONE, FAILED and EXPIRED tasks keep their state, and the counters are kept.
- A task that was RUNNING when the server stopped is restored as RETRY without consuming a retry.
- WAITING tasks are restored as QUEUED and re-evaluated.
- Pending urgent tasks submitted through `AGENT_SCHEDULER_URGENT_TASK` are lost. A stored task with `@priority urgent` is kept.
- The scheduler itself always starts stopped. Scheduling resumes only after `START`.

**Moving tasks between systems.** Clients export a task by reading `AGENT_SCHEDULER_TASK_<n>.TASK` and saving the text to a file, and import one by sending a file's text to `AGENT_SCHEDULER_ADD_TASK`. The configuration file is internal to the agent and is not an exchange format.

## Scheduling

All scheduler work runs on the agent's handler queue. Bus callbacks only queue work and never block.

### Scheduler control

- `START` runs the `STARTUP` hook, then begins scheduling. If the hook fails, the scheduler stops and its state becomes Alert.
- `STOP` lets the running task finish, then runs the `SHUTDOWN` hook and stops.
- `ABORT` aborts the running task as described in [Aborting a sequence](#aborting-a-sequence) and marks it RETRY without consuming a retry. It then runs the `SHUTDOWN` hook and stops.
- When every stored task is DONE, FAILED, EXPIRED or DISABLED and no urgent task is pending, the scheduler runs the `SHUTDOWN` hook and stops.

### Hooks

The `STARTUP`, `SHUTDOWN` and `IDLE` hooks name stored tasks.

- A hook task runs outside the state machine. Its constraints, state and counters are not evaluated or changed, so it can run every night.
- A task named as a hook is never selected as an ordinary task.
- A hook that fails is reported by a message. Only a failed `STARTUP` hook stops the scheduler.
- The `IDLE` hook runs once per idle period.

### When a decision is made

While no task is running, the agent selects the next task:

- after the previous task ends;
- after a task is uploaded, edited, enabled or reset;
- on `START`;
- every `IDLE_CHECK_INTERVAL` while it waits for constraints.

While a task is running, the agent checks every `IDLE_CHECK_INTERVAL` only whether an urgent task is pending and eligible; see [Urgent preemption](#urgent-preemption). It makes no other scheduling decisions. Limits inside a task, such as the target setting below the horizon, are the task body's job, through `Sequencer.js` steps like `break_if_target_altitude_below()` and `break_at()`.

### Selection

1. **Delays.** RETRY and REPEAT tasks whose delay has elapsed return to QUEUED.
2. **Expiry.** Tasks whose absolute `@window` end has passed become EXPIRED, or DONE with `@best_effort` after at least one completed run.
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

- The night ends at the next sunrise, or 24 hours from now if the sun does not set.
- The simulation starts with the running task, assumed to end at its start time plus `@duration`.
- At each simulated time, the agent applies the [selection](#selection) rules exactly as at run time: delays, expiry, eligibility over `@duration`, urgent tasks first, then score.
- The selected task is assumed to run for its `@duration` and succeed. Its simulated run counter increases, and `@repeat` intervals are applied.
- When nothing is eligible, the simulation advances to the time the next task becomes eligible.

The simulation works on a copy of the runtime status and never changes the real task states. It uses only ephemeris arithmetic and runs on the agent's handler queue.

**Items.** Each planned run is one item, in expected run order:

- The label shows the expected UTC start and end, the task name and its priority, for example `20:15–23:15  M31 LRGB (normal 50)`. A task that repeats appears once per planned run, with the run number.
- The light is Busy for the running task and Idle for future runs.
- Tasks that cannot run before the end of the night follow the planned runs as Alert items, with the reason, for example `M42 — below 30° all night` or `NGC 7000 — disabled`. DONE, FAILED and EXPIRED tasks are not listed.

**Updates.** The plan is recomputed:

- whenever a task starts or ends;
- after a task is uploaded, edited or deleted, a `TASK_COMMAND`, an urgent submission and a `SETTINGS` change;
- on `START`, `STOP` and `ABORT`;
- every `IDLE_CHECK_INTERVAL` while the scheduler waits.

It is also published while the scheduler is stopped, as the plan that `START` would follow.

**Limits.** The plan is a forecast, not a commitment:

- Failures, retries, tasks that end earlier or later than `@duration`, urgent tasks and safety stops change the real order. The plan is recomputed when they happen.
- The forecast assumes that the constraints and the site do not change.

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
- for a failure, the reason: the last alert message of the sequence, `script error`, or `not started`.

For example: `2026-11-12 20:15–21:02  M31 LRGB #1  preempted by SN 2026abc`.

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

To run a task, the agent sends the `scheduler_task` line followed by the task's full text through `AGENT_SCRIPTING_RUN_SCRIPT`. The header is ignored as comments, and the body builds the sequence and calls `start()`. The task does not need to be stored in the Scripting Agent.

The agent watches the result:

- `AGENT_SCRIPTING_RUN_SCRIPT` going to Alert means the script failed to run, for example because of a syntax error. The start fails immediately.
- A transition of `SEQUENCE_STATE` from not Busy to Busy, observed after the `RUN_SCRIPT` change together with `SEQUENCE_NAME` equal to the task name, marks the start. A body that names its sequence differently is still accepted on the state transition alone. If it is not observed within `START_TIMEOUT` seconds, the start fails, for example because the body never calls `start()`.
- After the start, Busy → Ok means success.
- Busy → Alert means failure, unless the agent itself aborted the sequence.

A failed start counts as a task failure.

A sequence started by hand, outside the scheduler, keeps `SEQUENCE_STATE` Busy. The scheduler does not start a task while such a sequence runs. An eligible urgent task aborts it like a preempted task.

## Implementation phases

1. This design document.
2. Scheduler Agent core in `indigo_drivers/agent_scheduler/`:
   - header parser and validation;
   - task store and persistence;
   - properties;
   - constraint engine;
   - selection;
   - urgent preemption;
   - scheduler control.

   The agent is registered in the server, the build and the project files, and documented in `PROPERTIES.md`.
3. Scoring weights, repeat and retry, startup, shutdown and idle hooks, Lorentzian moon avoidance, night plan preview, run history.
4. Safety: a related AUX safety or weather device. A warning prevents new tasks from starting. An alert aborts and runs the `SHUTDOWN` hook. Both have a debounce delay and a grace period.
5. Optional extensions:
   - task upload, download and editing in the web UI;
   - image grading;
   - mosaics;
   - a GCN/VOEvent bridge that writes `AGENT_SCHEDULER_URGENT_TASK`.

## Testing

- Unit tests for the header parser cover:
  - every key and its defaults;
  - case-insensitivity, whitespace, CRLF and byte-order mark handling;
  - every invalid input.
- Unit tests for the constraint engine use fixed dates and sites with known sun, moon and target positions, including the Lorentzian limit.
- Integration tests run the Scheduler Agent with the Scripting Agent and the CCD, mount and guider simulators. They cover:
  - upload, download, edit, rename and persistence across restarts, including restored counters;
  - task ordering, and a task uploaded during a run not interrupting it;
  - a task waiting for its window, and a task not started when its duration does not fit;
  - urgent preemption: the exposure is aborted, guiding stops, the agents become idle, the urgent task runs, and the preempted task becomes RETRY and later reruns, or becomes FAILED;
  - an urgent task that is not yet eligible not preempting the running task;
  - a second urgent task waiting for the first;
  - an urgent task submitted while the scheduler is stopped;
  - expiry and best effort;
  - the night plan preview: expected order and times of several tasks with different windows and priorities, repeats, unschedulable tasks with reasons, and recomputation after an upload, an urgent submission and a task ending early;
  - the run history: one entry per finished run with the correct result and reason for success, failure, failed start, preemption and abort, the entry limit, persistence across restarts and clearing;
  - a body with a syntax error and a body without `start()` counting as failures;
  - a `STOP_TIMEOUT` expiry stopping the scheduler without starting another task.
