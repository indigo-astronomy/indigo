# Design: configurable agents and scheduler takeover of the sequencer

Ticket: b7873f3d-bdd7-47c8-8e36-c5014ed5cd4f
Components: `indigo_drivers/agent_scheduler/indigo_agent_scheduler.c`, `indigo_drivers/agent_scripting/library/Sequencer.js`, `indigo_libs/indigo/indigo_names.h`, documentation, integration tests.

This design implements the points agreed with the owner on 2026-11-26, as revised by the owner's rejection of the first draft: **the lock applies only to the choice of agents**. While the scheduler owns the sequencer, any client may start any sequence; every `start()` then runs on the owner's agents. There is no permit and no refusal of sequences. The second revision applies the owner's answers to the open questions: the agents are chosen from per-role lists of the agents on the bus (Q1), the Scripting Agent gets a manual release of the owner (Q2), `select_solver_agent()` is ignored under automatic solver choice (Q3), the guider abort check stays as it is (Q4), no MCP documents are changed (Q5), two schedulers with the same name are documented as unsupported (Q6), and the web UI is not changed (Q7). The third revision makes a saved text property the configuration and turns the per-role lists into selectors of the agents on the bus that only fill it. What is still open is listed under [Decisions recorded and remaining questions](#decisions-recorded-and-remaining-questions).

## 1. Scheduler configuration

### 1.1 `AGENT_SCHEDULER_AGENTS`: the configuration

Text property of the Scheduler Agent, group `Agents`, label `Agents`, `INDIGO_RW_PERM`, state Ok; names in `indigo_names.h` next to the other `AGENT_SCHEDULER_*` names. It is the only saved and authoritative agent configuration.

| Item | Label | Default | Valid value (role convention) |
|---|---|---|---|
| `SCRIPTING` | Scripting agent | `Scripting Agent` | `Scripting Agent` or `Scripting Agent @ ...` |
| `IMAGER` | Imager agent | `Imager Agent` | `Imager Agent` or `Imager Agent @ ...` |
| `MOUNT` | Mount agent | `Mount Agent` | `Mount Agent` or `Mount Agent @ ...` |
| `GUIDER` | Guider agent | `Guider Agent` | `Guider Agent` or `Guider Agent @ ...` |
| `AUX` | Auxiliary agent | `Auxiliary Agent` | `Auxiliary Agent` or `Auxiliary Agent @ ...` |
| `SOLVER` | Solver agent (empty = automatic) | empty | empty, or `Astrometry Agent`, `Platesolver Agent`, `ASTAP Agent`, each also followed by ` @ ...` |

In this document `SCRIPTING`, `IMAGER`, `MOUNT`, `GUIDER`, `AUX` and `SOLVER` stand for the values of these items.

- **Names** are device names as the scheduler sees them, ` @ host` included: the sequencer works with remote agents (`start("Imager Agent @ observatory", ...)`, `sequencer_agent_host()`), so they are not stripped. Only the Scripting Agent's own suffix is stripped from the names sent to it (1.4).
- **Direct typing is allowed**: a client may change any items of the property directly. Validation of the resulting values (request merged into the current values): `SCRIPTING` to `AUX` non-empty, every non-empty value shorter than `INDIGO_NAME_SIZE` and matching the role convention in the table. A request that fails is rejected with Alert through `reject()` and nothing is applied. The agent does not have to be on the bus.
- **Persistence**: saved by `save_config()` (first, before `SETTINGS`) and restored through the configuration reader like the other properties. Without a saved value the defaults (today's names) apply, so existing setups do not change. A configured agent that is not on the bus stays configured; the scheduler behaves as today when that agent is missing (e.g. `waiting for the Scripting Agent`).
- **Change while the scheduler runs: refused** with Alert `"The agents cannot be changed while the scheduler runs, stop it first"` when `started` or `phase != RUN_IDLE`, also when the change comes from a selector (1.2). The configuration reader is never refused (the scheduler always attaches stopped). Rationale: the owner, the running task's FITS context, the busy baseline and the abort targets are tied to the agents at START.
- **After an accepted change** (handler thread), for each item whose value changed: the name is copied into `observed` under `observed.mutex` (1.5); the observed state of that role is cleared as if the old agent had been deleted (`scripting_present`, `sequencer_present`, `sequence_state`, owner fields and flip with `flip_generation++` for the Scripting Agent; `site_known` for the mount; `safety_defined`, `safety_light`, `safety_seen` for the aux; `imager_known` and `original_valid` for the imager; the role's bit of `agents_busy`); `library_sent = 0` for the Scripting Agent; `indigo_enumerate_properties(agent_client, &all)` re-learns the bus as `agent_client_attach()` does; `dirty`, `plan_dirty` and `plan_changed` are set; the selectors are updated (1.2). An estimate in flight to the old Scripting Agent times out (`ESTIMATE_TIMEOUT`), as today.

### 1.2 Selectors: the agents of each role on the bus

Six switch properties, group `Agents`, `INDIGO_RW_PERM`, `INDIGO_AT_MOST_ONE_RULE` (no item is on when the configured agent is not on the bus), state Ok, **not saved**:

| Property | Label | Lists the devices on the bus named by the convention of |
|---|---|---|
| `AGENT_SCHEDULER_SCRIPTING_AGENT_LIST` | Scripting agents | `SCRIPTING` |
| `AGENT_SCHEDULER_IMAGER_AGENT_LIST` | Imager agents | `IMAGER` |
| `AGENT_SCHEDULER_MOUNT_AGENT_LIST` | Mount agents | `MOUNT` |
| `AGENT_SCHEDULER_GUIDER_AGENT_LIST` | Guider agents | `GUIDER` |
| `AGENT_SCHEDULER_AUX_AGENT_LIST` | Auxiliary agents | `AUX` |
| `AGENT_SCHEDULER_SOLVER_AGENT_LIST` | Solver agents | `SOLVER`, plus the fixed first item `AUTOMATIC` (label `Automatic`) |

- **Items**: item name and label are the device name exactly as seen, ` @ host` included, as `FILTER_RELATED_AGENT_LIST` in `indigo_filter.c` does; device names and item names share the bound `INDIGO_NAME_SIZE`, so no mapping is needed. `AUTOMATIC` cannot collide with a solver device name. Capacity `MAX_ROLE_AGENTS` (16) agents per list (plus `AUTOMATIC`); further agents are not listed (logged once).
- **Value**: the item equal to the configured name is on, all others off; none is on when the configured agent is not on the bus. `AUTOMATIC` is on exactly when `SOLVER` is empty.
- **Selecting** an item (a request with that item on) is a change of `AGENT_SCHEDULER_AGENTS` that writes the item name into the corresponding text item (`AUTOMATIC` clears `SOLVER`), through the same validation and refusal as 1.1; on refusal the selector is answered with Alert and its previous value. A request with no item on, or naming an item that is not listed, changes nothing and republishes the selector unchanged with Alert.
- **Rebuilding**: the client callbacks recognise an agent by its `INFO` property: a define of `INFO` of a device whose name matches a role convention adds the name, a deletion of `INFO` or of the whole device (empty property name) removes it. The callbacks only record the present names in `scheduler_observed` under `observed.mutex` and increment `agents_generation`; they never touch the scheduler's properties. When the generation changed, the handler (each tick in `process()`) rebuilds the item list of a role from the present agents, sorted by name, sets the values from the configuration and, when the items changed, deletes and redefines that selector, as the filter agents redefine their device lists; when only the values changed (after a configuration change) it updates the selector.

### 1.3 Where the names are used

The five `#define`s `SCRIPTING_AGENT`, `IMAGER_AGENT`, `MOUNT_AGENT`, `GUIDER_AGENT`, `AUX_AGENT` are removed; `AUX_SAFETY_PROPERTY`/`AUX_SAFETY_SAFE_ITEM` stay. Every use takes the configured name:

| Use | Today | New |
|---|---|---|
| Library load, task, hook and estimate scripts (`load_library`, `start_task`, `process_estimation`) | `SCRIPTING_AGENT` | `SCRIPTING` |
| `SCHEDULER_RESULT` / `SCHEDULER_ESTIMATE` device name *inside* the JS wrapper | `SCRIPTING_AGENT` | the JS-local name of the Scripting Agent: `SCRIPTING` without its ` @ host` suffix (the script defines properties under the agent's local name; the scheduler sees them as `SCRIPTING`) |
| Observation of `AGENT_SCRIPTING_RUN_SCRIPT`, `SEQUENCE_*`, `SCHEDULER_*`, `MERIDIAN_FLIP`, new `SEQUENCER_OWNER` | `SCRIPTING_AGENT` | `SCRIPTING` |
| Reason `waiting for the Scripting Agent` | literal | unchanged text (the role, not the name) |
| Site for planning (`GEOGRAPHIC_COORDINATES`) | `MOUNT_AGENT` | `MOUNT` |
| Safety (`AGENT_AUX_SAFETY`) | `AUX_AGENT` | `AUX` |
| FITS/data context (`apply_context`, `restore_context`, `set_fits_header`, `CCD_LOCAL_MODE` observation) | `IMAGER_AGENT` | `IMAGER` |
| `send_abort()` | Scripting + `GUIDER_AGENT` | `SCRIPTING` + `GUIDER` |
| Busy tracking (`agent_busy_bit`) | Imager/Mount/Guider names, `Astrometry Agent`, `Platesolver Agent` | `IMAGER`, `MOUNT`, `GUIDER`; solver = `SOLVER` when configured, otherwise the `SOLVER` item of `SEQUENCER_OWNER` (the solver the sequencer actually selected, which covers `ASTAP Agent` and remote solvers) |
| Owner object sent to Sequencer.js (section 2) | - | `IMAGER`, `MOUNT`, `GUIDER`, `SOLVER`, with the Scripting Agent's own ` @ host` suffix stripped (1.4) |

### 1.4 Remote Scripting Agent

Names are those the scheduler sees, while a script on a remote Scripting Agent knows the agents by the names on its own server. The scheduler therefore strips the Scripting Agent's own ` @ host` suffix from the configured names with the same suffix before it sends them in the take script, and uses the Scripting Agent's name without suffix inside the scripts (table in 1.3). Names with another suffix or none are sent unchanged; the guide states that the configured agents must then be reachable from the Scripting Agent's server under those names.

### 1.5 Thread safety

`observe_property()` and `agent_delete_property()` run on bus threads and today compare against literals. The configured names are mirrored in `scheduler_observed` (`char agents[6][INDIGO_NAME_SIZE]`), written only by the handler under `observed.mutex`, read by the callbacks under the same mutex they already hold. The present agents recorded by the callbacks (`char present[...][INDIGO_NAME_SIZE]`, `agents_generation`) are written by the callbacks and read by the handler under the same mutex. The handler reads its own copy of the configuration from the text items. Nothing new is called on the bus while holding the mutex.

## 2. Sequencer.js: owner object and `SEQUENCER_OWNER`

### 2.1 The owner global

```js
// the client that controls the sequencer, kept by a reload like the running sequence and the flip; null when free
var indigo_sequencer_owner = typeof indigo_sequencer_owner !== "undefined" && indigo_sequencer_owner != null ? indigo_sequencer_owner : null;
// { name: "Scheduler Agent", imager: "...", mount: "...", guider: "...", solver: "" /* "" = automatic */ }
```

Placed in the "Library reload" section next to `indigo_sequencer_running` / `indigo_flipper_enabled` / `indigo_meridian_flip`, using the same self-preserving pattern as `indigo_meridian_flip`. The owner lives only in the Duktape heap: a restart of the Scripting Agent clears it (the scheduler then takes it again, section 3.2); it is never saved.

Global functions (all names start with `indigo_sequencer_`, defined in Sequencer.js):

| Function | Effect |
|---|---|
| `indigo_sequencer_take(owner)` | Sets `indigo_sequencer_owner` to a new object with the given `name`, `imager`, `mount`, `guider`, `solver` (`String()` of each, missing solver = `""`). When no sequence runs, applies the owner's agents (`indigo_sequencer_apply_owner()`). Publishes `SEQUENCER_OWNER`. Sends `"The sequencer is controlled by <name>"` only when the owner name changes from none/another. |
| `indigo_sequencer_release(name)` | Only when the current owner's name equals `name`: sets the owner to null, publishes `SEQUENCER_OWNER` empty and sends `"The sequencer is released by <name>"`. Otherwise does nothing. The agents of `indigo_sequencer.devices` stay as they are. |
| `indigo_sequencer_apply_owner()` | Writes the owner's imager, mount and guider into `indigo_sequencer.devices`, and the solver: `owner.solver` when set, otherwise `sequencer_solver_agent(owner.imager)`; publishes `SEQUENCER_OWNER` when the solver changed. |

Main code at the end of the library: after the running sequencer/flipper are restored, `if (indigo_sequencer_owner != null && indigo_sequencer_running == null) indigo_sequencer_apply_owner();` so that a reload of an idle sequencer, which creates a fresh `indigo_sequencer` with default devices, keeps using the owner's agents.

### 2.2 `SEQUENCER_OWNER`

Defined by `indigo_sequencer.on_enumerate_properties()` on the Scripting Agent (`this.devices[SCRIPTING_AGENT]`), group `Sequencer`, label `Owner`, text, `RO`, state Ok:

| Item | Label | Value |
|---|---|---|
| `OWNER` | Owner | owner name, empty when the sequencer is free |
| `IMAGER` | Imager agent | owner's imager, empty when free |
| `MOUNT` | Mount agent | owner's mount, empty when free |
| `GUIDER` | Guider agent | owner's guider, empty when free |
| `SOLVER` | Solver agent | the configured solver, or the solver currently selected automatically; empty when free |

Updated by take/release/apply_owner and whenever the sequencer's solver choice changes under the lock (`indigo_sequencer.loaded_solver_agent()` and the flipper's solver assignment). Clients show the owner from `OWNER`; the scheduler reads `OWNER` (owner check and manual release, section 3.1) and `SOLVER` (busy tracking).

### 2.3 `start()` under the lock

`Sequence.prototype.start(imager_agent, mount_agent, guider_agent)`, whoever calls it (a scheduler task or hook, another client, a script by hand):

1. Without an owner: unchanged.
2. With an owner: for each argument that is given and differs from the owner's agent of that role, a part `"<role> agent '<requested>' ignored, '<configured>' is used"` is collected; when there is at least one, **one** message `"Sequence '<name>': <parts joined by '; '> (agents of <owner>)"` is sent. Then `indigo_sequencer_apply_owner()` and `indigo_sequencer.start(...)` as today. Omitted arguments and arguments equal to the owner's agents cause no warning. The sequence is never refused.

When a sequence is already running, `start()` neither warns nor applies the owner's agents (that would change the agents of the running sequence); it only calls `indigo_sequencer.start()`, which reports `"Other sequence is executed"` as today.

Duration estimates of the scheduler replace `Sequence.prototype.start` with their counting stub, so they produce no warnings.

### 2.4 `select_*_agent()` steps under the lock

`indigo_sequencer.select_imager_agent/select_mount_agent/select_guider_agent/select_solver_agent(agent)` at execution time: with an owner, `this.devices` is not changed; when `agent` differs from the owner's agent of that role, `indigo_send_message("Step select_<role>_agent('<agent>') ignored, '<configured>' of <owner> is used")` is sent (independently of `verbose`, as agreed "only warns"); the step then succeeds via `indigo_sequencer_next_ok_handler` as today. For `select_solver_agent` with an automatic solver choice the step is ignored as well and the comparison is with the solver currently selected automatically (owner's decision Q3). Without an owner: unchanged. The builder methods of `Sequence.prototype` are unchanged (the check is at execution, where the lock state is current).

### 2.5 Solver under the lock

- `owner.solver` set: `sequencer_loaded_solver_agent()` returns `owner.solver` when it is loaded and `null` otherwise, with no fallback to another solver. This one change covers its three callers: `indigo_sequencer.loaded_solver_agent()` (a solver step then fails with the existing missing-property message naming the configured solver), `set_use_solver()`, and the flipper (`"Meridian flip failed (no solver agent is loaded)"`).
- `owner.solver` empty: today's automatic selection, seeded with the owner's imager (`sequencer_solver_agent(owner.imager)`), including the existing re-selection at use; each change of the selection is published in `SEQUENCER_OWNER.SOLVER`.

### 2.6 Meridian flip under the lock

In `indigo_sequencer.on_update()`, where the pause after transit starts the flip (`indigo_flipper.devices = this.devices`): with an owner, `indigo_sequencer_apply_owner()` is called first when no sequence runs, so the flipper always gets the owner's imager, mount, guider and solver (a flip enabled by an earlier sequence and triggered while idle under the lock included). Because `take()` applies the owner's agents to an idle sequencer, the trigger check `property.device == this.devices[IMAGER_AGENT]` already watches the owner's imager. No other flipper change. A sequence that was already running when the lock was taken keeps its own agents until it ends (the lock does not rewrite a running sequence); every sequence started afterwards, by anyone, uses the owner's agents.

### 2.7 What is not changed in Sequencer.js

`AGENT_ABORT_PROCESS`, pause, `SEQUENCE_RESET`, estimate counting and every step other than the four `select_*_agent()` steps behave as today. The guider check in `indigo_sequencer.abort()` (`startsWith("Guider Agent")`) stays: guider agent names follow that convention (owner's decision Q4).

### 2.8 Manual release: `SEQUENCER_RELEASE`

A writable switch defined by `indigo_sequencer.on_enumerate_properties()` on the Scripting Agent: `SEQUENCER_RELEASE`, group `Sequencer`, label `Release owner`, item `RELEASE` (label `Release`), `RW`, `ONE_OF_MANY`, state Ok, defined like `SEQUENCE_RESET`.

Handled in `indigo_sequencer.on_change_property()`: when `RELEASE` is true and there is an owner, the owner is cleared exactly as `indigo_sequencer_release(owner.name)` does (`SEQUENCER_OWNER` updated empty), with the message `"The sequencer was released from <owner> by hand"`; the switch is reset with `indigo_update_switch_property(..., { RELEASE: false }, "Ok")`. Without an owner the switch is reset with state Alert and the message `"The sequencer has no owner"`. A running sequence keeps the agents it runs on (`indigo_sequencer.devices` is not changed); sequences started afterwards use the agents they ask for, as without an owner.

## 3. Scheduler side: taking and releasing the lock

New private data: `owner_sent` (monotonic time of the last take/release script), `owner_held` (the scheduler believes it holds the lock); new observed fields: `owner_defined`, `owner[INDIGO_NAME_SIZE]`, `owner_imager/mount/guider/solver`. Owner name = `SCHEDULER_AGENT_NAME` ("Scheduler Agent"). New constant `OWNER_RETRY_INTERVAL 5` (seconds).

Scripts sent to `AGENT_SCRIPTING_RUN_SCRIPT` of `SCRIPTING` (names emitted with `json_string()`):

- take: `if (typeof indigo_sequencer_take == "function") { indigo_sequencer_take({ name: ..., imager: ..., mount: ..., guider: ..., solver: ... }); }`
- release: `if (typeof indigo_sequencer_release == "function") { indigo_sequencer_release("Scheduler Agent"); }`

The `typeof` guards keep a Scripting Agent with an older library working (no lock, nothing fails). The task, hook and estimate wrappers (`wrapper_script()`) are not changed apart from the device name of `SCHEDULER_RESULT`/`SCHEDULER_ESTIMATE` (section 1.1).

### 3.1 Lifecycle

| Event | Action |
|---|---|
| `START` (`control()`), also when postponed by `HOLD_START` | `owner_held = true`; the take is sent by `process_owner()` as soon as the Scripting Agent and Sequencer.js are present (the library is loaded by `process_library()` while started, as today). |
| Each tick while `started` (idle, standby, unsafe hold, stopping with the shutdown hook pending) — new `process_owner()` called from `process()` after `process_library()` | Unless `owner_released`: if `scripting_present && sequencer_present` and the observed `SEQUENCER_OWNER` is missing or its `OWNER`, `IMAGER`, `MOUNT`, `GUIDER` (and `SOLVER` when configured) differ from the configuration, and `now - owner_sent >= OWNER_RETRY_INTERVAL` (or nothing was sent since START): send the take. |
| `start_task()` (task or hook) | Unless `owner_released`: when the observed owner is not yet the scheduler (e.g. the library was just reloaded after a Scripting Agent restart and the tick has not sent the take), the take is sent first, then the task script, both through `AGENT_SCRIPTING_RUN_SCRIPT` from the scheduler's handler, so the Scripting Agent runs them in that order (the implementer verifies the ordering of `RUN_SCRIPT` requests in `indigo_agent_scripting.c`). |
| Manual release (`SEQUENCER_RELEASE`, or any other script that clears the owner) while started — **recommended, open decision D1** | The scheduler recognises it as `SEQUENCER_OWNER.OWNER` changing from its own name to empty while the Scripting Agent and Sequencer.js stayed present (a restart of the Scripting Agent deletes them first and is therefore not a release). It sets `owner_released = true`, sends once `"The sequencer was released on the Scripting Agent, the scheduler leaves the agents to the tasks until the next START"`, and neither `process_owner()` nor `start_task()` takes the sequencer again until the next START (a scheduler restart starts stopped, so the release also lasts until then). The scheduler itself keeps using the configured agents for abort, busy tracking, safety, site and FITS context; tasks run on the agents they ask for. `stop_scheduler()` sends no release then. START clears `owner_released`. |
| `STOP` / `ABORT` / all tasks finished / stop timeout | All end in `stop_scheduler()`, after the shutdown hook ran. `stop_scheduler()` sends the release when `owner_held` and the Scripting Agent is present, then `owner_held = false`. |
| Scripting Agent or Sequencer.js deleted ("disconnect") | Nothing can be sent; the observed owner is cleared. An owner left on a Scripting Agent by a scheduler that is gone is cleared with `SEQUENCER_RELEASE` (section 2.8) or by a restart of the Scripting Agent. While started, the lock is taken again when the agent and the library are back (library reload by `process_library()`, then the take). |
| Scheduler driver shutdown / detach | In `INDIGO_DRIVER_SHUTDOWN`, after `stop_commands()` and before `indigo_detach_client()`: if `owner_held` and the Scripting Agent is present, send the release (the client must still be attached). |
| Stale lock: not started and the observed `OWNER` equals our name (scheduler restarted while it held the lock, or a release that was lost) | `process_owner()` sends the release, rate-limited by `OWNER_RETRY_INTERVAL`. |

### 3.2 Library reload and Scripting Agent restart

- Reload by a client (clients send Sequencer.js before their scripts): the owner global survives (section 2.1); the idle sequencer re-applies the owner's agents; `SEQUENCER_OWNER` is re-defined with the same values by the final `indigo_enumerate_properties()`. The scheduler sees no change and sends nothing.
- Restart of the Scripting Agent: the heap is new; the scheduler sees `RUN_SCRIPT`/`SEQUENCE_STATE` deleted and redefined, reloads the library while started, sees `SEQUENCER_OWNER.OWNER` empty and sends the take within one tick (and at most every `OWNER_RETRY_INTERVAL`). A running task fails as today (`the Scripting Agent or Sequencer.js disappeared`). A sequence started by another client between the library load and the take runs with its own agents; accepted as part of the cooperative lock (agreed point 5).

### 3.3 Busy tracking, abort, safety, site and FITS context

Only the names change (table in 1.3); the logic of `process_stopping()`, `evaluate_safety()`, `refresh_sky()`, `apply_context()`/`restore_context()` is unchanged. When `SEQUENCER_OWNER.SOLVER` changes, the solver bit of `agents_busy` is cleared (it is set again by the next Busy update of the new solver).

### 3.4 Sequences started by other clients

Unchanged: another client may start a sequence at any time; the scheduler treats it as today (`another sequence is running`, an urgent task preempts it, the unsafe hold aborts it once and leaves one started during the hold alone). The only difference is that such a sequence, started while the scheduler owns the sequencer, runs on the owner's agents (with the warning of section 2.3 when it asked for others), so the abort, busy tracking and flip of the scheduler match the agents it actually uses.

## 4. Compatibility

- The defaults of `AGENT_SCHEDULER_AGENTS` equal today's names; an old configuration loads unchanged. Setups with the default names see no behaviour change apart from `SEQUENCER_OWNER` and the messages when the sequencer is taken and released.
- Tasks that call `start('Other Imager', ...)` or `select_*_agent()` now run on the configured agents with a warning. Such setups must configure their agents in `AGENT_SCHEDULER_AGENTS`, by typing or through the selectors (documented in the guide).
- Sequences of other clients, started while the scheduler runs (START..STOP), are not refused; they run on the scheduler's agents, with a warning when they asked for others. To run a sequence on other agents, stop the scheduler first.
- Without an owner Sequencer.js behaves exactly as today, so clients that never meet the scheduler are unaffected; the existing sequencer suite must pass unchanged.
- A client that loads an older Sequencer.js copy replaces the functions; the owner global survives but the old `start()` and steps ignore it. This is the accepted cooperative nature of the lock (point 5).

## 5. Build, versions, project files

- The embedded copies are build products and not checked in: `indigo_drivers/agent_scheduler/Sequencer.js.dat` (rule in `agent_scheduler/Makefile.inc`) and `indigo_server/resource/Sequencer.js.data` (rule in `indigo_server/Makefile`) are regenerated by the build from `Sequencer.js`; no manual regeneration and no checked-in diff.
- `indigo_agent_scheduler`: `DRIVER_VERSION` 0x03000029 -> 0x0300002A.
- `indigo_agent_scripting` (owner of `library/Sequencer.js`): `DRIVER_VERSION` 0x03000015 -> 0x03000016; the C code of the agent is otherwise unchanged.
- `indigo_libs/indigo/indigo_names.h`: `AGENT_SCHEDULER_AGENTS_PROPERTY_NAME` with its six `_ITEM_NAME`s, the six `AGENT_SCHEDULER_*_AGENT_LIST_PROPERTY_NAME`s and `AGENT_SCHEDULER_SOLVER_AGENT_LIST_AUTOMATIC_ITEM_NAME`. This triggers the `unit` flow check.
- No new files, so no Xcode or Visual Studio project changes. `MIGRATION_STATUS.md` is not affected.

## 6. Documentation

- `indigo_docs/PROPERTIES.md`: rows for `AGENT_SCHEDULER_AGENTS` and the six `AGENT_SCHEDULER_*_AGENT_LIST` selectors in the Scheduler agent table; in the Scripting agent paragraph about Sequencer.js properties, `SEQUENCER_OWNER` and `SEQUENCER_RELEASE`; sentence in the Scheduler paragraph that the observed agents are those of `AGENT_SCHEDULER_AGENTS`.
- `indigo_docs/INDIGO_SCHEDULER_GUIDE.md`: "Agents the scheduler depends on" (replace "must run on the same server ... does not use it" by `AGENT_SCHEDULER_AGENTS`, the selectors, the naming convention, the remote Scripting Agent rule, solver busy incl. ASTAP; two Scheduler Agents with the same name controlling one Scripting Agent are not supported), "Starting, stopping and restarting" (from START to STOP every sequence, also one started by hand, runs on the configured agents; manual release lasts until the next START), "Rules for the body" (agents passed to `start()`/`select_*_agent()` are ignored with a warning), "Messages" (new messages), "Troubleshooting" (warnings about ignored agents, owner left by a scheduler that is gone: use `SEQUENCER_RELEASE`), "Property reference".
- `indigo_drivers/agent_scheduler/doc/SCHEDULER.md`: the agent table and the paragraph "The agent names are currently fixed ... A later phase lets the user select the Scripting Agent" are now stale and are updated (existing authoritative documentation of the agent).
- `indigo_drivers/agent_scripting/library/Sequencer.md`: "Selecting agents & devices" and "Executing the sequence" (behaviour under an owner), "Properties published during execution" (`SEQUENCER_OWNER`, `SEQUENCER_RELEASE`), "Messages" (ignored agents, controlled/released, released by hand), the take/release functions as an interface for controlling clients.
- No MCP document under `indigo_docs/mcp/` is changed in this ticket (owner's decision Q5); they are updated later.
- No web UI change (owner's decision Q7).

## 7. Test plan

Builds/checks: `flow check` (`unit` because of `indigo_names.h`), `make -C indigo_test` targets `test-agent-scheduler` (and its ASan variant `test_agent_scheduler_asan`), `test-agent-scripting-sequencer`, `test-agent-scripting`. The opt-in `test-scheduler-simulators` is not required (it may be run if the owner wants an end-to-end check with simulators).

### 7.1 Acceptance criteria -> tests

| # | Criterion | Test (suite: S = `test_agent_scheduler.c`, Q = `test_agent_scripting_sequencer.c`) |
|---|---|---|
| A1 | `AGENT_SCHEDULER_AGENTS` has today's defaults (empty `SOLVER`), is saved and restored, also when a configured agent is not on the bus at load | S `agents_default_to_the_fixed_names`, S `agents_survive_a_restart` (configure `Mount Agent @ obs2`, restart the scheduler without that stand-in: still configured, its selector has no item on; attach it: listed, on, used) |
| A2 | Typing: values outside the role convention, empty required items and changes while running are rejected, nothing applied | S `agents_typed_change_is_validated` (`Imager Agentx`, `Mount Agent` in `IMAGER`, empty `GUIDER`, `ASTAP Agent @ obs2` in `SOLVER` accepted), S `agents_change_refused_while_running` (text and selector) |
| A3 | Selectors list only the agents of the role on the bus (also ` @ host`), follow attach/detach, show the configured one, and are not saved | S `agent_selectors_follow_the_bus` (attach/detach stand-ins `Imager Agent @ obs2`, `ASTAP Agent`, a device `Imager Agentx`; detaching the configured agent leaves the text item unchanged and no item on) |
| A4 | Selecting writes the text item; `AUTOMATIC` clears `SOLVER`; a typed change updates the selector | S `agent_selector_fills_the_configuration` |
| A5 | Selected non-default agents drive site, safety, FITS context, abort and busy tracking | S `non_default_agents_are_used`: stand-ins `Mount Agent @ obs2`, `Imager Agent @ obs2`, `Guider Agent @ obs2`, `Auxiliary Agent @ obs2` configured; a `@target` task becomes eligible from the renamed mount's site, `LOCAL`/`SET` events reach the renamed imager, ABORT reaches the renamed guider (`guider_aborts`), an Alert of the renamed aux enters the hold; the default-named stand-ins are not attached. The Scripting Agent keeps its default name (the in-process agent cannot be renamed; limitation of the suite; the suffix stripping is covered by A13). |
| A6 | Solver busy tracking incl. ASTAP and a configured solver | S `astap_solver_busy_delays_the_next_task` (an `ASTAP Agent` stand-in with `AGENT_PLATESOLVER_EXPOSURE` and a Busy `AGENT_START_PROCESS` keeps the stop waiting after an abort until it goes Ok), S `configured_solver_busy_delays_the_next_task` |
| A7 | Owner from START to STOP incl. idle and standby, published read-only | S `lock_is_held_from_start_to_stop` (OWNER set after START with no runnable task, still set during the idle hook / standby and during `HOLD_START`, empty after STOP), Q `owner_take_and_release_publish_the_owner` (`SEQUENCER_OWNER` RO, items, release by another name ignored) |
| A8 | Any `start()` under the owner runs on the owner's agents, one warning per sequence when others were requested, never refused | Q `owner_start_uses_owner_agents_with_one_warning`, Q `owner_start_while_running_keeps_the_running_agents`, S `task_agents_are_ignored_with_a_warning` |
| A9 | `select_*_agent()` steps under the owner warn and continue, `select_solver_agent()` also under automatic choice | Q `owner_select_agent_steps_warn_and_continue` (all four steps, with a selected and with an automatic solver; next step runs; `devices` unchanged) |
| A10 | Solver under the owner | Q `owner_selected_solver_has_no_fallback`, Q `owner_automatic_solver_is_published` |
| A11 | Meridian flip uses the owner's agents | Q `owner_flip_uses_owner_agents` (take with custom imager/mount/guider/solver stand-ins, `run_flip_to_centering()`, assert `indigo_flipper.devices` and the CENTER request go to the owner's agents; also triggered while idle) |
| A12 | Release on stop, abort, finish, detach, disconnect; re-take after a Scripting Agent restart | S `lock_released_on_stop` (in A7), S `lock_released_on_abort`, S `lock_released_when_all_tasks_finished`, S `lock_released_on_scheduler_shutdown`, S `stale_lock_released_after_restart`, S `lock_retaken_after_scripting_agent_restart` |
| A13 | Remote Scripting Agent: own suffix stripped from same-host names | S `take_script_strips_the_scripting_agent_suffix`: with `SCRIPTING` = `Scripting Agent @ obs2`, the case checks the produced names (`Imager Agent` for `Imager Agent @ obs2`, `Mount Agent @ obs3` unchanged) in the script the scheduler sends to a stand-in device named `Scripting Agent @ obs2`, which defines `AGENT_SCRIPTING_RUN_SCRIPT`, `SEQUENCE_STATE` and an empty `SEQUENCER_OWNER` and records the scripts it receives |
| A14 | Manual release clears the owner, keeps a running sequence's agents, and lasts until the next START | Q `manual_release_clears_the_owner` (running sequence keeps `devices`, `SEQUENCER_OWNER` empty, a later `start('Other Imager')` uses it without warning; without an owner the switch ends Alert), S `manual_release_lasts_until_the_next_start` (release while started: message once, no take within 3 × `OWNER_RETRY_INTERVAL`, a task still runs; STOP sends no release; START takes the sequencer again) |
| A15 | Library reload keeps the owner | Q `library_reload_keeps_the_owner`, S `library_reload_keeps_the_lock` (extends `library_reload_keeps_scheduled_task`: no take is resent, OWNER unchanged) |
| A16 | Without an owner nothing changes; estimates give no warnings | the whole existing Q suite unchanged; S `estimates_do_not_warn_under_the_owner` |

### 7.2 Existing tests

No existing expectation changes. Re-checked: S `hand_started_sequence_blocks_all_but_urgent` and S `unsafe_hold_aborts_a_hand_started_sequence` start `manual.wait(...)` sequences with `start()` without agents; under the owner they still start (no refusal) and only `wait` steps run, so their agents are irrelevant and no warning is sent. They stay as they are and also cover A8's "never refused" end to end. Checking the agents of a hand sequence in the scheduler suite would duplicate the Q case of A8 and is not added.

## Decisions recorded and remaining questions

Owner's decisions (2026-11-26): Q1 agents configured in a saved text property, filled by typing or from per-role selectors of the agents on the bus, ` @ host` names kept (section 1, third revision); Q2 manual release on the Scripting Agent (section 2.8); Q3 `select_solver_agent()` ignored with a warning under automatic choice; Q4 no change of the guider abort check; Q5 no MCP document changes; Q6 two schedulers with the same name documented as unsupported; Q7 no web UI change.

Remaining open question:

D1 (recommended, please decide). **A manual release lasts until the next START** of the scheduler (or its restart, which starts stopped): the running scheduler does not take the sequencer again on its own, its tasks then run on the agents they ask for, while the scheduler keeps using the configured agents for its own abort, busy tracking, safety, site and FITS context. The alternative is that the scheduler takes the sequencer again on the next tick, which makes the release useful only for a scheduler that is stopped or gone.
