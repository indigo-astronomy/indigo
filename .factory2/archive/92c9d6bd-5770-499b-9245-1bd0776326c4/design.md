# Design: readability rewrite of the scheduler user guide

Ticket: 92c9d6bd-5770-499b-9245-1bd0776326c4 (`indigo_docs: rewrite the scheduler guide for readability`)
Target file: `indigo_docs/INDIGO_SCHEDULER_GUIDE.md` (1164 lines, 15,183 words at commit 1b3e9749b)
Stage: design. The guide itself is not changed in this stage.

Line numbers below refer to the guide at commit 1b3e9749b.

Owner decisions already in force, and kept by this design:

- No INDIGO client is named. The guide says you can use the dedicated scheduler windows of the INDIGO clients. All command examples use `indigo_prop_tool`.
- `COMPACT` decides by priority at every start. `VISIBILITY` uses priority only to choose which runs get into the plan, then places them by visibility. Urgent tasks are never moved.
- The meridian flip is set once, in the `STARTUP` hook. `VISIBILITY_FLIP_PENALTY` applies only to the `VISIBILITY` strategy.
- The worked examples and the "Two nights compared" charts (`indigo_docs/INDIGO_SCHEDULER_GUIDE/busy_night.png`, `sparse_night.png`) stay.

---

## 1. Diagnosis of the current guide

The guide is correct and complete as far as it was checked, but it is hard to read. The causes fall into six groups.

### 1.1 Long sentences that carry several rules

| Line | Quote (shortened only where marked) | Problem |
|---|---|---|
| 215 | "an **altitude** bonus of up to `100 × ALTITUDE_WEIGHT`, largest for a target near its own culmination (highest point), where it reaches the full bonus. The altitude is taken relative to the highest altitude that target reaches at your latitude, so a southern target at transit scores the same as a northern one at transit, and tasks tend to run near their own culmination rather than all at dusk. A task without `@target` gets none. The bonus only compares the tasks that can start at the same moment; by default the scheduler never waits for a better time, see [How the next task is chosen] and the `VISIBILITY` strategy for the exception. `@ha` remains the hard window. A 0 only means no bonus: whether a task can run is decided by its constraints (eligibility), so an eligible task with a 0 altitude bonus, such as one without `@target`, still runs when it scores highest or is the only eligible task." | One list item of about 140 words with seven separate ideas: definition, normalisation, its effect, tasks without target, strategy exception with two cross-references, `@ha`, and "a bonus is not a condition". |
| 254 | "`SETTING_WEIGHT` and `ALTITUDE_WEIGHT` do not move runs: they shape the `COMPACT` plan that `VISIBILITY` starts from, and they decide whenever the scheduler falls back to the `COMPACT` choice. That happens when no plan beats `COMPACT`, when a night has more than 64 runs or the plan reaches more than 24 hours ahead, or when a planned run cannot start at its time even after the night is planned again." | A rule, its scope and three fallback conditions in one paragraph that also covers constraints and quality. |
| 256 | "At 42° N, 23° E on 19 November 2026, six tasks with `@twilight astronomical` and `@altitude > 20` (`> 10` for Pleiades and Soul), Orion with `@priority high`, astronomical dusk at 16:45 UTC, with the meridian flip disabled (or `VISIBILITY_FLIP_PENALTY` 0); with the flip enabled at `OFFSET` 0 and the default penalty, Soul, Pleiades and Jellyfish start once they have crossed the meridian and the night has no flip, see [Meridian flips]" | The set-up of an example, a condition, and the result of a second, different example in one sentence. |
| 393 | "The duration is the total exposure time plus `FRAME_OVERHEAD` seconds per frame, `OPERATION_OVERHEAD` seconds per operation, and the fixed waits. A change of `FRAME_OVERHEAD` or `OPERATION_OVERHEAD` does not recompute ... Operations are the steps that move or calibrate equipment: ... Steps whose length cannot be known, `wait_until()`, ... make the estimate impossible; the scheduler then uses 60 minutes until the task has run once, and tells you to set `@duration`. It also uses 60 minutes when the estimate gets no answer within ten seconds." | Formula, update rule, a definition and two fallbacks in one paragraph. |
| 486 | "After every task run, whatever its outcome (succeeded, failed, preempted or aborted), the scheduler puts the folder and the object name back ... A task that has to change them for a part of its run still can: ... In particular, a `set_directory()` of a task no longer stays for the next task. A hook run with `@folder <name>` gets its folder and object name back in the same way. For any other hook run the scheduler sets nothing and puts nothing back, so ..." | Rule, exception, a change-history remark ("no longer"), and hook behaviour in one paragraph. |
| 920 | "While a hook runs, whatever its kind (...six kinds...), it is the first item, for example `...`. Its end is its start plus its duration, but not earlier than a minute from now, and the planned runs follow from that end. A running night hook replaces its planned item, and the item disappears when the hook ends. Only the night hooks are forecast; ..." | Five rules about hook items in one list item. |

### 1.2 Ambiguous wording

| Line | Quote | Ambiguity |
|---|---|---|
| 229 | "The scheduler decides only between tasks: a running task is never interrupted for a more important normal task, only for an urgent one." | "decides only between tasks" has no clear meaning; the intended rule is the second half. |
| 250 | "It leaves gaps where waiting gains more than it costs." | Gains what, costs what? The measure (quality vs. idle time) is defined only two paragraphs later. |
| 397 | "A task is not started while its estimate is pending." | Reads as "that task waits". In the code (`select_task()` in `indigo_agent_scheduler.c`, lines 1645-1683) no task except an urgent one starts while **any** estimate is pending. The reason table (line 995) says it correctly. |
| 718 | "The night only times the night hooks." | "Night" here means the `AGENT_SCHEDULER_NIGHT` interval, not the sky darkness used by `@twilight`. The word has two meanings in the guide. |
| 664 | "It does not run again while the equipment is in standby, see [Several nights]" | "Standby" is used before it is defined (line 728). |
| 491 | "Hooks are not labelled" | "Label" also means the property label and the six-column item label (line 860). Here it means object name / FITS keywords / folder. |
| 285 | "Waiting is idle time." | States the obvious, while the actual rule (IDLE hook vs. a planned run within `IDLE_HOOK_DELAY`) follows. |
| 37 | "Selecting the Scripting Agent is planned." | A future feature in a user guide, mid-paragraph. |

### 1.3 Rules mixed with their exceptions

- Line 220: the `hook` class, a list of eleven forbidden keys, the keys that stay valid, the one key a hook run uses, a cross-reference and the urgent-task restriction, all in one paragraph.
- Line 445: "Set up everything you rely on ... The exceptions are ... Set the filter ... The meridian flip is the exception: ..." Two different "exceptions" in one bullet.
- Line 491: a hook gets no folder, except `@folder <name>`, which must not deselect the camera, except without `@folder` or with `@folder none`.
- Line 650: the automatic stop, the case where it does not happen, and how to cancel it.

### 1.4 Cross-references in mid-sentence

There are 51 `see [...]` links, most of them in the middle of a sentence or inside a table cell. Examples: line 215 (two inside one sentence), line 217 ("set ... to 0 in [Settings](#settings)"), line 245 ("see *What `VISIBILITY` respects* below"), line 312, line 445 (two), line 919 ("with [`VISIBILITY`](#choosing-a-strategy) the plan waits"). The reader must decide whether to jump away in the middle of a rule.

### 1.5 Inconsistent terms

| Concept | Terms used | Lines |
|---|---|---|
| A task that may start now | "eligible", "can run", "is ready", "its constraints hold" | 215, 235, 799, 996 |
| The `VISIBILITY` strategy | `VISIBILITY`, "Best visibility", "the strategy" | 245, 804 |
| The `AGENT_SCHEDULER_NIGHT` interval | "the night", "the scheduler's night", "the night of the night hooks", "night limit" | 711, 718, 922, 1093 |
| The state after `IDLE`/`NIGHT_END` | "standby", "put the equipment away" | 665, 728 |
| Writing object name, FITS keywords and folder | "label", "labelled", "tell the frames which task made them" | 451, 491, 703 |
| A single execution of a task | "run", "task", "task run" | 233, 486, 619 |
| A task used by a hook | "hook", "hook task", "hook-only task", "hook run" | 220, 656, 699, 704 |
| The ranking value | "score", "class and level", "priority" | 212, 237, 1029 |

### 1.6 Repetition

The same rule is stated in several places, each time with slightly different wording, so the reader cannot tell whether the versions differ.

| Rule | Lines |
|---|---|
| Constraints must hold for the whole duration | 13, 199, 235, 254, 281, 619, 624 |
| The duration is not a time limit; use checkpoints | 201, 399-428, 446, 725 |
| Urgent tasks run first, in submission order, never interrupt each other | 16, 222-227, 236, 245, 280, 591-630 |
| What `VISIBILITY` does | 238, 245, 250, 254, 277-286, 814, 1120 |
| Weights and their role under each strategy | 217, 254, 802, 803, 1029 |
| Meridian flip set once in `STARTUP` | 299, 445, 696 |
| Hooks get no object name, FITS keywords or folder | 486, 491, 703 |
| `@ha` stays the hard limit | 215, 281 |
| A bonus of 0 does not stop a task | 215, 237 |

### 1.7 Order

The guide puts concepts after details. "Priority under each strategy" (line 245) comes before the two strategies are introduced (line 247). The six-column label format (line 860) comes before the lists it describes. Standby is used (line 664) before it is defined (line 728). The quick start (line 88) comes after a long agent table and a section on tools.

---

## 2. New outline

Three parts and a short appendix. Headings are unique across the whole file, so GitHub anchors never get a `-1` suffix.

### Part 1: Quick start (about 900 words)

1. **Introduction** - what the scheduler does, in six short bullets; where the design and the property list are.
2. **What you need** - the Scripting and Mount Agents on the same server, the site set in the Mount Agent, times are UTC. The full agent table moves to the reference.
3. **Loading the agents** - the two commands.
4. **How to talk to the scheduler** - the dedicated scheduler windows of the INDIGO clients; `indigo_prop_tool` with `set_script`, `set`, `get`, `-r host`, `-s`.
5. **A first task in five minutes** - unchanged walk-through: save, upload, start, watch `CURRENT`/`REASON` and the night plan, delete.

### Part 2: Concepts (about 5,500 words, examples included)

Each section explains one idea with an example first and gives the main rule. Edge cases go to the reference, with one closing "Details:" line.

6. **Tasks** - header and body, the M31 example, `scheduler_task`. Uploading, the file convention, what happens on upload.
7. **Constraints** - "a task starts only if it can finish"; altitude, airmass, hour angle, moon, illumination, twilight, time windows explained with one example each; the duration is a forecast, not a limit, with the checkpoint example.
8. **Task duration** - the four sources (set, measured, estimated, default) and when to set `@duration`.
9. **Priorities** - class, level, score, urgent tasks, ties. (Sample section in chapter 4.)
10. **How the next task is chosen** - the decision steps. (Sample section in chapter 4.)
11. **Choosing a strategy** - `COMPACT` and `VISIBILITY`, how each uses priority, quality of a run, the six-task example table, when to use which.
12. **Two nights compared** - the busy and sparse nights with charts and tables, unchanged in content, shorter sentences.
13. **The night plan** - what it is (a forecast), how to read it, the example plan.
14. **Hooks and the night** - what hooks are, the park/unpark example, the scheduler night (`AGENT_SCHEDULER_NIGHT`), standby and resume, the observatory example with its timeline table.
15. **Meridian flips** - set once in `STARTUP`; which runs flip; the plan marks them; `VISIBILITY` avoids short flips with `VISIBILITY_FLIP_PENALTY`.
16. **Where the data goes** - object name, FITS keywords, folders; `@folder`, `@observer`, `@pi`, `@project`; everything is put back after a run.
17. **Urgent tasks** - the two ways, the supernova example, what happens to the interrupted task.
18. **Repeats, retries and the history** - `@repeat`, `@retry`, what counts as success, reading the history.
19. **Starting, stopping and restarting** - `START`/`STOP`/`ABORT`, the automatic stop, several nights, server restart.
20. **More examples** - the three example tasks (NGC 7000, SS Cyg, sky flats).

### Part 3: Reference (about 6,000 words, mostly tables)

21. **Agents the scheduler depends on** (heading kept) - the full agent table and site sources.
22. **Header syntax** - header end, line length, case, uniqueness, comparison signs, `>=`/`<=`.
23. **Header keys** (heading kept) - the key table, unchanged columns.
24. **Constraint rules** - five-minute grid, horizon rule, keys that need `@target`, window forms and expiry, Lorentzian moon formula, Moon below horizon, `beyond the forecast`.
25. **Duration rules** - estimate formula, operations, steps that block an estimate, ten-second timeout, measurement, checkpoints.
26. **Score rules** - the score formula (class base, level, setting term with its 12-hour horizon, altitude term), the 900/1000 bound, ties.
27. **Strategy rules** - everything currently under "What `VISIBILITY` respects", the fallbacks (no better plan, more than 64 runs, more than 24 hours, a planned run that cannot start), replanning triggers, IDLE hook during a wait.
28. **Meridian flip rules** - `MERIDIAN_FLIP` items, flip condition, negative `OFFSET`, penalty cost, duration.
29. **Hook rules** - the `AGENT_SCHEDULER_HOOKS` item table, failures, disabled hooks, one task for several hooks, `@priority hook` and its forbidden keys, removing a hook.
30. **Night rules** - night limit, offsets, what happens each night, no night (polar cases).
31. **Urgent task rules** - what an urgent task does not interrupt, preemption steps, `@preempted`, submission while stopped, limits.
32. **Data rules** - `AGENT_SCHEDULER_DATA` table, `BASE`, character replacement, restore rules, hooks, "Imager Agent" only.
33. **Rules for the body** (heading kept).
34. **Managing tasks** - task commands table, changing the order, editing, deleting, upload limits.
35. **Settings** (heading kept) - the settings table.
36. **Scheduler state** - `AGENT_SCHEDULER_STATE` and `AGENT_SCHEDULER_CURRENT` tables.
37. **Label format** (heading kept, anchor used by `SCHEDULER.md`).
38. **The task list** (heading kept) - columns and states.
39. **Night plan items** - columns, states, reasons, hook items, `NOT TONIGHT`.
40. **The history** (heading kept) - columns and results, clearing.
41. **Messages** (heading kept).
42. **Reasons** (heading kept).
43. **What happens when the server restarts** (heading kept).
44. **Troubleshooting** (heading kept).
45. **Property reference** (heading kept).

### Appendix

46. **Trying the scheduler by hand** (heading kept) - the sandbox tool.
47. **Regenerating the charts** - moved from the end of "Two nights compared" (line 353); it is for developers.

### 2.1 Mapping of every current heading

| Current heading (line) | New place | Action |
|---|---|---|
| Introduction (9) | 1 Introduction | Shortened; the two link sentences kept. |
| Before you start (24) | 2 What you need | Merged and shortened. |
| Agents the scheduler depends on (26) | 2 (summary) and 21 (full table) | Full table moved to reference, heading kept there. "Selecting the Scripting Agent is planned" moved to an open note in 21. |
| Agents the tasks depend on (49) | 2 What you need | Merged (two sentences). |
| Loading the agents (53) | 3 Loading the agents | Kept. "Times are UTC" moved to 2. |
| How to talk to the scheduler (73) | 4 | Kept, split into shorter bullets. |
| A first task in five minutes (88) | 5 | Kept. |
| Writing a task (133) | 6 Tasks | Renamed part heading. |
| The shape of a task (135) | 6 Tasks (example), 22 Header syntax (rules) | Split: example and one-line rules in 6, header-end details in 22. |
| The `scheduler_task` object (161) | 6 Tasks | Kept as a table. |
| Header keys (173) | 23 Header keys | Moved to reference, heading kept. The "starts only if it can finish" paragraphs (199-201) move to 7. |
| Priorities (203) | 9 Priorities (concept) and 26 Score rules | Split; see sample. Hook-class rules (220) move to 29. |
| How the next task is chosen (231) | 10 | Kept, rewritten; see sample. |
| Choosing a strategy (243) | 11 (concept, example, when to use) and 27 Strategy rules | Split; "What `VISIBILITY` respects" (277-286) moves to 27. Heading kept (anchor used by `SCHEDULER.md`). |
| Meridian flips (297) | 15 (concept) and 28 (rules) | Split. The 1000-night statistic (306) stays in 15 as one sentence. |
| Two nights compared (308) | 12 | Kept, heading and charts unchanged. "Regenerating the charts" (353) moves to 47. |
| Time windows (355) | 7 (example) and 24 (rules) | Merged into Constraints. Old anchor `#time-windows` no longer needed (only internal links). |
| Moon avoidance (370) | 7 (example) and 24 (formula) | Merged into Constraints. |
| Task duration (384) | 8 (concept), 7 (not a limit), 25 (rules) | Split. Duplicate "not a time limit" text (201, 399) merged into one place in 7. |
| Repeats and retries (430) | 18 | Merged with history. |
| Rules for the body (442) | 33 | Moved to reference, heading kept. The meridian flip bullet becomes one line pointing to 15. |
| Where the data goes (449) | 16 (concept) and 32 (rules) | Split; the three statements on hooks (486, 491, 703) merged into one rule in 32. |
| More examples (495) | 20 | Kept. |
| Submitting tasks (553) | 6 Tasks | Part heading removed. |
| Uploading a task (555) | 6 Tasks; limits to 34 | Merged. Duplicate M31 example removed (it repeats line 137 without `@moon`, `@repeat`, `@retry`); the walk-through uses the same file as the "shape" example. |
| Urgent tasks (591) | 17 (concept) and 31 (rules) | Split. |
| Running the scheduler (632) | 19 | Part heading removed. |
| Start, stop and abort (634) | 19 | Kept. |
| Hooks (654) | 14 (concept, example) and 29 (rules) | Split. |
| Several nights (707) | 14 (concept, example, timeline) and 30 (rules) | Split. |
| Settings (792) | 35 | Kept, heading kept; the `VISIBILITY` command example (814-818) removed as duplicate of 11. |
| Watching the scheduler (822) | 36-42 | Part heading removed; contents in reference. |
| At a glance: state and current task (824) | 36 Scheduler state | Renamed. |
| Label format (860) | 37 | Heading kept (anchor used by `SCHEDULER.md`). |
| The task list (877) | 38 | Kept. |
| The night plan (903) | 13 (concept) and 39 Night plan items | Split; the concept keeps the heading "The night plan". |
| The history (926) | 18 (reading it) and 40 | Split; reference keeps the heading. |
| Messages (950) | 41 | Kept. |
| Reasons (977) | 42 | Kept. |
| Managing tasks (1001) | 34 | Kept as heading. |
| Commands (1003) | 34 | Merged ("Task commands"). |
| Changing the order (1027) | 34 | Merged; duplicate of 9 reduced to one sentence. |
| Editing a task (1031) | 34 | Merged. |
| Deleting a task (1059) | 34 | Merged. |
| What happens when the server restarts (1069) | 43 (rules), short summary in 19 | Kept. |
| Trying the scheduler by hand (1079) | 46 | Moved to appendix. |
| Troubleshooting (1115) | 44 | Kept; each row links to the concept section at its end. |
| Property reference (1138) | 45 | Kept. |

Nothing is removed except text that repeats another place (marked "duplicate" above). Every repeated rule in table 1.6 is kept once in the reference and summarised once in the concept part.

### 2.2 Anchors used by other files

Searched with `git ls-files | grep -v '^.factory2/archive' | xargs grep -n INDIGO_SCHEDULER_GUIDE`.

| Link | Used in | How it is preserved |
|---|---|---|
| `INDIGO_SCHEDULER_GUIDE.md#label-format` | `indigo_drivers/agent_scheduler/doc/SCHEDULER.md:352` | Heading "Label format" kept verbatim (section 37). |
| `INDIGO_SCHEDULER_GUIDE.md#choosing-a-strategy` | `indigo_drivers/agent_scheduler/doc/SCHEDULER.md:682` (links to the worked six-task example) | Heading "Choosing a strategy" kept verbatim (section 11), and the six-task example stays in that section. |
| `INDIGO_SCHEDULER_GUIDE.md` (no anchor) | `indigo_docs/HOME.md:58`, `SCHEDULER.md:9` | File name unchanged. |
| "Two nights compared" section (by name, in a comment) | `indigo_test/benchmark/bench_scheduler_plan.c:189` | Heading kept verbatim (section 12). |
| `indigo_docs/INDIGO_SCHEDULER_GUIDE/` (chart directory) | `tools/scheduler_plan_charts.py:33,234` | Directory and PNG names unchanged. |
| The guide file | `indigo.xcodeproj/project.pbxproj:5116,6492` | File name unchanged; no new files, so no Xcode change. |

`indigo_docs/PROPERTIES.md` and `indigo_drivers/agent_scheduler/README.md` do not link into the guide. Internal anchors (18 distinct ones, for example `#priorities`, `#hooks`, `#time-windows`) are rewritten to the new headings; the check in chapter 5 verifies that every internal link resolves.

Headings kept verbatim also include, for continuity: Header keys, Priorities, How the next task is chosen, Meridian flips, The night plan, Where the data goes, Urgent tasks, Rules for the body, Settings, The task list, The history, Messages, Reasons, What happens when the server restarts, Trying the scheduler by hand, Troubleshooting, Property reference.

---

## 3. Style guide for the rewrite

### 3.1 Sentences

- Target 15 words per sentence, on average. Hard limit 25 words, except sentences that only list names (keys, items).
- One idea per sentence. A rule and its exception are two sentences, or a rule and a bullet list of exceptions.
- No semicolons joining two rules. No parenthetical rules: if the text in brackets is a rule, it becomes its own sentence.
- Active voice with a clear subject: "The scheduler starts the task", not "The task is started".
- Say what happens, not what does not happen, where both are possible.
- No change history ("no longer", "now") and no planned features in running text. Planned features go to one "Not yet available" line in the reference section they concern.

### 3.2 Order inside a section

1. One or two sentences: what this is for.
2. An example (task header, command or table).
3. The rule in plain words.
4. Edge cases, as a bullet list under **Edge cases** or moved to the reference.
5. One closing line: "Details: [Score rules](#score-rules)." or "See also: ...".

### 3.3 Cross-references

- Never in the middle of a sentence.
- At the end of a paragraph or section, as "Details: [Section](#anchor)." or "See also: [A](#a), [B](#b)."
- In tables, a link is allowed only in a last "See" column or at the end of a cell.
- Each concept section links to its reference section once; each reference section links back to its concept section once.

### 3.4 Warnings and edge cases

- A behaviour that can damage equipment or lose data is a blockquote starting with **Warning:**. Candidates: a hook with `@folder` that deselects the camera; `DELETE_ALL` without confirmation from the command line; an `IDLE` hook that parks when `IDLE_HOOK_DELAY` is short under `VISIBILITY`.
- A behaviour that surprises users but is harmless is a blockquote starting with **Note:**. Candidates: the duration is not a time limit; latitude 0°/longitude 0° is accepted as a site; times are UTC.
- Other edge cases are plain bullets under **Edge cases**.

### 3.5 Terminology (one term per concept)

| Term | Meaning | Do not use |
|---|---|---|
| task | A stored text with a header and a body. | job, sequence (for the task) |
| header, body | The `// @key value` lines; the `Sequencer.js` code after them. | script (for the whole task) |
| run | One execution of a task's body. | task run, execution |
| can run (now) | All constraints hold from now until the run would end. | eligible, ready, available |
| constraint | A header key that limits when a task can run (`@altitude`, `@airmass`, `@ha`, `@moon`, `@illumination`, `@twilight`, `@window`, the horizon). | condition, limit (alone) |
| priority, class, level | `@priority`; `low`/`normal`/`high`/`urgent`/`hook`; the number 0-100. | importance |
| score | Level plus setting bonus plus altitude bonus, compared within a class. | rank, value |
| setting bonus, altitude bonus | The two score terms. | setting term, sky bonus |
| culmination | The highest altitude of a target at the site. | transit (except in "meridian transit"), highest point |
| strategy, `COMPACT`, `VISIBILITY` | `AGENT_SCHEDULER_STRATEGY` and its two items. The client label "Best visibility" is mentioned once, in section 11. | best visibility, greedy, visibility strategy |
| night plan | `AGENT_SCHEDULER_PLAN`. | plan list, forecast list |
| quality | Mean altitude of a run in percent of culmination. | score (for quality) |
| hook | A role in `AGENT_SCHEDULER_HOOKS` (`STARTUP`, ...). | |
| hook task | A task named by a hook, or a task with `@priority hook`. | hook-only task |
| hook run | One execution of a hook task. | |
| scheduler night | The interval set by `AGENT_SCHEDULER_NIGHT` and the offsets, which times the night hooks. | the night (alone, when the interval is meant) |
| darkness | What `@twilight` requires. | night |
| standby | The state after the `IDLE` or `NIGHT_END` hook, until `RESUME` or `NIGHT_START`. | put away, parked state |
| urgent task, one-off urgent task, stored urgent task | Any urgent task; one submitted through `AGENT_SCHEDULER_URGENT_TASK`; one with `@priority urgent`. | |
| labels (of frames) | Not used. Say "object name, FITS keywords and folder". | labelled, labelling |
| item label | The six-column text of a light item. | label (alone) |
| property states | Idle, Ok, Busy, Alert, as in INDIGO. | OK, light colour words |

### 3.6 Formatting

- Property names, item names, header keys, task states, history results and settings in code font: `AGENT_SCHEDULER_PLAN`, `IDLE_HOOK_DELAY`, `@window`, `WAITING`.
- Property states (Idle, Ok, Busy, Alert) in plain text, capitalised.
- Properties are written as `PROPERTY.ITEM` only in commands; in text, "the `REASON` item of `AGENT_SCHEDULER_CURRENT`" the first time in a section, then `REASON`.
- Commands in fenced blocks without a language; tasks in ` ```js ` blocks.
- Units always with the number: "30 minutes", "10 s" in tables.
- Messages and reasons quoted verbatim in code font.
- Numbers from the code (limits, defaults) only in the reference tables, and once in the concept text when they help the example.

---

## 4. Sample section: "Priorities" and "How the next task is chosen"

These two sections, together with the priority paragraph of "Choosing a strategy" (line 245), show the most change.

### 4.1 Before (lines 203-241 and 245, verbatim)

> ### Priorities
>
> `@priority` has a class and a level:
>
> ```
> // @priority high 80
> ```
>
> - The class decides first. Any `high` task is preferred to any `normal` task, and any `normal` task to any `low` task.
> - Within a class, the scheduler compares a score made of the level and two bonuses:
>   - the level, 0–100;
>   - a **setting** bonus of up to `100 × SETTING_WEIGHT`, largest for a task whose constraints stop holding soonest, for example a target that is setting or a window that is closing;
>   - an **altitude** bonus of up to `100 × ALTITUDE_WEIGHT`, largest for a target near its own culmination (highest point), where it reaches the full bonus. The altitude is taken relative to the highest altitude that target reaches at your latitude, so a southern target at transit scores the same as a northern one at transit, and tasks tend to run near their own culmination rather than all at dusk. A task without `@target` gets none. The bonus only compares the tasks that can start at the same moment; by default the scheduler never waits for a better time, see [How the next task is chosen](#how-the-next-task-is-chosen) and the `VISIBILITY` strategy for the exception. `@ha` remains the hard window. A 0 only means no bonus: whether a task can run is decided by its constraints (eligibility), so an eligible task with a 0 altitude bonus, such as one without `@target`, still runs when it scores highest or is the only eligible task.
>
>   With the default weights of 1, the two bonuses together can add up to 200 points, more than the whole level range. A `normal 40` target that is high and about to set can therefore run before a `normal 60` target. To make the level decide strictly, set `SETTING_WEIGHT` and `ALTITUDE_WEIGHT` to 0 in [Settings](#settings); to make the sky position count more, raise them. The bonuses never cross a class: with the maximum weights of 4, the level and the bonuses together add at most 900 points, less than the 1000-point gap between classes. These weights choose the next task under `COMPACT`; under `VISIBILITY` they matter only as described in [Choosing a strategy](#choosing-a-strategy).
> - When the scores are equal, the task higher in the task list wins. You can move tasks in the list, see [Changing the order](#changing-the-order).
>
> `hook` is a class of its own, without a level: `@priority hook` marks a hook-only task that is never scheduled as an ordinary task, see [Hooks](#hooks). `@priority hook 50` is a script error, and so are `@repeat`, `@retry`, `@enabled`, `@window`, `@altitude`, `@airmass`, `@ha`, `@moon`, `@illumination`, `@twilight` and `@preempted` in the same header, because a hook never uses them. `@duration` (it places the hook in the night plan), `@target` and the data keys stay valid; a hook run uses only `@folder <name>` of them, see [Where the data goes](#where-the-data-goes). A task submitted through `AGENT_SCHEDULER_URGENT_TASK` cannot be a hook.
>
> `urgent` is different from the other classes:
>
> - An urgent task runs before every other task, as soon as its own constraints hold.
> - If another, non-urgent task is running, the scheduler interrupts it, see [Urgent tasks](#urgent-tasks).
> - Several urgent tasks run in the order in which they were submitted. They never interrupt each other. A stored urgent task that becomes ready again after its repeat or retry delay, or after `RESET`, queues behind the urgent tasks already waiting.
> - The level is ignored for urgent tasks.
>
> The scheduler decides only between tasks: a running task is never interrupted for a more important normal task, only for an urgent one.
>
> ### How the next task is chosen
>
> Whenever nothing runs (when a task ends, after a change of the tasks and every `IDLE_CHECK_INTERVAL` while it waits), the scheduler decides what to start:
>
> 1. **Which tasks can run.** A task can run now only when all its constraints (`@window`, `@altitude`, `@airmass`, `@ha`, `@moon`, `@illumination`, `@twilight`) hold for its **whole duration**, from now until now + `@duration`. A target that would sink below `@altitude > 30` twenty minutes into a one-hour run does not start. Tasks that cannot run show `WAITING`.
> 2. **Urgent tasks first.** An urgent task that can run starts before everything else, in the order the urgent tasks were submitted.
> 3. **The best score wins.** The other tasks that can run are compared by their score: the class (`high`, `normal`, `low`), then the level plus the setting and altitude bonuses, see [Priorities](#priorities). The altitude bonus is 100 at the target's own culmination and 0 for a task without `@target`. A bonus of 0 never stops a task from running; it only makes it lose against tasks with a higher score.
> 4. **Start now, or as planned.** With the default strategy `COMPACT` the best task starts at once. The scheduler is then *greedy*: it never leaves the equipment idle while some task can run, and it does not look ahead in the night. With a short queue this means that all tasks run one after another from dusk, even when a target would be much higher a few hours later. With the strategy `VISIBILITY` the scheduler plans the whole night first, never with fewer runs than `COMPACT` would make, and places the runs where their targets stand highest; it starts the first run of that plan at its planned time and may wait for it, see [Choosing a strategy](#choosing-a-strategy).
> 5. **Nothing to start.** The scheduler shows the reason in `REASON` and checks again every `IDLE_CHECK_INTERVAL`. After `IDLE_HOOK_DELAY` minutes the `IDLE` hook runs, see [Hooks](#hooks).
>
> The night plan applies exactly the same rules to the rest of the night, so it shows the order and times the scheduler will follow, see [The night plan](#the-night-plan).
>
> (line 245) **Priority under each strategy.** `COMPACT` decides by priority at every start: whenever it can start something, it starts the best task that can run then (class first, then level and sky bonuses, see [Priorities](#priorities)). A `normal` task can run before a `high` one whose constraints do not hold yet. `VISIBILITY` uses priority only to decide which runs get into the night plan: it starts from the runs `COMPACT` would make and never plans fewer, see *What `VISIBILITY` respects* below. It then places the runs by when their targets stand highest, not by priority, so a `high` task may run after a `normal` one. Urgent tasks are the exception: `VISIBILITY` never moves them; they keep their `COMPACT` times in the plan and still start at once when they can.

About 900 words; average sentence length about 27 words; longest sentence 62 words.

### 4.2 After (concept part, sections 9, 10 and the priority part of 11)

> ### Priorities
>
> The priority decides which task starts when several tasks can run at the same moment.
>
> ```
> // @priority high 80
> ```
>
> A priority has two parts:
>
> - the **class**: `low`, `normal`, `high` or `urgent`;
> - the **level**: a number from 0 to 100.
>
> A task without `@priority` is `normal 50`. A class without a level, such as `@priority high`, gets level 50.
>
> **The class decides first.** Any `high` task wins over any `normal` task. Any `normal` task wins over any `low` task.
>
> **Within a class, the score decides.** The score is the level plus two bonuses. Both bonuses depend on where the task stands in the sky now.
>
> - The **setting bonus** is largest for a task whose constraints will stop holding soonest. Examples are a target that is about to set and a window that is about to close.
> - The **altitude bonus** is largest for a target at its culmination. It compares the target's altitude now with the highest altitude that target reaches at your site. A low southern target at culmination therefore gets the same bonus as a high northern target at culmination.
>
> A task without `@target` gets no altitude bonus.
>
> **Example.** Two `normal` tasks can run at 21:00. Task A is `normal 60` and its target stays up all night. Task B is `normal 40`, and its target is near culmination and sets in an hour. Task B has the higher score and starts first.
>
> With the default settings, each bonus adds up to 100 points. The two bonuses together can outweigh the whole level range. You can change this with two settings:
>
> - Set `SETTING_WEIGHT` and `ALTITUDE_WEIGHT` to 0, and the level alone decides within a class.
> - Raise them, up to 4, and the sky position counts more.
>
> Even at the highest weights, the bonuses never lift a task into a higher class.
>
> **Equal scores.** When two tasks have the same score, the task higher in the task list wins. `MOVE_UP` and `MOVE_DOWN` change the list order.
>
> **A bonus is not a condition.** Bonuses only rank the tasks that can run now. A task with no bonus still starts when it has the best score, or when it is the only task that can run. Only the constraints decide whether a task can run.
>
> **Urgent tasks.** The class `urgent` works differently. The level of an urgent task is ignored.
>
> - An urgent task starts before every other task, as soon as its constraints hold.
> - It interrupts a running task that is not urgent.
> - Urgent tasks never interrupt each other. They start in the order you submitted them.
>
> **A running task is not interrupted** for a task with a better score. Only an urgent task interrupts a run.
>
> **Hook tasks.** The class `hook` marks a task that only a hook runs. It has no level and no score.
>
> Details: [Score rules](#score-rules), [Urgent task rules](#urgent-task-rules), [Hook rules](#hook-rules).
>
> ### How the next task is chosen
>
> The scheduler decides what to start each time nothing runs:
>
> - when a run ends;
> - when you change a task, a setting or the strategy;
> - every `IDLE_CHECK_INTERVAL` seconds while it waits.
>
> Each decision has four steps.
>
> 1. **Find the tasks that can run now.** A task can run now when all its constraints hold from now until the end of its run. Example: a one-hour run on a target with `@altitude > 30`. If the target sinks below 30° after twenty minutes, the task cannot run now. It shows `WAITING` in the task list.
> 2. **Start an urgent task, if one can run.** When several urgent tasks can run, the one submitted first starts.
> 3. **Rank the other tasks.** The class decides first, then the score.
> 4. **Start the best task.** When it starts depends on the strategy:
>    - With `COMPACT`, the default, it starts now.
>    - With `VISIBILITY`, the scheduler follows its night plan. The next planned run may start later. Until then, `REASON` names the run and its start time.
>
> When no task can run, `REASON` shows why. It gives the reason of the first waiting task in the task list. After `IDLE_HOOK_DELAY` minutes without a run, the `IDLE` hook runs.
>
> > **Note:** While the scheduler estimates the duration of any task, only an urgent task can start. `REASON` is `estimating the task duration`. An estimate takes a few seconds after an upload or an edit.
>
> The night plan uses the same steps for the rest of the night. It therefore shows the order and the times the scheduler will follow.
>
> See also: [Choosing a strategy](#choosing-a-strategy), [The night plan](#the-night-plan), [Hooks and the night](#hooks-and-the-night).
>
> ### Choosing a strategy (opening part only)
>
> `AGENT_SCHEDULER_STRATEGY` decides how the scheduler uses the night. INDIGO clients show `VISIBILITY` as "Best visibility".
>
> - **`COMPACT`** starts the best task as soon as it can run. This is the default.
> - **`VISIBILITY`** plans the whole night first. It moves the runs to the times when their targets stand highest.
>
> **How each strategy uses priority.**
>
> - `COMPACT` uses priority at every start. It starts the task with the best class and score among the tasks that can run then. A `normal` task can therefore start before a `high` task whose constraints do not hold yet.
> - `VISIBILITY` uses priority only to choose which runs get into the plan. It starts from the runs `COMPACT` would make and never plans fewer. It then places the runs by when their targets stand highest, not by priority. A `high` task may therefore run after a `normal` one.
> - Urgent tasks are never moved. They keep their `COMPACT` times in the plan, and they start at once when they can.
>
> (The rest of the section continues with the quality of a run, the six-task example, and "When to use which", in the same style.)

The score formula, the 900-point bound and the setting-bonus horizon move to the reference:

> ### Score rules
>
> Within a class, the scheduler compares
>
> ```
> score = level + SETTING_WEIGHT × setting + ALTITUDE_WEIGHT × altitude
> ```
>
> - `level` is 0-100 (default 50).
> - `setting` is 100 when the task's constraints stop holding now. It falls linearly to 0 for constraints that hold 12 hours or more.
> - `altitude` is the target's altitude now in percent of its culmination altitude, at most 100. It is 0 for a task without `@target` and for a target below the horizon.
> - `SETTING_WEIGHT` and `ALTITUDE_WEIGHT` are 0-4, default 1.
>
> Each class adds 1000 points to the score of the class below (`low` 0, `normal` 1000, `high` 2000). Level and bonuses add at most 900 points, so they never cross a class.
>
> Edge cases:
>
> - Equal scores: the task higher in the task list wins.
> - An urgent task has no score. Urgent tasks start in submission order. A stored urgent task that becomes ready again after a repeat or retry delay, or after `RESET`, goes to the end of that order.
> - `@priority hook` takes no level. `@priority hook 50` is a header error.

Result: about 720 words in the concept part plus about 200 in the reference (900 before); average sentence length about 12 words; longest sentence 24 words; no cross-reference inside a sentence; one rule ("any pending estimate blocks every non-urgent start") is now stated correctly where it was ambiguous (line 397).

The forbidden-keys list of `@priority hook` (line 220) moves unchanged to [Hook rules](#hook-rules) in a table "Keys allowed with `@priority hook`".

### 4.3 Verification of the sample against the code

All references are to the working tree at commit 1b3e9749b.

| Rule in the sample | Code | Result |
|---|---|---|
| Classes `low`, `normal`, `high`, `urgent`, `hook`; level 0-100 | `indigo_agent_scheduler_header.c:333-345` (`classes[]`, `parse_integer(tokens[1], 0, 100, ...)`) | Confirmed |
| Default `normal 50`; a class without a level keeps level 50 | `header.c:500` (`header->level = 50`), level set only when two tokens (`header.c:344`) | Confirmed |
| `@priority hook 50` is an error | `header.c:340-342` | Confirmed |
| Class base 0/1000/2000/3000 | `indigo_agent_scheduler_plan.c:68` (`class_base[] = { 0, 1000, 2000, 3000, 0 }`) | Confirmed (urgent never uses the score, see below) |
| Setting term `w × 100 × (1 − T/12 h)`, T capped at 12 h | `plan.c:69-71`, `SCHEDULER_SETTING_HORIZON (12 * 3600)` in `indigo_agent_scheduler_sky.h:40`, cap documented at `sky.h:117` | Confirmed |
| Altitude term `w × min(100, 100 × alt / culmination alt)`, 0 without target or below horizon | `plan.c:72-79` | Confirmed |
| Weights 0-4, default 1 | `indigo_agent_scheduler.c:3120-3121` | Confirmed; max 100 + 400 + 400 = 900 < 1000 |
| Equal scores: higher in list wins | `select_task()` `indigo_agent_scheduler.c:1641-1670` iterates `PRIVATE_DATA->order` (list order) and replaces only on `value > best_score`; the plan does the same (`plan.c:288`) | Confirmed |
| Urgent first, in submission order, level ignored | `select_urgent()` `scheduler.c:1617-1630` (lowest `urgent_order`); plan gives urgent tasks value 0 and orders by `urgent_order` (`plan.c:279-289`) | Confirmed |
| Stored urgent task ready again after delay or `RESET` goes to the end | `scheduler.c:1597-1600` (`urgent_order = ++urgent_counter` when RETRY/REPEAT becomes QUEUED), `scheduler.c:2837` (reset) | Confirmed |
| Urgent interrupts a running non-urgent task, not a hook, not after `STOP` | `scheduler.c:1775-1781` (`kind == RUN_TASK && !is_urgent(task)`, `!stop_requested`) | Confirmed |
| Running task never interrupted for a better score | No other call of `begin_stop(..., STOP_PREEMPT)` than the urgent path above | Confirmed |
| Decision triggers: run end, task/setting/strategy change, every `IDLE_CHECK_INTERVAL` | `dirty = true` on task changes (e.g. `scheduler.c:2658-2852`), strategy `scheduler.c:2997`, settings `scheduler.c:3005`; `next_check = now + IDLE_CHECK_INTERVAL` (`scheduler.c:2012`) | Confirmed |
| Can run = constraints hold from now to now + duration | `eligible_now()` `scheduler.c:1575-1578` | Confirmed |
| Tasks that cannot run show `WAITING` | `scheduler.c:1655-1660` | Confirmed |
| Any pending estimate blocks every non-urgent start; reason `estimating the task duration` | `scheduler.c:1647-1653` and `1673-1677` | Confirmed; corrects the ambiguous line 397 |
| `REASON` = reason of the first waiting task in list order | `first_reason` set once, in list order (`scheduler.c:1656-1659`) | Confirmed |
| `VISIBILITY`: next planned run may start later; `REASON` names it | `select_planned()` `scheduler.c:2503-2530` (`"%s starts at %02d:%02d UTC, planned for better visibility"`) | Confirmed |
| Urgent tasks never wait for the `VISIBILITY` plan | `scheduler.c:1679` (`!is_urgent(...)` before `select_planned`) | Confirmed |
| `VISIBILITY` starts from the `COMPACT` runs, never plans fewer | `indigo_scheduler_plan_visibility()` `plan.c:1114` (COMPACT plan first), `plan.c:1210` (fewer runs → `NO_ARRANGEMENT`, COMPACT plan kept); `vis_better()` `plan.c:1008-1016` | Confirmed |
| `VISIBILITY` places runs by quality, not priority; added runs chosen by value | `vis_add_left_out()` `plan.c:945-970`, `vis_improve()` `plan.c:973-997` compare only `value` | Confirmed |
| Urgent runs keep their `COMPACT` start | `plan.c:1159-1166` (pin at the COMPACT minute) | Confirmed |
| `IDLE` hook after `IDLE_HOOK_DELAY` minutes without a run | `scheduler.c:2079-2080` | Confirmed (the `VISIBILITY` exception for a run within the delay goes to Strategy rules) |
| Estimate takes "a few seconds" | Wording from the current guide (line 628); the ten-second timeout is in the estimate code | Kept as in the current guide |

---

## 5. Verification plan for the full rewrite

### 5.1 Correctness

1. **Rule inventory.** Before writing, the implementer extracts every normative statement of the current guide into a checklist in a ticket note (one line per rule, with its current line number). Expected size: about 350 rules.
2. **Rewrite against the inventory.** Each rule in the new guide is marked in the checklist with its new section. A rule is either kept, merged with a named duplicate, or removed with a reason. Nothing is dropped silently.
3. **Code check.** Every rule is checked against `indigo_drivers/agent_scheduler/` (`indigo_agent_scheduler.c`, `_plan.c`, `_header.c`, `_night.c`, `_sky.c`) and, for sequencer steps, `indigo_drivers/agent_scripting/library/Sequencer.js` docs. The checklist records the file and line. Statements found wrong in the current guide are not silently fixed: they are listed in the ticket note and raised with the owner.
4. **Numbers and examples.** All limits and defaults are compared with the `indigo_init_*_item()` calls and the header parser. The worked-example tables and the two-nights tables are copied unchanged; their numbers are already produced by `bench_scheduler_plan --examples` and `tools/scheduler_plan_charts.py`. Optionally the implementer reruns them to confirm (only if the owner wants a build).
5. **Messages and reasons.** Every quoted message and reason is found verbatim in the source with `grep`.
6. **Links.** A script (standard library only, in the scratchpad) lists every `](#anchor)` and checks it against the GitHub anchors generated from the headings, and checks the external anchors of chapter 2.2. It also checks that headings are unique.
7. **Correctness review.** A reviewer receives the diff, the rule checklist and the code. They check every changed rule against the code, not against the old guide alone.

### 5.2 Readability

1. **Metrics** (reported, not a gate): average and maximum sentence length per section (target 15 and 25 words), number of sentences over 25 words, number of mid-sentence links (target 0), word count.
2. **Terminology check.** `grep` for the banned terms of table 3.5 (eligible, labelled, hook-only, best visibility in text, greedy, task run).
3. **Fresh-eyes review.** A second reviewer, who has not seen the old guide or the design, reads only the new guide and answers a fixed set of questions from it, for example: "Why did my `high` task run after a `normal` one?", "When does `RESUME` run?", "Where do the frames of a hook go?", "What happens to the running task when an urgent task arrives?", "How do I stop a run at 02:00?". The reviewer reports passages that needed rereading.

### 5.3 Estimated length

About 12,000-13,000 words (current: 15,183). Removing the repeated rules (table 1.6) saves about 2,000 words. Shorter sentences add some words back. The reference tables keep their size. Line count will rise somewhat, because rules become bullets.

### 5.4 Files touched by the implementation

- `indigo_docs/INDIGO_SCHEDULER_GUIDE.md` only.
- No change to `SCHEDULER.md`, `HOME.md`, `PROPERTIES.md`, the charts, the Xcode project or any `README.md`, because all external anchors are kept.

---

## 6. Open questions for the owner

1. **Score formula in the guide.** The current guide describes the bonuses in words only; `SCHEDULER.md` has the formula. Should the reference part of the guide show the formula (as in the sample "Score rules"), or keep words only and link to `SCHEDULER.md`?
2. **Corrections found while rewriting.** One ambiguity already found (line 397, any pending estimate blocks all non-urgent starts) is a wording fix. If the rewrite finds a statement that is wrong against the code, should the implementer fix the guide in this ticket and list it in a note, or stop and open a separate ticket?
3. **Unpark example and the meridian flip.** The `Unpark` hook (line 684) calls `enable_meridian_flip()` and is named as both `STARTUP` and `RESUME` (line 693), so the flip is set again on every resume. The several-nights example (lines 734-775) has no `STARTUP` hook at all, so it does not set the flip. Should the rewrite split the examples so that only a `STARTUP` hook sets the flip, matching your decision "set once in the `STARTUP` hook"?
4. **Planned features.** The guide mentions two planned features: selecting the Scripting Agent (line 37) and `@timeout` (line 428). Keep them as one "Not yet available" line each in the reference, or remove them from the user guide?
5. **Sandbox and chart regeneration.** Move "Trying the scheduler by hand" and "Regenerating the charts" to an appendix as proposed, or keep the sandbox in the main text?
6. **Header block.** Keep the current revision/author/e-mail block at the top, and update the revision date?
7. **Length.** Is 12,000-13,000 words acceptable, or do you want a stricter target (for example under 10,000) at the cost of moving more detail to `SCHEDULER.md`?
8. **Generated PDF.** The primary checkout has an untracked `indigo_docs/INDIGO_SCHEDULER_GUIDE.pdf`. It is not part of the repository and this ticket will not touch it; regenerate it yourself after the rewrite if you use it.
