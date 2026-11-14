# INDIGO Scheduler Agent - User Guide

Revision: 14.11.2026 (draft)

Author: **Rumen G. Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Introduction

The INDIGO Scheduler Agent decides which imaging task runs next, when it runs and under which sky conditions. You give it a list of tasks. Each task says what to observe, when it may be observed and how important it is. The scheduler then runs the tasks unattended, through the night and over as many nights as they need:

- it starts a task only when its constraints (altitude, airmass, hour angle, moon, twilight, time window) hold for the task's whole duration;
- it picks the most important task among the ones that can run now;
- it repeats and retries tasks as you ask;
- it interrupts the running task for an **urgent** task, such as a new supernova or a gamma-ray burst follow-up;
- it runs your own tasks, called hooks, to prepare the equipment when it starts, at the start of every night and after a pause, and to put it away when it stops, during a pause and at the end of every night;
- it shows what it is doing, what it expects to do for the rest of the night, and what it has done.

A task is a short text: a few `// @key value` header lines that tell the scheduler when and how to run the task, followed by an ordinary [Sequencer.js](../indigo_drivers/agent_scripting/library/Sequencer.md) sequence that does the work (slew, focus, guide, capture). The scheduler runs that sequence in the Scripting Agent, the same way as a sequence you start by hand.

The scheduler is controlled entirely through INDIGO properties of the device **Scheduler Agent**. This guide shows how to write tasks, submit them, follow and control the scheduler, and fix common problems, using those properties. The design and the exact rules are documented in [SCHEDULER.md](../indigo_drivers/agent_scheduler/doc/SCHEDULER.md), and every property is listed in [PROPERTIES.md](PROPERTIES.md).

## Before you start

### Agents the scheduler depends on

The Scheduler Agent itself (`indigo_agent_scheduler`) works directly with these agents:

| Agent | Driver | Needed | What the scheduler uses it for |
|---|---|---|---|
| Scripting Agent | `indigo_agent_scripting` | always | Runs every task, estimates task durations, and is told to abort a sequence. The scheduler loads `Sequencer.js` into it when needed. Without it, no task starts and `REASON` is `waiting for the Scripting Agent` |
| Mount Agent | `indigo_agent_mount` | for tasks with `@target` or `@twilight`, and for the night hooks | The site coordinates, from its `GEOGRAPHIC_COORDINATES` property. Without it, or without a site, these tasks wait with `site coordinates unknown`, and the night hooks do not run |
| Guider Agent | `indigo_agent_guider` | optional | Stopped when a task is aborted or preempted, because the sequencer's own abort leaves guiding running |
| Imager Agent and the plate solver agents | `indigo_agent_imager`, `indigo_agent_astrometry`, `indigo_agent_platesolver` | optional | After an abort, the scheduler waits until they are idle before it starts the next task |

**Load them on the same server as the Scheduler Agent.** The scheduler finds the agents by their exact names, such as "Scripting Agent" and "Mount Agent". An agent on another server appears as `Scripting Agent @ obs2` and is not used. Selecting the Scripting Agent is planned.

**Set the site in the Mount Agent.** Its `AGENT_SITE_DATA_SOURCE` property decides where the site comes from. With the default, `HOST` ("Use agent coordinates"), you set the site in the Mount Agent itself, and the agent also writes it to the selected mount and dome:

```
indigo_prop_tool set "Mount Agent.GEOGRAPHIC_COORDINATES.LATITUDE=42.7;LONGITUDE=23.3;ELEVATION=550"
```

With `MOUNT`, `DOME` or `GPS`, the Mount Agent takes the site from that selected device instead, and a site set directly is replaced. The scheduler picks up a new site at its next check, within `IDLE_CHECK_INTERVAL` seconds.

An unconfigured Mount Agent reports latitude 0° and longitude 0°. The scheduler accepts that as a site, so `site coordinates unknown` disappears, but it then computes the sky for the wrong place.

### Agents the tasks depend on

The task bodies are ordinary `Sequencer.js` sequences. They use the Imager, Mount and Guider Agents and a plate solver agent, as any sequence does, and these must be set up as usual, with the camera, mount, guider and other devices selected. The scheduler does not check them: a task that needs a missing agent or device fails when it runs.

### Loading the agents

Name the agents when you start the server:

```
indigo_server indigo_agent_scripting indigo_agent_mount indigo_agent_scheduler
```

or load them into a running server with the command line tool, one driver per command:

```
indigo_prop_tool set "Server.LOAD.DRIVER=indigo_agent_scripting"
indigo_prop_tool set "Server.LOAD.DRIVER=indigo_agent_mount"
indigo_prop_tool set "Server.LOAD.DRIVER=indigo_agent_scheduler"
```

Add `indigo_agent_imager`, `indigo_agent_guider` and a plate solver agent for tasks that capture, guide or solve.

**Times are UTC.** Every time the scheduler accepts or shows (`@window`, the night plan, the run history, `NEXT_START`) is in UTC.

## How to talk to the scheduler

There is no dedicated scheduler window in the INDIGO clients yet. You control the scheduler by setting its properties and watch it by reading them, with two general tools:

- **`indigo_prop_tool`** for everything: uploading and editing tasks, starting and stopping, task commands, deleting, settings and hooks, and reading the scheduler state. You keep each task in a text file and upload it with `set_script`, which sends the file's content as the `TASK` item:

  ```
  indigo_prop_tool set_script "Scheduler Agent.AGENT_SCHEDULER_ADD_TASK.TASK=m31.task"
  ```

  Use `set` only for short values on one line; it splits values at `;` and cannot send a task. `indigo_prop_tool` prints the messages of the Scheduler Agent that answer your change, such as the reason of a rejection. It shows light items only as `ON`/`OFF` without their labels, so read the task list, the night plan and the history in a client.
- **Any INDIGO client that shows all properties**, such as the server's web control panel (`http://<server>:7624/ctrl.html`). It shows the task list, the night plan and the run history with their labels and lights, shows the messages, and lets you press the control switches. It edits text items in a single line, so upload tasks with `indigo_prop_tool`.

All examples assume the server runs on the local computer. For a remote one, add `-r host` after the command word, for example `indigo_prop_tool get -r host ...`; options placed before the command word are not recognised. Add `-s` to see the state of the property you change, for example `[OK]` for an accepted upload and `[ALERT]` for a rejected one.

## A first task in five minutes

This walkthrough needs only the Scheduler and Scripting Agents. The test task waits ten seconds instead of using any equipment.

1. Save this task as `test.task`:

   ```js
   // @task Test
   // @duration 1

   var sequence = new Sequence(scheduler_task.name);
   sequence.wait(10);
   sequence.start();
   ```

2. Upload it:

   ```
   indigo_prop_tool set_script -s "Scheduler Agent.AGENT_SCHEDULER_ADD_TASK.TASK=test.task"
   ```

   An accepted upload ends with `[OK]`. A rejected one ends with `[ALERT]` and a message that names the problem, for example `Scheduler Agent: Task rejected: line 2: @bogus: unknown key`.

   The task appears in the task list `AGENT_SCHEDULER_TASK_STATE` as `Test - QUEUED, 0h01 set`, and as its own property `AGENT_SCHEDULER_TASK_0`.

3. Start the scheduler:

   ```
   indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_CONTROL.START=ON"
   ```

4. Follow it:

   ```
   indigo_prop_tool get "Scheduler Agent.AGENT_SCHEDULER_CURRENT.CURRENT;NEXT;NEXT_START;REASON"
   ```

   While the task runs, `CURRENT` is `Test`. Ten seconds later the task is `DONE`, the history `AGENT_SCHEDULER_HISTORY` shows `... Test #1  succeeded`, and, because nothing is left to do, the scheduler stops by itself with the message `All tasks finished, the scheduler stopped`.

5. Remove the task:

   ```
   indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_DELETE_TASK.NAME=Test"
   ```

## Writing a task

### The shape of a task

```js
// @task       M31 LRGB
// @priority   normal 60
// @target     00:42:44.3 +41:16:09
// @altitude   > 30
// @moon       > 40 width 7
// @twilight   astronomical
// @repeat     2 every 30
// @retry      1 after 15

var sequence = new Sequence(scheduler_task.name);
sequence.enable_meridian_flip(true, 0);
sequence.precise_goto(5, scheduler_task.ra, scheduler_task.dec);
sequence.start_guiding();
sequence.select_filter("L");
sequence.capture_batch(20, 300);
sequence.stop_guiding();
sequence.start();
```

- The **header** is the block of `// @key value` lines at the top. It ends at the first line that is not a header line: a blank line, as above, but also a plain comment such as `// my notes`. Empty `//` lines are allowed inside it. A `// @key` line after the end of the header belongs to the body and is ignored, so keep all header lines together at the top. A header line may be at most 255 characters long.
- The **body** is everything after the header: a normal `Sequencer.js` sequence that ends with `sequence.start()`.
- Because the header consists only of comments, the whole task is valid JavaScript.
- Keys and keywords are not case-sensitive. Each key may appear only once. Only `@task` is required.

### The `scheduler_task` object

Before the body runs, the scheduler defines a `scheduler_task` object from the header, so the body does not have to repeat the name or the coordinates:

| Field | Meaning |
|---|---|
| `scheduler_task.name` | The task name from `@task`. Name the sequence after it, as in `new Sequence(scheduler_task.name)` |
| `scheduler_task.ra`, `scheduler_task.dec` | The `@target` coordinates, RA in hours and Dec in degrees, J2000. Defined only with `@target` |
| `scheduler_task.run` | The number of this run, starting at 1 |
| `scheduler_task.estimate` | Defined only while the scheduler estimates the duration, see [Task duration](#task-duration) |
| `scheduler_task.token` | An internal identifier of the run |

### Header keys

| Key | Syntax | Meaning | Default |
|---|---|---|---|
| `@task` | `name` | Name of the task, 1–40 characters, unique (not case-sensitive) | required |
| `@priority` | `urgent`, `high`, `normal` or `low`, optionally followed by a level 0–100 | Importance, see [Priorities](#priorities) | `normal 50` |
| `@target` | `ra dec` | J2000 coordinates. RA in hours (`hh:mm:ss.s` or decimal), Dec in degrees (`±dd:mm:ss` or decimal). A task with a target also needs the target above the horizon | none |
| `@altitude` | `> degrees` | Minimum target altitude, 0–90 | off |
| `@airmass` | `< value` | Maximum airmass, 1–100 | off |
| `@ha` | `from .. to` | Allowed hour angle in hours, −12 to +12. When `from` is greater than `to` the range wraps across 12h | off |
| `@moon` | `> degrees`, optionally `width days` | Minimum distance from the Moon, 0–180°, with a width of 0.5–30 days, see [Moon avoidance](#moon-avoidance) | off |
| `@illumination` | `< percent` | Maximum Moon illumination, 0–100 | off |
| `@twilight` | `none`, `civil`, `nautical` or `astronomical` | `civil`, `nautical` and `astronomical`: the Sun must be below −6°, −12° or −18°. `none`: no limit, the task may also run in daylight | `none` |
| `@window` | `[start] .. [end]` | Time window in UTC, see [Time windows](#time-windows) | open |
| `@duration` | `minutes` | Expected run time, 1–1440 | estimated, see [Task duration](#task-duration) |
| `@repeat` | `count`, optionally `every minutes` | Number of successful runs, 1–10000, with an optional pause of 0–100000 minutes between them | `1` |
| `@retry` | `count`, optionally `after minutes` | Number of reruns after failures, 0–100, with an optional delay of 0–100000 minutes | `0` |
| `@preempted` | `retry` or `fail` | What happens to this task when an urgent task interrupts it | `retry` |
| `@enabled` | `yes` or `no` | A disabled task is kept but never selected | `yes` |

`@altitude`, `@airmass`, `@ha`, `@moon` and `@illumination` need `@target`. The comparison signs are required: write `> 30`, not `30`. `>=` and `<=` are accepted and mean the same as `>` and `<`.

**A task starts only if it can finish.** All constraints must hold for the whole duration of the task, from the moment it would start until it would end. A three-hour task on a target that sets below 30° in two hours waits for another night.

The check is made when the task starts, with the expected duration, in five-minute steps. A run that takes longer than expected is not stopped when its target sets or its window ends. To stop a sequence at such a limit, use steps like `break_if_target_altitude_below()` and `break_at()` in the body.

### Priorities

`@priority` has a class and a level:

```
// @priority high 80
```

- The class decides first. Any `high` task is preferred to any `normal` task, and any `normal` task to any `low` task.
- Within a class, the scheduler compares a score made of the level and two bonuses:
  - the level, 0–100;
  - a **setting** bonus of up to `100 × SETTING_WEIGHT`, largest for a task whose constraints stop holding soonest, for example a target that is setting or a window that is closing;
  - an **altitude** bonus of up to `100 × ALTITUDE_WEIGHT`, largest for a target near the zenith. A task without `@target` gets none.

  With the default weights of 1, the two bonuses together can add up to 200 points, more than the whole level range. A `normal 40` target that is high and about to set can therefore run before a `normal 60` target. To make the level decide strictly, set `SETTING_WEIGHT` and `ALTITUDE_WEIGHT` to 0 in [Settings](#settings); to make the sky position count more, raise them. The bonuses never cross a class: with the maximum weights of 4, the level and the bonuses together add at most 900 points, less than the 1000-point gap between classes.
- When the scores are equal, the task higher in the task list wins. You can move tasks in the list, see [Changing the order](#changing-the-order).

`urgent` is different from the other classes:

- An urgent task runs before every other task, as soon as its own constraints hold.
- If another, non-urgent task is running, the scheduler interrupts it, see [Urgent tasks](#urgent-tasks).
- Several urgent tasks run in the order in which they were submitted. They never interrupt each other. A stored urgent task that becomes ready again after its repeat or retry delay, or after `RESET`, queues behind the urgent tasks already waiting.
- The level is ignored for urgent tasks.

The scheduler decides only between tasks: a running task is never interrupted for a more important normal task, only for an urgent one.

### Time windows

`@window` limits when a task may run. Both ends are optional, and both must use the same form:

```
// @window 2026-11-20 21:00 .. 2026-11-21 02:30     absolute, UTC
// @window 2026-11-20 21:00 ..                      from a time on
// @window .. 2026-11-21 02:30                      until a time
// @window 22:00 .. 03:00                           every night, across midnight
```

- `YYYY-MM-DD HH:MM` is an absolute time. When the end of an absolute window has passed, the task expires and is not run again.
- `HH:MM` alone is a nightly window that repeats every day. It never expires. Nightly windows count UTC days: `22:00 ..` means from 22:00 until midnight UTC, and `.. 03:00` from midnight until 03:00 UTC. For a window across midnight give both ends, as in `22:00 .. 03:00`.
- The whole run must fit in the window.

### Moon avoidance

`@moon > 40` keeps the target at least 40° from the Moon.

With `width`, the limit follows the brightness of the Moon (Lorentzian moon avoidance):

```
// @moon > 40 width 7
```

The full limit, 40°, applies at full moon. It falls to half, 20°, seven days before or after full moon, and to about 7° at new moon. A small width suits narrowband imaging, which tolerates moonlight; a large width keeps the limit high for most of the month.

The moon distance and `@illumination` are ignored while the Moon is below the horizon.

### Task duration

The scheduler needs to know how long a task takes, to check the constraints before it starts the task and to plan the night. It uses the first of these that is available:

1. `@duration` from the header (`set` in the task list);
2. the measured duration of the last successful run (`measured`);
3. an estimate computed from the sequence (`estimated`);
4. 60 minutes (`default`).

**Estimate.** After you upload or edit a task, the scheduler runs its body once in estimation mode: the body builds the sequence, but `sequence.start()` only counts the steps instead of starting them. The duration is the total exposure time plus `FRAME_OVERHEAD` seconds per frame, `OPERATION_OVERHEAD` seconds per operation, and the fixed waits. Operations are the steps that move or calibrate equipment: slews and centering, focusing, guiding calibration and start, park, unpark, home and the dome steps. Steps whose length cannot be known, `wait_until()`, `wait_until_solar_altitude_below()` and `wait_until_target_altitude_above()`, make the estimate impossible; the scheduler then uses 60 minutes until the task has run once, and tells you to set `@duration`. It also uses 60 minutes when the estimate gets no answer within ten seconds.

**Measurement.** After every successful run the scheduler remembers how long the run really took, including meridian flips, autofocus retries and slow downloads, and uses that from then on. An edit of the task forgets the measurement.

A task is not started while its estimate is pending. Set `@duration` when you know the time a task needs, or when it must stay fixed.

**The duration is not a time limit.** The scheduler uses the duration only to decide whether a task may start, to place it in the night plan and to show it in the task list. It never stops a run that takes longer. A task with `@duration 10` whose sequence needs 25 minutes runs for 25 minutes; the night plan then moves the later runs back. A running task stops only when:

- its sequence ends;
- you press `ABORT` or delete the task;
- an urgent task preempts it;
- a step of its own sequence ends it early.

To keep a run within a limit, put the limit into the body with the sequencer's checkpoint steps:

- `break_at(time)` skips forward to the next `resume_point()`, or ends the sequence when none follows, once `time` has passed;
- `break_at_ha(limit)` does the same when the hour angle is past `limit`;
- `break_if_target_altitude_below(...)` does the same when the target sinks below an altitude.

```js
// @task       M31 until 02:00
// @target     00:42:44.3 +41:16:09
// @duration   120

var sequence = new Sequence(scheduler_task.name);
sequence.precise_goto(5, scheduler_task.ra, scheduler_task.dec);
sequence.select_filter("L");
sequence.repeat(24, function() {
	sequence.break_at("02:00");
	sequence.capture_batch(1, 300);
});
sequence.resume_point();
sequence.start();
```

Checkpoints act only where the sequence reaches them: an exposure, a slew or a focusing run in progress finishes first. A run that ends early through a checkpoint ends Ok and counts as a successful run, see [Repeats and retries](#repeats-and-retries). A time limit enforced by the scheduler itself, `@timeout`, is planned; see [SCHEDULER.md](../indigo_drivers/agent_scheduler/doc/SCHEDULER.md#implementation-phases).

### Repeats and retries

```
// @repeat 3 every 60
// @retry  2 after 10
```

- `@repeat 3 every 60` runs the task three times, with at least 60 minutes between the end of one run and the start of the next. Other tasks can run in between. The task is DONE after three successful runs.
- `@retry 2 after 10` allows two reruns after failures, each at least ten minutes after the failure. The retries are shared by all runs of the task, also by its repeats: once two failures have been retried, the next failure makes the task FAILED. `RESET` gives the retries back. The task list shows `retries left`.
- Every run, also a rerun, executes the whole sequence from the beginning. Frames saved by an interrupted run stay on disk.
- A run counts as successful when the sequence ends in the Ok state, even if it ended early, for example through `break_if_target_altitude_below()`.

### Rules for the body

- **Build, then start.** The body runs once on every upload and edit to estimate the duration. Up to `sequence.start()`, it must only build the sequence: it must not change properties, start timers or send messages itself. Everything that acts on the equipment belongs in sequence steps.
- **Set up everything you rely on.** The scheduler does not reset the agents between tasks, so settings made by one task stay for the next one. Set the filter, the binning and other options in every task, and call `enable_meridian_flip()` or `disable_meridian_flip()` explicitly.
- **End the sequence in the right state.** A sequence that ends Ok counts as a success, and one that ends Alert as a failure. Use the sequencer's own steps to stop early, such as `break_if_target_altitude_below()` and `break_at()`.
- **Name the sequence after the task** with `new Sequence(scheduler_task.name)`.

### More examples

A target that must be high, away from a bright Moon, in astronomical darkness:

```js
// @task       NGC 7000 Ha
// @priority   normal 40
// @target     20:59:17 +44:31:44
// @altitude   > 40
// @moon       > 30 width 3
// @twilight   astronomical
// @duration   120

var sequence = new Sequence(scheduler_task.name);
sequence.enable_meridian_flip(true, 0);
sequence.precise_goto(5, scheduler_task.ra, scheduler_task.dec);
sequence.focus(5);
sequence.start_guiding();
sequence.select_filter("Ha");
sequence.capture_batch(12, 600);
sequence.stop_guiding();
sequence.start();
```

A variable star observed every night near the meridian, every 30 minutes, ten times:

```js
// @task       SS Cyg
// @priority   high 20
// @target     21:42:42.8 +43:35:10
// @ha         -2 .. 2
// @twilight   nautical
// @window     20:00 .. 04:00
// @repeat     10 every 30
// @retry      3 after 5
// @duration   5

var sequence = new Sequence(scheduler_task.name);
sequence.disable_meridian_flip();
sequence.precise_goto(3, scheduler_task.ra, scheduler_task.dec);
sequence.select_filter("V");
sequence.capture_batch(3, 60);
sequence.start();
```

A task without a target, for calibration frames at dusk:

```js
// @task       Sky flats
// @priority   high 90
// @window     16:30 .. 17:10
// @duration   20

var sequence = new Sequence(scheduler_task.name);
sequence.select_frame_type("Flat");
sequence.select_filter("L");
sequence.capture_batch(20, 1);
sequence.start();
```

## Submitting tasks

### Uploading a task

You upload a task by writing its text to the `TASK` item of `AGENT_SCHEDULER_ADD_TASK`. The name comes from `@task`. Keep the task in a file, for example `m31.task`:

```js
// @task       M31 LRGB
// @priority   normal 60
// @target     00:42:44.3 +41:16:09
// @altitude   > 30
// @twilight   astronomical

var sequence = new Sequence(scheduler_task.name);
sequence.enable_meridian_flip(true, 0);
sequence.precise_goto(5, scheduler_task.ra, scheduler_task.dec);
sequence.start_guiding();
sequence.select_filter("L");
sequence.capture_batch(20, 300);
sequence.stop_guiding();
sequence.start();
```

and upload it:

```
indigo_prop_tool set_script -s "Scheduler Agent.AGENT_SCHEDULER_ADD_TASK.TASK=m31.task"
```

The file name and its extension do not matter to the scheduler; `.task` only reminds you what the file is.

**What happens on upload:**

1. The scheduler checks the header. If it is wrong, the upload is rejected: `AGENT_SCHEDULER_ADD_TASK` goes to Alert and a message names the problem, for example `Task rejected: line 4: @altitude: expected '> degrees'`. Nothing is stored.
2. If it is right, the task is stored. It gets a property `AGENT_SCHEDULER_TASK_<n>`, labelled with the task name, and a line in the task list.
3. The scheduler estimates the duration. If the body has an error, even a syntax error, the task is marked `SCRIPT ERROR`, the message tells you the error, and the task is not run until you fix it.

**Limits:** at most 128 tasks, counting hook tasks and pending urgent tasks, at most 16 KiB of text per task, task names of at most 40 characters, and the body must not be empty. A task name must be unique, also among urgent tasks.

### Urgent tasks

There are two ways to make a task urgent:

- **Submit it through `AGENT_SCHEDULER_URGENT_TASK`.** The task is urgent whatever its `@priority`, and it is used once: it runs, appears in the history and is removed, whether it succeeded, failed, was aborted or expired. `@repeat` and `@retry` have no effect. It is not stored and is lost when the server restarts. This is the way for a one-off follow-up, and for programs that receive alerts.
- **Store a task with `@priority urgent`** through `AGENT_SCHEDULER_ADD_TASK`. It is kept like any stored task, with repeats and retries.

To submit an urgent task, upload its file to `AGENT_SCHEDULER_URGENT_TASK` instead of `AGENT_SCHEDULER_ADD_TASK`. For example, `sn2026abc.task`:

```js
// @task       SN 2026abc
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

```
indigo_prop_tool set_script -s "Scheduler Agent.AGENT_SCHEDULER_URGENT_TASK.TASK=sn2026abc.task"
```

**What an urgent task does:**

1. If it can run now, that is, all its own constraints hold for its whole duration, and a non-urgent task is running, the scheduler interrupts that task: it aborts the exposure, the slew, the focusing and the guiding, and waits until the agents are idle.
2. The interrupted task is marked according to its `@preempted` key: `retry` puts it back in the queue to run again from the beginning, after its `@retry` delay if it has one, without using one of its retries; `fail` marks it FAILED.
3. The urgent task runs. Its own sequence does the slew, centering and capture.
4. Normal scheduling continues afterwards.

An urgent task never ignores its own constraints. An urgent target below its `@altitude` waits until it rises, and the running task is not touched meanwhile. An urgent task also does not interrupt:

- a hook task;
- a task the scheduler is letting finish after `STOP`;
- anything before its own duration estimate has finished, which takes a few seconds after the submission.

An urgent task submitted while the scheduler is stopped is accepted and waits; the message `Urgent task queued, but the scheduler is not running` reminds you to start the scheduler.

## Running the scheduler

### Start, stop and abort

`AGENT_SCHEDULER_CONTROL` has three switches:

| Switch | What it does |
|---|---|
| `START` | Runs the `STARTUP` hook, if set, then starts scheduling. During a night with work, the `NIGHT_START` hook runs right after `STARTUP`, see [Several nights](#several-nights) |
| `STOP` | Lets the running task finish, then runs the `SHUTDOWN` hook and stops |
| `ABORT` | Aborts the running task at once, runs the `SHUTDOWN` hook and stops. A stored task is marked RETRY without using a retry; a one-off urgent task is discarded |

```
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_CONTROL.START=ON"
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_CONTROL.STOP=ON"
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_CONTROL.ABORT=ON"
```

The scheduler also stops by itself, after its `SHUTDOWN` hook, when every task is DONE, FAILED, EXPIRED, disabled or has a script error. With no tasks at all, or only hook tasks, it keeps running, ready for urgent tasks. A task that you add or reset before the shutdown hook starts cancels this automatic stop.

The scheduler always starts stopped after a server restart. Press `START` again.

### Hooks

Hooks are stored tasks that the scheduler runs at special moments, for example to unpark the mount when it starts, to park it while it waits, and to cool the camera at the start of every night. You create the hook tasks like any other task and give their names in `AGENT_SCHEDULER_HOOKS`:

| Item | When the task runs |
|---|---|
| `STARTUP` | On `START`, before the first task. If it fails, the scheduler stops with Alert |
| `SHUTDOWN` | When the scheduler stops: on `STOP`, on `ABORT` and on the automatic stop. It does not run after a failed `STARTUP` hook or after a stop timeout, and it is skipped with a message when another sequence occupies the Scripting Agent |
| `IDLE` | Once, after the scheduler has found nothing to run for `IDLE_PARK_AFTER` minutes, for example to park the mount while waiting for clouds or for a target to rise. It does not run again while the equipment is in standby, see [Several nights](#several-nights) |
| `NIGHT_START` | When a night with work starts, see [Several nights](#several-nights). If it fails, the `SHUTDOWN` hook runs and the scheduler stops with Alert |
| `NIGHT_END` | When that night ends |
| `RESUME` | Before a task that starts after the `IDLE` or `NIGHT_END` hook put the equipment away. If it fails, the `SHUTDOWN` hook runs and the scheduler stops with Alert |

For example, save a park task as `park.task`:

```js
// @task Park

var sequence = new Sequence(scheduler_task.name);
sequence.park();
sequence.start();
```

and an unpark task as `unpark.task`:

```js
// @task Unpark

var sequence = new Sequence(scheduler_task.name);
sequence.unpark();
sequence.start();
```

Upload both and name them as hooks:

```
indigo_prop_tool set_script "Scheduler Agent.AGENT_SCHEDULER_ADD_TASK.TASK=park.task"
indigo_prop_tool set_script "Scheduler Agent.AGENT_SCHEDULER_ADD_TASK.TASK=unpark.task"
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_HOOKS.STARTUP=Unpark;SHUTDOWN=Park;IDLE=Park;RESUME=Unpark"
```

The mount is unparked on `START`, parked when nothing can run for `IDLE_PARK_AFTER` minutes, unparked again before the next task, and parked when the scheduler stops.

- Hooks name their tasks: renaming a hook task with an edit detaches it from the hook.
- A hook task is never scheduled as an ordinary task. Its header constraints are ignored, so they never keep it from running. The task list shows it as `STARTUP HOOK`, `SHUTDOWN HOOK`, `IDLE HOOK`, `NIGHT START HOOK`, `NIGHT END HOOK` or `RESUME HOOK`.
- One task may serve several hooks, for example `Park` as `SHUTDOWN`, `IDLE` and `NIGHT_END`. The task list shows the first of them. When two hooks naming the same task are due one right after the other, such as `STARTUP` and `NIGHT_START` on `START` during the night, the task runs once.
- Hook runs appear in the history, marked `(hook)`.
- To remove a hook, set its item to an empty value. `indigo_prop_tool` cannot send an empty value, so clear the field in a client such as the web control panel.

### Several nights

The scheduler can run unattended over several nights: leave it started, and the tasks that are not finished continue the next night. `STARTUP` runs only on `START` and `SHUTDOWN` only when the scheduler stops, so for the start and the end of every night there are the night hooks.

**The night.** `AGENT_SCHEDULER_NIGHT` sets how dark it must be for the scheduler's night: `CIVIL` (Sun below −6°), `NAUTICAL` (below −12°, the default) or `ASTRONOMICAL` (below −18°). `NIGHT_START_OFFSET` and `NIGHT_END_OFFSET` in `AGENT_SCHEDULER_SETTINGS` move its start and end by up to 240 minutes; negative is earlier. Use a negative start offset to cool the camera before the night begins:

```
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_NIGHT.NAUTICAL=ON"
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_SETTINGS.NIGHT_START_OFFSET=-30"
```

The night only times the night hooks. Each task keeps its own `@twilight`; a task without it may also run in daylight.

**What happens each night:**

- When the night starts and a task can run in it, the `NIGHT_START` hook runs, for example to cool the camera, open the roof and unpark.
- When no task can run that night, `NIGHT_START` is skipped and the message `No task can run tonight, the night start hook is skipped` tells you why. If work appears later that night, because you upload or reset a task, submit an urgent task or a constraint clears, `NIGHT_START` runs then, before that task.
- When the night ends, `NIGHT_END` runs, for example to park, close the roof and warm the camera up. It runs only after a `NIGHT_START` of the same night.
- Night hooks never interrupt a task. A boundary that passes while a task runs is handled when the task ends. To stop a task at dawn, give it `@twilight` or a checkpoint such as `break_if_solar_altitude_above()`.
- `START` during the night runs `STARTUP`, then `NIGHT_START` right away. `STOP`, `ABORT` and the automatic stop run only `SHUTDOWN`, also at night.

**Standby and resume.** The `IDLE` and `NIGHT_END` hooks put the equipment in standby. While it is in standby, the `IDLE` hook does not run again, so nothing is parked twice and nothing runs during the day after `NIGHT_END`. Before the next task, the `RESUME` hook prepares the equipment again, for example to unpark and cool the camera after the `IDLE` hook parked and warmed it. A successful `NIGHT_START` also ends the standby, so the first task of a night does not run `RESUME` too. Without a `RESUME` hook a task starts directly; the Mount Agent unparks the mount before a slew, and the dome when dome slaving is enabled.

**Failures.** A failed `NIGHT_START` or `RESUME` leaves the equipment half prepared, so the scheduler runs `SHUTDOWN` and stops with Alert. A failed `NIGHT_END` or `IDLE` is only reported.

**No night.** Near the poles the Sun may stay above the limit, or below it, for days. With a night hook set, the scheduler tells you `The Sun does not go below the night limit within a day, the night hooks do not run` or `The Sun does not rise above the night limit, the night does not end`, and the night plan shows `No night at the night limit within a day`.

**Example.** An observatory that cools the camera and opens the dome half an hour before nautical dusk, parks while no task can run, and closes at dawn. Save the evening preparation as `evening.task`:

```js
// @task Evening

var sequence = new Sequence(scheduler_task.name);
sequence.enable_cooler(-10);
sequence.dome_open();
sequence.unpark();
sequence.start();
```

the pause as `park.task`:

```js
// @task Park

var sequence = new Sequence(scheduler_task.name);
sequence.park();
sequence.start();
```

and the end of the night as `morning.task`:

```js
// @task Morning

var sequence = new Sequence(scheduler_task.name);
sequence.park();
sequence.dome_close();
sequence.disable_cooler();
sequence.start();
```

Upload the three tasks and set the hooks and the offset:

```
indigo_prop_tool set_script "Scheduler Agent.AGENT_SCHEDULER_ADD_TASK.TASK=evening.task"
indigo_prop_tool set_script "Scheduler Agent.AGENT_SCHEDULER_ADD_TASK.TASK=park.task"
indigo_prop_tool set_script "Scheduler Agent.AGENT_SCHEDULER_ADD_TASK.TASK=morning.task"
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_SETTINGS.NIGHT_START_OFFSET=-30"
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_HOOKS.NIGHT_START=Evening;RESUME=Evening;IDLE=Park;NIGHT_END=Morning;SHUTDOWN=Morning"
```

A night then runs like this:

| Time | What happens |
|---|---|
| 30 minutes before nautical dusk | `Evening` cools the camera, opens the dome and unparks |
| Nautical dusk and later | The tasks run as their constraints allow |
| No task can run for `IDLE_PARK_AFTER` minutes | `Park` parks the mount; the camera stays cool and the dome open |
| A task can run again | `Evening` prepares again (it only unparks, the rest is already done), then the task runs |
| Nautical dawn | `Morning` parks, closes the dome and turns the cooler off, after the running task has finished |
| During the day | Nothing; the next evening starts again with `Evening`, if a task can run that night |
| `STOP` at any time | `Morning` runs as the `SHUTDOWN` hook |

`enable_cooler()` waits until the camera reaches the temperature, so `Evening` can take several minutes; the start offset gives it that time. `dome_open()` and `dome_close()` need a dome selected in the Mount Agent; leave them out without a dome. Keep hook tasks short and safe to repeat: `RESUME` may run several times a night.

### Settings

`AGENT_SCHEDULER_SETTINGS`:

| Item | Unit | Default | Range | Meaning |
|---|---|---|---|---|
| `IDLE_CHECK_INTERVAL` | s | 30 | 1–3600 | How often the scheduler checks the constraints while it waits, and checks for urgent tasks while a task runs |
| `IDLE_PARK_AFTER` | min | 30 | 0–1440 | Time without an eligible task before the `IDLE` hook runs |
| `STOP_TIMEOUT` | s | 60 | 1–3600 | Time allowed for an aborted sequence and its agents to stop |
| `START_TIMEOUT` | s | 10 | 1–600 | Time allowed for a started task's sequence to begin |
| `SETTING_WEIGHT` | — | 1 | 0–4 | How strongly tasks whose constraints end soon are preferred within a priority class, see [Priorities](#priorities) |
| `ALTITUDE_WEIGHT` | — | 1 | 0–4 | How strongly high targets are preferred within a priority class |
| `FRAME_OVERHEAD` | s | 10 | 0–600 | Time added per frame in the duration estimate (download, saving, dithering) |
| `OPERATION_OVERHEAD` | s | 120 | 0–3600 | Time added per slew, centering, focus or guiding start in the duration estimate |
| `NIGHT_START_OFFSET` | min | 0 | −240–240 | Shift of the night start for the night hooks; negative is earlier, see [Several nights](#several-nights) |
| `NIGHT_END_OFFSET` | min | 0 | −240–240 | Shift of the night end for the night hooks; negative is earlier |

```
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_SETTINGS.IDLE_CHECK_INTERVAL=10;IDLE_PARK_AFTER=20"
```

The tasks, their states and counters, the settings, the hooks, the night limit and the history are saved in the scheduler's configuration and restored when the server starts.

## Watching the scheduler

### At a glance: state and current task

`AGENT_SCHEDULER_STATE` tells you whether the scheduler runs, and counts the tasks:

| Property state | Meaning |
|---|---|
| Idle | Not started since the server started |
| Busy | Running |
| Ok | Stopped, after `STOP` or after all tasks finished |
| Alert | Stopped after `ABORT` (also pressed while stopped), a failed `STARTUP`, `NIGHT_START`, `RESUME` or `SHUTDOWN` hook, or a stop timeout |

Its items count the tasks: `DONE` the finished ones, `FAILED` the failed and expired ones, and `TOTAL` all entries of the task list, including hook tasks and pending urgent tasks.

`AGENT_SCHEDULER_CURRENT` says what the scheduler does now:

| Item | Meaning |
|---|---|
| `CURRENT` | The running task or hook, empty when none runs |
| `NEXT` | The task the scheduler expects to run next |
| `NEXT_START` | When `NEXT` can start, in UTC (`YYYY-MM-DDTHH:MM:SS`) |
| `REASON` | Why nothing runs now: the reason of the first waiting task in the list, see [Reasons](#reasons) |

```
indigo_prop_tool get_state "Scheduler Agent.AGENT_SCHEDULER_STATE"
indigo_prop_tool get "Scheduler Agent.AGENT_SCHEDULER_STATE.DONE;FAILED;TOTAL"
indigo_prop_tool get "Scheduler Agent.AGENT_SCHEDULER_CURRENT.CURRENT;NEXT;NEXT_START;REASON"
```

### The task list

`AGENT_SCHEDULER_TASK_STATE` has one light item per task, in list order. The label shows the task's name, state, completed runs of a repeated task, retries left and duration, for example:

```
M31 LRGB - REPEAT, run 1/2, retries left 1, 1h52 estimated
SN 2026abc (urgent) - RUNNING, 0h30 set
Park - SHUTDOWN HOOK
```

| State | Light | Meaning |
|---|---|---|
| QUEUED | Idle | Ready to run |
| WAITING | Idle | Its constraints do not hold now; `NEXT_START` and the night plan show when they will |
| RUNNING | Busy | Its sequence is executing |
| RETRY | Idle | Failed or interrupted, will run again after the retry delay |
| REPEAT | Idle | Completed a run, more runs to go after the repeat interval |
| DONE | Ok | All runs completed |
| FAILED | Alert | Failed with no retries left, or interrupted with `@preempted fail` |
| EXPIRED | Alert | The end of its `@window` passed before it completed |
| DISABLED | Idle | `@enabled no`, or disabled by command |
| SCRIPT ERROR | Alert | The body has an error; fix it with an edit |

### The night plan

`AGENT_SCHEDULER_PLAN` shows what the scheduler expects to do for the rest of the night, one item per planned run, in order:

```
20:14–20:44  SN 2026abc (urgent)
20:44–22:36  M31 LRGB #1 (normal 60)
23:36–01:28  M31 LRGB #2 (normal 60)
01:28–03:28  NGC 7000 Ha (normal 40)
05:10–05:12  Morning (night end hook)
M42 — target below the altitude limit
NGC 891 — disabled
```

- Each run shows its expected UTC start and end, the task, the run number of a repeated task and the priority. The night hooks appear at their expected times, marked as hooks.
- The running task has a Busy light, planned runs an Idle light.
- After the runs come the tasks that will not run tonight, with an Alert light and the reason. Typical reasons are a constraint that never holds tonight (see [Reasons](#reasons)), `disabled`, `script error`, or `no time left tonight` for a task that could run but finds no room after the others.
- During the day the plan shows the coming night. When the night of the night hooks ends after sunrise, the plan lasts until it ends. It is shown also while the scheduler is stopped, as the plan that `START` would follow.

The plan is a forecast. It assumes that every run succeeds and takes its expected duration, and it does not show where an urgent task will interrupt a run. The scheduler recomputes it whenever something changes and at least every `IDLE_CHECK_INTERVAL`.

### The history

`AGENT_SCHEDULER_HISTORY` lists the finished runs, newest first, up to 200:

```
2026-11-21 01:28–03:31  NGC 7000 Ha #1  succeeded
2026-11-20 21:10–21:40  SN 2026abc (urgent) #1  succeeded
2026-11-20 20:44–21:10  M31 LRGB #1  preempted: by SN 2026abc
2026-11-20 20:02–20:04  Evening (hook) #1  succeeded
```

| Result | Light | Meaning |
|---|---|---|
| `succeeded` | Ok | The sequence ended Ok |
| `failed` | Alert | The sequence ended Alert, or the task could not start. A start failure or a JavaScript error is followed by its reason; for a sequence that ended Alert, look at the Scripting Agent's messages |
| `preempted` | Busy | An urgent task interrupted the run |
| `aborted` | Busy | `ABORT` or a delete of the running task. When the sequence then does not stop within `STOP_TIMEOUT`, the reason says `did not stop in time` |

The history survives restarts. To clear it:

```
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_CLEAR_HISTORY.CLEAR=ON"
```

### Messages

The scheduler reports events as INDIGO messages of the device "Scheduler Agent", which clients show in their message log:

| Message | When |
|---|---|
| `Starting task 'M31 LRGB'` | A task starts |
| `Task 'M31 LRGB' completed`, `Task 'M31 LRGB' completed run 1 of 2` | A run succeeded |
| `Task 'M31 LRGB' failed: ...`, `..., it will be retried` | A run failed |
| `Task 'M31 LRGB' preempted by an urgent task, it will be retried`, `... and failed` | Interrupted by an urgent task |
| `Task 'M31 LRGB' aborted`, `Task 'M31 LRGB' aborted and deleted` | `ABORT`, or a delete of the running task |
| `Task 'M31 LRGB' expired` | Its window ended |
| `Task 'M31 LRGB' has a script error: ...` | The estimate found an error in the body |
| `The duration of task 'M31 LRGB' cannot be estimated, ...` | Set `@duration` |
| `Starting hook 'Park'`, `Hook 'Park' completed` | A hook runs |
| `The scheduler stops after the running task` | `STOP` while a task runs |
| `All tasks finished, the scheduler stopped`, `Scheduler stopped`, `Scheduler aborted` | The scheduler stopped |
| `The startup hook failed, the scheduler stopped` | The `STARTUP` hook failed |
| `The night start hook failed, the scheduler stopped`, `The resume hook failed, the scheduler stopped` | The `NIGHT_START` or `RESUME` hook failed; `SHUTDOWN` ran |
| `No task can run tonight, the night start hook is skipped` | A night without work, see [Several nights](#several-nights) |
| `The night start hook 'Evening' names no task`, also for the other hooks | A hook item names a task that does not exist |
| `The Sun does not go below the night limit within a day, the night hooks do not run` | No night at the night limit, for example near the poles in summer |
| `The Sun does not rise above the night limit, the night does not end` | A polar night; `NIGHT_END` does not run until it ends |
| `The sequence or an agent did not stop in time, the scheduler stopped` | An aborted sequence did not stop within `STOP_TIMEOUT` |

A rejected request puts the property you changed into the Alert state, and a message gives the reason, for example `Task 'M42' not found`. The messages of property updates, such as the stop messages of `AGENT_SCHEDULER_STATE`, arrive the same way as the other messages.

### Reasons

`REASON` in `AGENT_SCHEDULER_CURRENT` and the Alert items of the night plan explain why a task does not run:

| Reason | Meaning |
|---|---|
| `target below the horizon` | The target is below 0° |
| `target below the altitude limit` | `@altitude` |
| `airmass too high` | `@airmass` |
| `hour angle out of range` | `@ha` |
| `too close to the moon` | `@moon` |
| `moon too bright` | `@illumination` |
| `too bright (twilight)` | `@twilight` |
| `window not open yet`, `outside of the nightly window` | `@window` |
| `window ends before the task can finish`, `nightly window ends before the task can finish` | The run does not fit in `@window` |
| `site coordinates unknown` | No Mount Agent is loaded on this server, or it has no site coordinates |
| `beyond the forecast` | The constraints would have to hold beyond the next 48 hours |
| `estimating the task duration` | The estimate has not finished yet |
| `no task is ready`, `no tasks`, `all tasks finished` | Nothing to run |
| `another sequence is running` | A sequence started by hand occupies the Scripting Agent |
| `loading Sequencer.js`, `waiting for the Scripting Agent` | The scheduler is preparing the Scripting Agent |
| `stopping the running sequence` | An aborted sequence is still stopping |

## Managing tasks

### Commands

Commands use two properties: first write the task name to `AGENT_SCHEDULER_TASK_NAME`, then switch on the command in `AGENT_SCHEDULER_TASK_COMMAND`.

```
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_TASK_NAME.NAME=M31 LRGB"
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_TASK_COMMAND.DISABLE=ON"
```

| Command | What it does |
|---|---|
| `RESET` | Puts the task back to QUEUED and clears its run and retry counters. Use it to run a DONE, FAILED or EXPIRED task again, or to retry a task after a script error. A running task cannot be reset |
| `DISABLE` | Keeps the task but never selects it. A running task finishes its run first |
| `ENABLE` | Undoes `DISABLE`. A task with `@enabled no` in its header stays disabled until you edit the header |
| `MOVE_UP`, `MOVE_DOWN` | Moves the task in the list. The list order decides only between otherwise equal tasks |

### Changing the order

The priority and the sky position, not the list order, decide which task runs. The list order matters only when two tasks have the same score. To prefer a task, raise its `@priority` level, or its class when the level is not enough; with the default weights the sky bonuses can outweigh a difference in level, see [Priorities](#priorities). To change the order of equal tasks, use `MOVE_UP` and `MOVE_DOWN`.

### Editing a task

Every stored task has a property `AGENT_SCHEDULER_TASK_<n>` in the group "Tasks", labelled with the task name. Its `TASK` item holds the full text. The number `<n>` is only a handle that can change after a restart, so find a task by its label:

```
indigo_prop_tool list -e "Scheduler Agent" | grep -A3 "AGENT_SCHEDULER_TASK_[0-9]"
```

```
Name : Scheduler Agent.AGENT_SCHEDULER_TASK_0 (RW, TEXT_VECTOR)
State: OK
Group: Tasks
Label: M31 LRGB
```

To edit the task, change its file and upload it to that property:

```
indigo_prop_tool set_script -s "Scheduler Agent.AGENT_SCHEDULER_TASK_0.TASK=m31.task"
```

- An edit is checked like an upload. An invalid edit is rejected and the stored task stays unchanged.
- An edit may rename the task by changing `@task`, as long as the new name is unique.
- An edit keeps the task's state and counters, but forgets its measured duration. To run a finished task again, use `RESET`.
- An edit of a running task takes effect when the task ends.

Keep your task files: they are the easiest copy to edit. `indigo_prop_tool get` prints only the first 80 characters of a long text, so to see the full text of a stored task, open its property in a client such as the web control panel.

### Deleting a task

```
indigo_prop_tool set "Scheduler Agent.AGENT_SCHEDULER_DELETE_TASK.NAME=M31 LRGB"
```

- This deletes stored tasks and pending urgent tasks.
- A running task is aborted first; the history records the run as `aborted`.
- The history entries of a deleted task are kept.

## What happens when the server restarts

- Stored tasks, with their states and counters, the settings, the hooks, the night limit and the history are restored.
- Finished work is not repeated: DONE, FAILED and EXPIRED tasks keep their state.
- A task that was running when the server stopped is restored as RETRY, without using a retry.
- Repeat and retry delays are not kept: a task waiting in REPEAT or RETRY can run as soon as the scheduler starts.
- Urgent tasks submitted through `AGENT_SCHEDULER_URGENT_TASK` are lost. Stored tasks with `@priority urgent` are kept.
- The scheduler starts stopped. Press `START`. If you press it during a night with work, `STARTUP` runs and then `NIGHT_START`, so the equipment is prepared again.
- Standby is not kept: after `START` the first task runs without `RESUME`.

## Troubleshooting

| Problem | What to check |
|---|---|
| A task never starts | `REASON` and the night plan say why. A task waits until all its constraints hold for its whole duration; a shorter `@duration` or wider limits help |
| Every task waits with `site coordinates unknown` | Load the Mount Agent (`indigo_agent_mount`) on the same server as the Scheduler Agent and set its site (`GEOGRAPHIC_COORDINATES`), see [Agents the scheduler depends on](#agents-the-scheduler-depends-on) |
| The upload is rejected | The message of the Scheduler Agent names the line and the problem |
| A task shows `SCRIPT ERROR` | The message names the JavaScript error. Fix the body with an edit, or `RESET` the task after fixing an agent the body needed |
| A task shows `1h00 default` | The duration cannot be estimated from the sequence; set `@duration` |
| `another sequence is running` | A sequence started by hand occupies the Scripting Agent. Stop it, or submit an urgent task, which interrupts it |
| The scheduler stopped with Alert | Check the messages: `ABORT`, a failed `STARTUP`, `NIGHT_START`, `RESUME` or `SHUTDOWN` hook, or a sequence that did not stop within `STOP_TIMEOUT` |
| The night start hook does not run | No task can run that night (see the message and the night plan), the hook names no task, there is no night at the night limit, or the site is unknown. It runs later if a task becomes able to run |
| The night start hook runs too late for the camera to cool | Set a negative `NIGHT_START_OFFSET`, for example −30 |
| The night end hook runs long after dawn | A task was still running; night hooks never interrupt a task. Give the tasks `@twilight`, or end them with `break_if_solar_altitude_above()` |
| The equipment stays parked after a pause | Set a `RESUME` hook, or make every task prepare what it needs |
| The idle hook does not run again | After `IDLE` or `NIGHT_END` the equipment is in standby; `IDLE` runs again after a task or the night start hook has run |
| An urgent task does not interrupt the running task | Its own constraints do not hold yet, its duration estimate is still running, the running task is a hook, or a `STOP` is pending; see `REASON` |
| A failed run shows no reason in the history | The scheduler does not yet receive the failure message of a sequence; the Scripting Agent's messages show why it failed |
| A finished task should run again | `RESET` it |
| The tasks run in an unexpected order | The order is decided by the priority class, the level and the sky bonuses, not by the list; see [Priorities](#priorities) |

## Property reference

All properties belong to the device "Scheduler Agent". [PROPERTIES.md](PROPERTIES.md) lists them with their items and [SCHEDULER.md](../indigo_drivers/agent_scheduler/doc/SCHEDULER.md) describes their exact behaviour.

| Property | Type | Items | Use |
|---|---|---|---|
| `AGENT_SCHEDULER_ADD_TASK` | text | `TASK` | Upload a task |
| `AGENT_SCHEDULER_TASK_<n>` | text | `TASK` | One stored task: read to download, write to edit |
| `AGENT_SCHEDULER_URGENT_TASK` | text | `TASK` | Submit a one-off urgent task |
| `AGENT_SCHEDULER_DELETE_TASK` | text | `NAME` | Delete a task |
| `AGENT_SCHEDULER_TASK_NAME` | text | `NAME` | The task that `AGENT_SCHEDULER_TASK_COMMAND` applies to |
| `AGENT_SCHEDULER_TASK_COMMAND` | switch | `RESET`, `ENABLE`, `DISABLE`, `MOVE_UP`, `MOVE_DOWN` | Task commands |
| `AGENT_SCHEDULER_CONTROL` | switch | `START`, `STOP`, `ABORT` | Run the scheduler |
| `AGENT_SCHEDULER_STATE` | number | `DONE`, `FAILED`, `TOTAL` | Scheduler state and task counters |
| `AGENT_SCHEDULER_CURRENT` | text | `CURRENT`, `NEXT`, `NEXT_START`, `REASON` | What runs now and next |
| `AGENT_SCHEDULER_TASK_STATE` | light | one per task | The task list |
| `AGENT_SCHEDULER_PLAN` | light | one per planned run | The night plan |
| `AGENT_SCHEDULER_HISTORY` | light | one per finished run | The run history |
| `AGENT_SCHEDULER_CLEAR_HISTORY` | switch | `CLEAR` | Clear the history |
| `AGENT_SCHEDULER_SETTINGS` | number | see [Settings](#settings) | Settings |
| `AGENT_SCHEDULER_HOOKS` | text | `STARTUP`, `SHUTDOWN`, `IDLE`, `NIGHT_START`, `NIGHT_END`, `RESUME` | Hook task names |
| `AGENT_SCHEDULER_NIGHT` | switch | `CIVIL`, `NAUTICAL`, `ASTRONOMICAL` | The night limit of the night hooks |

`AGENT_SCHEDULER_ADD_TASK.STATUS` and `AGENT_SCHEDULER_RESTORE_HISTORY` are used only to restore the configuration; leave them alone.
