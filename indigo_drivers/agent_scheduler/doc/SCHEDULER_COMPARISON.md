# INDIGO Scheduler Agent compared with other schedulers

*Author: Claude Opus 5.5 (Anthropic), November 2026.*

The comparison is about features. The INDIGO scheduler is not released yet, so field experience is not compared.

This document compares the INDIGO Scheduler Agent with the schedulers most often used in amateur and small professional observatories:

- **Ekos Scheduler** (KStars/Ekos);
- **N.I.N.A. Target Scheduler** (plugin for N.I.N.A.);
- **Sequence Generator Pro** (SGP);
- **Voyager RoboTarget**;
- **ACP Scheduler** (ACP Expert);
- **APT SessionCraft** (session control of Astro Photography Tool).

A separate section, [Professional schedulers](#professional-schedulers), compares it with the schedulers of professional observatories: LCO, ESO VLT, Gemini, ALMA, Rubin Observatory, ZTF and RTS2.

APT is different from the others: it has no scheduler that chooses between targets. SessionCraft runs imaging plans from triggers: at a time or a night event (end of astronomical night, end of deep-sky darkness), when the target altitude crosses a limit, after another trigger by tag, or when the safety monitor reports unsafe conditions. Only one plan runs at a time, and when several triggers fire the one with the highest priority runs.

The INDIGO side describes the agent as it is implemented now: the user guide [INDIGO_SCHEDULER_GUIDE.md](../../../indigo_docs/INDIGO_SCHEDULER_GUIDE.md) and the design [SCHEDULER.md](SCHEDULER.md). The other schedulers are described from their public documentation and their widely known behaviour, not from tests run for this comparison; details can differ between their versions.

## Summary

The INDIGO scheduler is the most flexible and the easiest to automate of the group:

- tasks are scripts;
- urgent tasks really interrupt the running work;
- it runs headless and is controlled entirely through INDIGO properties;
- its night plan is visible in advance;
- the `VISIBILITY` strategy plans the whole night.

It is behind in goal-driven progress: frame counts, image grading and per-filter rules.

Its safety handling is on a par with the best of the group: an unsafe report aborts everything, runs an `UNSAFE` hook, and the scheduler resumes only after the conditions have been safe for a set time. The safety signal itself comes from the Auxiliary Agent, which computes `AGENT_AUX_SAFETY` from `AUX_WEATHER_SAFETY` of the selected aux devices; that is outside the scheduler and is not scored here.

The user interface is not part of this comparison. The agent provides the scheduling and its properties; presenting them is the job of the INDIGO client, and AIN Imager provides the user interface for the scheduler.

## Where INDIGO is stronger

### 1. Urgent tasks interrupt the running work

A task with `@priority urgent`, or one submitted through `AGENT_SCHEDULER_URGENT_TASK`:

- aborts the running exposure, slew, focus and guiding;
- runs at once;
- leaves the interrupted task retried or failed, according to that task's `@preempted` key.

Of the others, only ACP has the same behaviour, through its target-of-opportunity interface. N.I.N.A. Target Scheduler, Voyager and SGP change targets only at their next decision point. Ekos re-plans greedily but has no explicit urgent class. APT checks its triggers only after the current exposure ends, so a running exposure is never interrupted, and it has no urgent class; its unsafe-conditions trigger overrides the others, but it is a safety reaction, not a way to submit a new target.

An urgent task is one property write. Alert feeds such as GCN or VOEvent can therefore submit one without any client-side logic.

### 2. A task is one plain-text document

A task is a `// @key value` header followed by an ordinary `Sequencer.js` body. The whole text is valid JavaScript, which has these consequences:

- it can be run by hand as an ordinary sequence;
- it can be kept under version control and compared with diff;
- it can be shared by mail or in a repository.

The body can use everything the Sequencer offers, including loops, conditions, focusing, meridian flips and custom steps. The other schedulers mostly offer a fixed list of exposure plans per target:

| Scheduler | Where a task lives |
|---|---|
| ACP | plan text files, not full programs |
| Ekos | two files, a job list `.esl` and a sequence `.esq` |
| N.I.N.A. Target Scheduler | a database edited in the UI |
| Voyager | a database edited in the UI |
| SGP | its own sequence format |
| APT | plans in the Plan Editor and triggers in SessionCraft, both set up in the UI |

**Why this makes INDIGO the most flexible.** The rankings below score "flexibility of a task" as how much a single task can do, and INDIGO scores 3 in every one of them:
- **A task is a program, not a list.** The body is JavaScript with variables, loops and conditions. It can react to what happens during the run, for example repeat a focus when the HFD grows, change the exposure by the measured sky brightness, or take a different branch when a plate solve fails.
- **It reaches the whole bus.** Besides the Sequencer steps, a task can read and set any property of any device or agent on the INDIGO bus, so it can drive a dome, a flat panel, a spectrograph or a custom device that the scheduler knows nothing about.
- **Hooks are tasks too.** Startup, shutdown, night start and end, idle, resume, unsafe and safe hooks are written in the same language and stored in the same list, so preparing and putting away the observatory needs no second tool.
- **Scheduling and execution stay separate.** The header holds only the scheduling fields; the body is unaware of the scheduler. The same text runs by hand for testing and runs unchanged under the scheduler.

The others give less freedom per task:
- **ACP (2):** a plan language with directives for filters, repeats, dithering, focus and calibration, and custom scripts called from a plan, but no conditions or loops of its own.
- **Voyager (2):** RoboTarget runs fixed shot sequences per target; DragScript adds visual programming with conditions, but for the night as a whole, not per target.
- **N.I.N.A. Target Scheduler (2):** exposure plans per filter, with custom instructions and triggers around the scheduler container in the N.I.N.A. advanced sequencer.
- **Ekos, SGP and APT (1):** a fixed list of capture jobs or plans per target.
- **RTS2 (2):** a script per target in its own simple command language.
- **Professional systems (1):** observation blocks or requests are declarative and run through fixed, validated templates. That is a deliberate choice for shared telescopes, where an observer must not run arbitrary code, not a lack of engineering.

### 3. A task starts only if it can finish

All constraints must hold for the whole duration of the run, not only at its start:

- `@window`;
- `@altitude`, `@airmass` and `@ha`;
- `@moon` and `@illumination`;
- `@twilight`.

The duration is estimated by running the body once in estimation mode, and it is replaced by the measured duration after the first run.

ACP checks the start and the end of a run. Most of the others check the start, or rely on triggers that stop a run part-way through. APT starts a plan when its trigger fires; "execute till time/altitude" repeats a shorter plan and cancels a longer one at the start of the next exposure after the limit, so a plan is cut rather than kept from starting.

### 4. The whole night is planned in advance with the `VISIBILITY` strategy

`AGENT_SCHEDULER_STRATEGY` has two settings:

- **`COMPACT`** (the default) is greedy: it starts the best task as soon as one can run.
- **`VISIBILITY`** plans the whole night before starting anything:
  - it never has fewer runs than `COMPACT` and adds the runs `COMPACT` leaves out where they still fit;
  - it then moves the runs to the times where their targets stand highest;
  - it maximises the sum of run quality, each weighted by the length of its run (the quality of a run is the mean altitude in percent of the culmination altitude);
  - priority classes are respected, and urgent tasks never wait;
  - every constraint holds for the whole run;
  - `@repeat … every` keeps its spacing;
  - hooks are not blocked;
  - `VISIBILITY_FLIP_PENALTY` counts every meridian flip of a German equatorial mount as lost imaging time, so short runs are moved to one side of the meridian and only runs long enough to be worth it keep a flip.

In the guide's example, the same six runs move from a mean quality of 73 % with `COMPACT` to 97 % with `VISIBILITY`. In its two-night comparison, the default flip penalty of 10 minutes cuts the flips of a full night from four to one for less than one percentage point of quality.

The amateur tools avoid flips only through hard limits, such as the meridian window of N.I.N.A. Target Scheduler or an hour-angle constraint, which INDIGO also has with `@ha`. None of them weighs a flip against imaging quality.

None of the other schedulers plans the whole night with a guarantee that no run is lost:

- N.I.N.A. Target Scheduler and Voyager rank the targets at each decision point.
- Ekos is greedy.
- ACP and SGP are essentially greedy.
- APT does not plan at all: the user decides the order and the times with triggers and tags.

### 5. The plan is visible and explained

`AGENT_SCHEDULER_PLAN` applies exactly the rules of the live scheduler. It shows every planned run of the night with its time and a reason, for example `NOT TONIGHT | too bright (twilight)`. `REASON` always says why nothing runs, for example `Soul starts at 21:15 UTC, planned for better visibility`. `AGENT_SCHEDULER_HISTORY` keeps the outcome of every run.

Voyager and N.I.N.A. Target Scheduler offer previews. In Ekos and SGP the plan is mostly seen as it happens. APT shows the list of triggers the user set up, not a computed plan of the night.

### 6. Hooks cover the whole night

The hooks are `STARTUP`, `SHUTDOWN`, `IDLE`, `NIGHT_START`, `NIGHT_END` and `RESUME`. They are ordinary tasks, so they can use everything a sequence can do, for example:

- cool the camera;
- open the dome;
- park while waiting;
- resume after a wait;
- put the equipment away at dawn.

A disabled hook is skipped.

ACP has startup and shutdown scripts. N.I.N.A. Target Scheduler has instructions before and after each wait. APT comes close with triggers at night events (end of deep-sky darkness, end of astronomical night, end of night) that run a plan, for example to park and warm the camera.

### 7. It is headless and controlled through the bus

The scheduler is an INDIGO agent, and everything is an INDIGO property. Any INDIGO client can control it:

- AIN;
- the web control panel;
- `indigo_prop_tool`;
- MCP;
- another INDIGO server.

The scheduler, the equipment and the client can be on different machines, and the scheduling keeps running when no client is connected. AIN Imager provides the user interface; other clients can provide their own over the same properties. N.I.N.A., SGP, Voyager, ACP and APT are Windows desktop applications in which the scheduler and its interface are one program. Ekos is a desktop application that needs a running KStars.

### 8. Smaller advantages

- **Moon avoidance:** the Lorentzian limit (`@moon > deg width days`), plus a maximum illumination (`@illumination`).
- **Restarts:** runs, retries and history survive a server restart.
- **Editing a running task:** the change takes effect only after the current run ends.
- **FITS headers per task:** `OBSERVER`, `PI_NAME`, `PROPID`, `SCHTASK`, `SCHRUN` and `SCHNIGHT`.

### 9. Safety handling with hysteresis (level with the best)

The scheduler follows the `SAFE` light of `AGENT_AUX_SAFETY`. When the conditions become unsafe, it enters the unsafe hold at once:
- the running task is aborted and queued again; the interruption uses no retry, and `@preempted` does not apply;
- every running hook is aborted except `SHUTDOWN`, `NIGHT_END` and `UNSAFE`, and so is a sequence started by hand;
- the `UNSAFE` hook runs once, for example to park and close;
- during the hold no task, no urgent task and no hook starts, except `NIGHT_END` at dawn.

The hold ends only after the conditions have been safe for `SAFE_DELAY` minutes without interruption (default 10). Every unsafe report, even one shorter than a check, restarts the delay, so a flickering sensor does not open the roof. The `SAFE` hook then runs only when the night still has work; otherwise the scheduler waits in standby and does not reopen for nothing.

Further details:
- `AGENT_SCHEDULER_SAFETY_MISSING` chooses whether a missing signal (no agent, no property, an Idle light) counts as safe or as unsafe. `UNSAFE` gives fail-safe operation when the weather station disappears.
- `START` while unsafe waits until the conditions have been safe for `SAFE_DELAY`.
- The time of the last unsafe report survives a server restart; a server stopped during unsafe conditions counts as unsafe until it starts again.
- `REASON` shows `unsafe conditions` or `unsafe hold ends in <n> min`, and the history records every run aborted with the result `UNSAFE`.

This matches what ACP and Voyager do. Ekos has a warning level in addition, which stops new jobs but lets the running one finish; INDIGO has no such middle level.

## Where INDIGO is weaker or incomplete

### 1. No progress towards a goal in frames

N.I.N.A. Target Scheduler and Voyager count accepted frames per filter towards a goal. They keep scheduling a target over several nights until the goal is reached. N.I.N.A. Target Scheduler can also grade images (stars, HFR) and count only the good frames.

INDIGO counts successful runs (`@repeat`), not frames. A run whose frames were spoiled by clouds still counts as done.

### 2. No per-filter or per-condition plans

N.I.N.A. Target Scheduler can set twilight and Moon limits per filter, for example narrowband in moonlight or twilight and broadband only in dark time. It chooses the exposure plan that suits the current sky.

In INDIGO the constraints of a task apply to its whole body. To get the same effect, a target has to be split into one task per filter group.

### 3. No mosaics

Ekos and Voyager, and N.I.N.A. through its framing tool, generate the panels of a mosaic. In INDIGO each panel is a task written by hand.

### 4. Limits of the planning

- With more than 64 runs in a night, `VISIBILITY` falls back to the `COMPACT` plan.
- Within a priority class, the score uses only the level, the setting bonus and the altitude bonus. There are no per-target weighting rules such as those of N.I.N.A. Target Scheduler.
- The plan cannot foresee clouds. It is computed again after every run, and the guide recommends `COMPACT` when the weather is uncertain.

### 5. No horizon profile

Ekos, N.I.N.A. Target Scheduler and ACP support a custom horizon for trees and buildings. INDIGO has only a minimum altitude per task (`@altitude > deg`).

### 6. Open items

- **No time limit:** `@duration` is a planning value, not a time limit. The `@timeout` key is planned but not implemented.
- **Fixed Scripting Agent:** the Scripting Agent that runs the tasks is the local one. Selecting it through the related-agents list is planned.
- **Settings carry over:** agent settings left by one task carry over into the next, for example meridian flip, HA limits and filter offsets.

## Professional schedulers

Professional observatories schedule at a different level from the tools above. Their schedulers serve many observing programmes on one or more telescopes, divide the time awarded by a time allocation committee, and react to the measured sky quality. This section compares the INDIGO scheduler with the following systems, described from their published papers and documentation:

- **Las Cumbres Observatory (LCO) adaptive scheduler:** schedules a global network of robotic telescopes. It solves an integer linear programme over all pending requests and all telescopes, re-solves it every few minutes and whenever the weather or the request pool changes, and has a rapid-response mode that interrupts the running observation for a time-critical target within minutes.
- **ESO VLT service mode:** observations are prepared as observation blocks (OBs) with constraint sets for seeing, transparency, Moon, airmass and twilight. They are executed from a queue ranked by programme grade and current conditions, with the night astronomer in the loop. The Rapid Response Mode lets an external trigger, such as a gamma-ray-burst alert, start an observation automatically within minutes.
- **Gemini queue scheduling:** programmes are prepared in an observing tool and scheduled by rank band and the current conditions (image quality, cloud cover, sky background, water vapour). Rapid targets of opportunity interrupt the queue. Gemini is moving to an automated scheduler as part of its new program platform.
- **ALMA dynamic scheduling:** scheduling blocks are ranked in real time by a score combining scientific priority, the current precipitable water vapour and atmospheric phase stability, the array configuration and the source elevation.
- **Rubin Observatory (LSST) feature-based scheduler:** chooses the next pointing of the survey from a weighted sum of basis functions: sky brightness, seeing, slew time, filter changes, coverage of the footprint and revisit cadence. It runs in real time and is tuned with simulations of the whole ten-year survey.
- **Zwicky Transient Facility (ZTF) scheduler:** optimises a night of survey fields with integer linear programming over time slots, maximising the expected volume searched and keeping the revisit cadence of each programme.
- **RTS2 (Remote Telescope System 2):** an open-source observatory control system for robotic telescopes. Its selector picks targets by priority and constraints, and it receives GRB alerts from GCN and interrupts the running observation for them.

| Capability | Professional systems | INDIGO |
|---|---|---|
| Constraints on target position: altitude, airmass, HA, Moon, twilight, windows | all | yes, without a horizon profile or per-filter limits |
| Constraints on measured conditions: seeing, transparency, sky brightness, water vapour | ESO, Gemini, ALMA, Rubin, LCO | no |
| Urgent targets interrupting the running observation | LCO, ESO, Gemini, RTS2; Rubin and ZTF for gravitational-wave follow-up | yes, `@priority urgent` or `AGENT_SCHEDULER_URGENT_TASK` |
| Optimising a whole night or longer | LCO and ZTF (integer programming), Rubin (simulated over the whole survey) | yes, `VISIBILITY` plans the whole night |
| Cost of slews and filter changes in the plan | Rubin, ZTF | no |
| Several telescopes scheduled together | LCO | no, one scheduler per observatory |
| Programmes, time allocation and fairness between users | ESO, Gemini, ALMA, LCO | no; priorities only |
| Cadence and revisit requirements | Rubin, ZTF, LCO | partly, with `@repeat … every` |
| Data quality fed back into scheduling: an observation is repeated if the data are poor | ESO, Gemini, ALMA, Rubin | no |
| Weather and safety | all | yes, an unsafe hold with hysteresis and `UNSAFE`/`SAFE` hooks |
| A task as a script that runs unchanged by hand | no; tasks are declarative blocks executed by fixed templates | yes |

**What INDIGO shares with professional systems:**
- interrupting the running observation for urgent targets;
- checking every constraint over the whole duration of an observation;
- planning the whole night ahead;
- a queue that a programme can feed through a single interface.

**What it lacks:**
- constraints on measured sky conditions such as seeing and transparency;
- the cost of slews and filter changes in the plan;
- several telescopes in one plan;
- time allocation and fairness between users;
- feedback from data quality.

The cost of slews and filter changes, several telescopes in one plan, and time allocation and fairness between users matter to observatories shared by many users and to surveys. They are out of scope for INDIGO and are not scored below. Sky-condition constraints and data-quality feedback are in scope: they would help a single observatory too.

### Ranking of the professional systems

The professional systems are ranked separately from the tools of the next section, with INDIGO included for reference. Each area is scored from 0 to 3 as below: 0 missing, 1 basic, 2 good, 3 most advanced in this group. The scores are a judgement based on published descriptions, not measurements.

Receivers of alerts (GCN, VOEvent, alert brokers) are not part of a scheduler: they are separate programs that feed urgent tasks to it, so they are out of scope here and the urgent-targets area scores only how the scheduler handles an urgent task once it arrives. Professional systems usually bundle such receivers; INDIGO accepts their tasks through `AGENT_SCHEDULER_URGENT_TASK`.

The cost of slews and filter changes, several telescopes in one plan, and programmes, time allocation and fairness are out of scope, as explained above, and are not scored.

| Area | LCO | Rubin | ESO VLT | Gemini | ALMA | ZTF | RTS2 | INDIGO |
|---|---|---|---|---|---|---|---|---|
| Constraints on target position | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 2 |
| Constraints on measured sky conditions | 2 | 3 | 3 | 3 | 3 | 1 | 1 | 0 |
| Urgent targets interrupting the running work | 3 | 2 | 3 | 3 | 1 | 2 | 3 | 3 |
| Optimisation of the plan | 3 | 3 | 1 | 1 | 2 | 3 | 1 | 3 |
| Cadence and revisits | 3 | 3 | 2 | 2 | 1 | 3 | 1 | 1 |
| Data quality fed back into scheduling | 1 | 2 | 3 | 3 | 3 | 1 | 1 | 0 |
| Weather and safety | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 |
| Flexibility of a task | 1 | 1 | 1 | 1 | 1 | 1 | 2 | 3 |
| **Total (max 24)** | **19** | **20** | **19** | **19** | **17** | **17** | **15** | **15** |

INDIGO's target-position score is 2 because it has no horizon profile and no per-filter limits. Its flexibility scores 3 against 1 for the professional systems, whose tasks are declarative blocks run by fixed templates; see [A task is one plain-text document](#2-a-task-is-one-plain-text-document). Rubin and ZTF score 2 for urgent targets: both take gravitational-wave alerts into the scheduler and re-plan around them, but urgent follow-up is a small part of their operation and is driven by a fixed set of strategies.

By total:

1. **Rubin feature-based scheduler (20):** the most advanced survey optimisation, with sky conditions, cadence and gravitational-wave follow-up.
2. **LCO adaptive scheduler (19)**, **ESO VLT service mode (19)** and **Gemini queue (19)**, level:
   - LCO: optimisation over a whole network, rapid response and cadence;
   - ESO and Gemini: condition-driven queues with rapid response and data-quality feedback; the night astronomer takes part in the decisions, and the plan is not optimised as a whole.
3. **ALMA dynamic scheduling (17)** and **ZTF scheduler (17):** ALMA ranks live by the atmosphere and data quality; ZTF optimises a survey night with integer programming and cadence.
4. **RTS2 (15)** and **INDIGO Scheduler Agent (15)**, level:
   - RTS2: a robotic observatory system with priority selection and urgent interruption; the closest to INDIGO in purpose;
   - INDIGO: whole-night planning, urgent interruption, safety hold and script tasks, without sky-condition constraints, data-quality feedback or a horizon profile.

INDIGO's gap:
- constraints on measured sky conditions from an AUX sky quality or seeing monitor would add 2 to 3;
- re-running a task whose frames are poor would add 2;
- a horizon profile would add 1.

With these, INDIGO would score about 20 to 21 of 24, level with the leading professional systems.

The professional systems are not included in the ranking below, which compares tools for single amateur and small professional observatories.

## Ranking

The ranking scores each scheduler from 0 to 3 in eight areas of scheduling capability: 0 means missing, 1 basic, 2 good, 3 most advanced in this group. The user interface is left out, as above. The scores are a judgement based on the comparison above, not measurements; a different weighting of the areas can change close positions.

| Area | INDIGO | ACP | Voyager | N.I.N.A. TS | Ekos | SGP | APT |
|---|---|---|---|---|---|---|---|
| Night planning: choosing and placing the runs | 3 | 2 | 2 | 2 | 1 | 1 | 0 |
| Constraints: altitude, airmass, HA, Moon, twilight, windows, horizon | 2 | 3 | 3 | 3 | 2 | 1 | 1 |
| Urgent targets interrupting the running work | 3 | 3 | 0 | 0 | 2 | 0 | 0 |
| Safety and weather | 3 | 3 | 3 | 2 | 2 | 2 | 1 |
| Progress towards goals: frakvme counts, grading | 1 | 2 | 3 | 3 | 2 | 1 | 0 |
| Flexibility of a task | 3 | 2 | 2 | 2 | 1 | 1 | 1 |
| Night automation: startup, shutdown, idle, night events | 3 | 3 | 3 | 2 | 2 | 2 | 2 |
| Headless operation and remote control | 3 | 2 | 2 | 1 | 1 | 1 | 0 |
| **Capability (max 24)** | **21** | **20** | **18** | **15** | **13** | **9** | **5** |

By capability:

INDIGO's constraints score 2 because it has no horizon profile and no per-filter limits. Its flexibility scores 3 because a task is a full program with access to the whole bus, as explained in [A task is one plain-text document](#2-a-task-is-one-plain-text-document). Ekos scores 2 for urgent targets: its greedy scheduler interrupts a running job when a higher-priority job becomes eligible, although it has no explicit urgent class.

1. **INDIGO Scheduler Agent (21):** the most advanced planning, urgent handling, task flexibility and headless operation, with safety handling level with ACP and Voyager; held back by progress counted in runs rather than frames and by the missing horizon profile.
2. **ACP Scheduler (20):** complete and balanced: urgent targets, safety, constraints and night automation, with no weak area.
3. **Voyager RoboTarget (18):** strong safety and goal tracking, without whole-night planning or urgent interruption.
4. **N.I.N.A. Target Scheduler (15):** the best progress tracking with image grading and per-filter rules, with safety through N.I.N.A. and no urgent interruption.
5. **Ekos Scheduler (13):** a capable greedy scheduler with safety handling, frame counts and interruption by higher-priority jobs.
6. **Sequence Generator Pro (9):** target order and simple limits, with safety and recovery.
7. **APT SessionCraft (5):** user-defined triggers without planning, with a safety-monitor reaction.

INDIGO leads the capability ranking; with a horizon profile it would score 22.

## Combined ranking

This ranking puts the professional systems and the amateur tools on one scale. It merges the areas of both rankings and leaves out the three multi-user areas that are out of scope (cost of slews and filter changes, several telescopes in one plan, time allocation and fairness):
- night planning and optimisation of the plan merge into one area;
- progress towards goals and data quality fed back into scheduling merge into one area;
- night automation and headless operation come from the amateur ranking;
- the others are common to both or come from the professional ranking.

Each area is scored from 0 to 3, where 3 is the most advanced among all fourteen systems. Some scores therefore differ from the two rankings above, where 3 meant the most advanced within that group: the other amateur tools' planning drops to 1 next to the systems that plan the whole night. Night automation and headless operation score the scheduler together with the control system it runs in; ESO, Gemini and ALMA score lower there because operators run the night. The scores are a judgement based on published descriptions, not measurements.

| Area | Rubin | LCO | ZTF | ESO VLT | Gemini | INDIGO | RTS2 | ALMA | ACP | Voyager | N.I.N.A. TS | Ekos | SGP | APT |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Night planning and optimisation | 3 | 3 | 3 | 1 | 1 | 3 | 1 | 2 | 1 | 1 | 1 | 1 | 1 | 0 |
| Constraints on target position | 3 | 3 | 3 | 3 | 3 | 2 | 3 | 3 | 3 | 3 | 3 | 2 | 1 | 1 |
| Constraints on measured sky conditions | 3 | 2 | 1 | 3 | 3 | 0 | 1 | 3 | 0 | 1 | 0 | 0 | 0 | 0 |
| Urgent targets interrupting the running work | 2 | 3 | 2 | 3 | 3 | 3 | 3 | 1 | 3 | 0 | 0 | 2 | 0 | 0 |
| Cadence and revisits | 3 | 3 | 3 | 2 | 2 | 1 | 1 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| Goals and data quality | 2 | 1 | 1 | 3 | 3 | 1 | 1 | 3 | 2 | 2 | 3 | 2 | 1 | 0 |
| Weather and safety | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 2 | 2 | 2 | 1 |
| Flexibility of a task | 1 | 1 | 1 | 1 | 1 | 3 | 2 | 1 | 2 | 2 | 2 | 1 | 1 | 1 |
| Night automation | 3 | 3 | 3 | 1 | 1 | 3 | 3 | 1 | 3 | 3 | 2 | 2 | 2 | 2 |
| Headless operation and remote control | 3 | 3 | 3 | 2 | 2 | 3 | 3 | 2 | 2 | 2 | 1 | 1 | 1 | 0 |
| **Total (max 30)** | **26** | **25** | **23** | **22** | **22** | **22** | **21** | **20** | **20** | **18** | **14** | **13** | **9** | **5** |

Notes on individual scores:
- INDIGO scores 3 for flexibility of a task, the only 3 in the table: a task is a full program with access to the whole bus, and the hooks use the same language. See [A task is one plain-text document](#2-a-task-is-one-plain-text-document).
- INDIGO's target-position score is 2: it has no horizon profile and no per-filter limits.
- Voyager scores 1 for sky conditions: RoboTarget can use a sky-quality (SQM) limit.
- N.I.N.A. Target Scheduler has no sky-condition constraints yet; its documentation lists them as planned.
- Ekos scores 2 for urgent targets because its greedy scheduler interrupts a running job for a higher-priority one, and 0 for cadence because it repeats jobs without a revisit interval.
- ACP and INDIGO score 1 for cadence: both repeat with an interval, without the cadence windows of the survey schedulers.
- Planning scores the quality of the plan for the night, not the scale of the optimisation. Slew and filter-change costs, several telescopes and thousands of observations belong to the out-of-scope areas and add nothing here. INDIGO scores 3 with LCO, Rubin and ZTF: `VISIBILITY` plans the whole night, maximises the altitude quality of every run, keeps every run and, through `VISIBILITY_FLIP_PENALTY`, avoids meridian flips where they cost more than they gain. Flip avoidance is part of the quality of the plan: a flip interrupts the imaging and needs a re-centre, refocus and guider restart, which is not a slew cost. Its limit of 64 runs per night is far above an amateur night.

By total:

1. **Rubin (26)**
2. **LCO (25)**
3. **ZTF (23)**
4. **ESO VLT (22)**, **Gemini (22)** and **INDIGO Scheduler Agent (22)**, the leading amateur tool
5. **RTS2 (21)**
6. **ALMA (20)** and **ACP Scheduler (20)**
7. **Voyager RoboTarget (18)**
8. **N.I.N.A. Target Scheduler (14)**
9. **Ekos Scheduler (13)**
10. **Sequence Generator Pro (9)**
11. **APT SessionCraft (5)**

INDIGO is level with ESO and Gemini and ahead of every other amateur tool. Its strengths are night planning, task flexibility, urgent interruption, safety handling, night automation and headless operation. Its only zero is sky-condition constraints. A horizon profile (+1) would bring it to 23, level with ZTF; sky-condition constraints (+2 to 3) and data-quality reruns (+1 to 2) to about 26 to 28, level with or above LCO and Rubin.

## Suggested priorities

1. **Progress towards a goal in frames per filter**, possibly with image grading.
2. **Per-filter constraints** and a **horizon profile**.
3. **Mosaics** and **`@timeout`**.
4. Optionally a **warning level** between safe and unsafe that stops new tasks but lets the running one finish.
5. From the professional systems: **constraints on measured sky conditions** (seeing, transparency, sky brightness from an AUX sky quality or seeing monitor) and **feedback from data quality**, so that a run with poor frames is repeated.
