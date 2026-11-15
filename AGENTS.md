# AI Factory 2

This file is the bootstrap and usage guide for the coordinating agent. It is
not a rule source and must not define or restate rules; it points to the rule
files instead.

Rules are maintained in `.factory2/rules.json` (Factory rules) and
`.factory2/project-rules.json`, including nested
`DIRECTORY/.factory2/project-rules.json` files (project-owned rules). Read
them there, or through `config show`. The worker brief printed by `flow brief`
already contains the effective rule text for the files a ticket changes.

The flow and command references mentioned below (`docs/flow.md`,
`docs/cli.md`) are part of the Factory development repository and are not
installed in this project; here every command explains itself with `--help`.
The INDIGO conventions that are still kept in this file follow the Factory
guide under [INDIGO repository rules](#indigo-repository-rules).

## Commands

`.factory2/factory.py` is the entry point. Run it from the project root:

```sh
python3 ".factory2/factory.py" ticket list
python3 ".factory2/factory.py" ticket create --title "..." --description "..." [--priority P] [--severity S] [--profile NAME]
python3 ".factory2/factory.py" ticket confirm TICKET_ID [--priority P] [--severity S] [--profile NAME]
python3 ".factory2/factory.py" flow start TICKET_ID
python3 ".factory2/factory.py" flow check TICKET_ID [--check NAME ... | --command "..."]
python3 ".factory2/factory.py" flow review TICKET_ID --verdict pass|changes --text "..."
python3 ".factory2/factory.py" flow close TICKET_ID
```

`flow status TICKET_ID` lists what `close` still needs. `flow brief TICKET_ID
--role implement|review` prints the short assignment for a worker. Details are
in `docs/flow.md`.

## Seeing what a worker is doing

Flow commands and workers write short progress lines into the journal of the
task being worked on: when implementation, a check, a review or a stage starts
and how it ended, plus the worker's own lines.

```sh
python3 ".factory2/factory.py" flow log TICKET_ID --text "Writing the parser tests" [--task implement|review|STAGE]
python3 ".factory2/factory.py" flow log TICKET_ID --model MODEL [--provider P] [--reasoning R] --text "Implementer launched"
python3 ".factory2/factory.py" ticket task-journal TICKET_ID TASK_ID
```

- When the owner asks what a worker is doing, read the task journal (task IDs
  are in `ticket show TICKET_ID`) and report its latest lines; the Console
  shows the same journal for the selected task.
- A worker records a line with `flow log` when it starts a distinct piece of
  work and before it runs tests outside `flow check`; the briefs tell it so.
  Without `--task` the line goes to the running task. `flow log` only adds
  the line, and with `--model` records the model on that task's latest
  attempt.

## Changes go through a ticket

Work on this repository in this order; the governing rule text is in the rule
files named above.

1. Before touching any file, have a ticket. Look through `ticket list` for an
   open one that matches and reuse it; otherwise register the request with
   `ticket create`. Creating a ticket does not start it: tell the owner the
   ticket ID and ask whether to start it. Run `flow start` (or `flow stage`,
   `ticket start`) only once the owner has said so for that ticket; being
   told to create a ticket is not being told to start it.
2. Edit files only in that ticket's worktree, never directly in the primary
   checkout. The change reaches the base branch only through `flow close`.
3. This procedure also covers work on Factory itself, documentation and
   configuration. A question or a discussion that changes nothing needs no
   ticket.
4. If the primary checkout holds uncommitted changes that belong to no ticket,
   report them to the owner rather than building on them. Modified files
   under `.factory2/tickets/` of open tickets are expected there: Factory
   commits them itself or leaves them uncommitted, depending on
   `git/auto_commit`, so leave them alone. Which commits a ticket produces
   is described in `docs/flow.md`.
5. When the owner changes or adds to a request after its ticket exists,
   change the ticket with `ticket edit TICKET_ID --expected-revision N
   [--title "..."] [--description "..."]` or record the addition with
   `flow note TICKET_ID --text "..."`. `flow brief` prints the notes after
   the description, so the worker receives them.

## One ticket, step by step

1. `ticket create` registers the request and nothing more; in a Git project
   with `git/auto_commit` enabled it commits the new ticket's files. Pass the
   priority, severity and profile you propose (`--priority`, `--severity`,
   `--profile`). Then tell the owner the proposed values, each estimated one
   with a one-line reason, and ask them in one question to agree or to give
   other values; record the reply with `ticket confirm TICKET_ID`, adding
   `--priority`, `--severity` or `--profile` for what they changed. Until then
   the ticket reports the `ticket/confirm` wait. Once the owner
   has agreed to start that ticket, `flow start` creates the branch
   `ticket/TICKET_ID`, a worktree in
   `.factory2/local/worktrees/TICKET_ID` and prints the implementer brief.
2. Give the brief to one implementer. Before launching it, read the
   `recommendation` field of the `flow start` or `flow brief` result (and of
   `flow stage` for a stage worker): for each provider with a model policy it
   gives the tier, model and reasoning for that worker's task; `--provider P`
   limits it to one provider. The `models` rule in the rule files governs the
   choice. Record the model the worker actually runs on with `flow log
   TICKET_ID --model MODEL [--provider P] [--reasoning R] --text "..."`; the
   brief asks the worker to do the same, and the Console shows the model as a
   badge on the task. The
   recommended tier depends on the ticket's difficulty, set with `ticket
   create --difficulty low|medium|high` (default `unknown`) or `ticket edit
   --difficulty`. The implementer edits the worktree and runs
   `flow check` itself. A failing check returns the end of its output; the
   same implementer fixes the cause and repeats the command.
3. Give `flow brief --role review` to a second, fresh worker; requesting that
   brief also switches the time tracking from the Implement task to the
   Review task. The reviewer reads the diff and records its verdict with
   `flow review`. Notes from a `changes`
   verdict go back to the same implementer, followed by check and review
   again.
4. `flow close` puts the change on the base branch as one commit, archives
   the ticket in a second commit and removes the worktree. Committing, merging and pushing follow the `git` settings in the
   effective configuration; when committing or merging is not automatic,
   `close` says so, names the command (`flow close TICKET_ID` with
   `--commit`, `--merge` or both) and waits for it to be run on the owner's
   instruction.

## When a ticket waits for the owner

Results of `ticket list`, `ticket show` and every `flow` command carry `waiting_for_human` and `pending_human_actions`; the Console
shows the same. Whenever a command reports `waiting_for_human: true`, say so
in the conversation first, as a direct question to the owner that names the
ticket (ID and title) and the action asked for in `pending_human_actions`,
before doing anything else on that ticket.

- When you need the owner's decision about a ticket, record the question with
  `flow ask TICKET_ID --text "..."`, then ask it in the conversation. Record
  the reply with `flow answer TICKET_ID --text "..."`; the same command
  records the owner's decision after two review rounds asked for changes.
- When a stage file is ready, show it to the owner and record
  `flow present TICKET_ID --stage STAGE --file FILE`; their decision is
  recorded with `flow approve`.
- The indication is not a lock and is not stored: it goes away once the
  owner's action is recorded. The cases are listed in
  `docs/flow.md`.

## Larger changes

- Choose a profile with `flow profile TICKET_ID full` (or `full-ui`, or one
  from `config.json`); the owner confirms the choice. It can be changed later.
- Before implementation, `flow stage TICKET_ID --stage design` prints the
  brief for a design worker, who writes one file into the ticket directory.
  Present it to the owner, record that with `flow present`, and record their
  decision with `flow approve`.
- Split work with `ticket create --related-ticket-id PARENT`. Start the
  parent first when the parts must land together.
- Request additional reviews with `flow brief --role review --kind KIND` and
  `flow review --kind KIND`.

## When something goes wrong

Nothing in this flow is locked. A check or a review belongs to the exact file
tree it was recorded for, so any new edit simply makes it stale and it is run
again. An error message names the next command; act on it in the same ticket
instead of opening a replacement. Every command may be repeated after an
interruption.

Return to the owner when the review has asked for changes twice, when the
request itself is unclear, or when `close --unresolved --reason "..."` is the
right outcome.

## Tickets and tasks without Git

`ticket` and `task` commands record work that does not go through a branch:
`ticket start|pause|close`, `task create|start|finish|retry|cancel`. A larger
change is better split into several tickets than into many tasks. The full
command reference is in `docs/cli.md`.

# INDIGO repository rules

The following project requirements supplement the Factory workflow. Explicit owner instructions take precedence. Legacy incremental REVIEW.md tracking has been retired; record findings as Factory tickets. Exclude `indigo_test/shell_compliance/` from routine development and validation unless explicitly requested; the automated suite lives in the other `indigo_test/` subdirectories.

# Repository Guidelines

## Repository searches

- Do not run `rg` or `ripgrep`, including from scripts or tests. Use Git file inventories, `find`, `grep`, or standard-library searches instead. Exclude `.factory2/archive/` from routine discovery.

## Language

Write all project documents in English, including specifications, plans, ticket descriptions and notes, review reports, architecture decisions, README files, and agent instructions. Use English for new code comments, identifiers where practical, and commit/PR descriptions. User-facing product localization may use the languages required by the specification. Conversation with the owner may use their preferred language.

## Scope

- These instructions apply to the whole INDIGO repository. More specific instructions may exist in subdirectories.
- For driver work under `indigo_drivers/`, read `indigo_drivers/AGENTS.md` and follow it in addition to this file. It contains the driver-refactoring workflow and is more specific than this root file.
- For automated tests under `indigo_test/`, read `indigo_test/AGENTS.md` and follow it in addition to this file.
- Never modify `indigo_drivers/ccd_apogee`, `indigo_drivers/ccd_sbig`, `indigo_drivers/mount_asi`, `indigo_drivers/mount_mxhd` and `indigo_drivers/system_ascol`. These drivers are off limits: do not refactor, reformat, fix, test-instrument, or otherwise change any file in them, and do not include them in repository-wide sweeps. They may be read for reference only. If a task appears to require a change there, stop and ask the user first.

## Developer References

Read the relevant documentation before changing behavior:

- `README.md` covers platform requirements and top-level build commands.
- `indigo_docs/DEVELOPMENT.md` introduces the bus, device, client, and property model.
- `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` is the main reference for driver lifecycle, property semantics, INDIGO 3.0 APIs, portable I/O, and handler queues.
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` documents generated-driver migration.
- `indigo_docs/MAKEFILES.md` explains the makefile layers.
- `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` documents the host-side serial simulator contract and ready-file convention.
- `indigo_drivers/*/README.md` files document driver-specific hardware, prerequisites, connection details, limitations, and operational notes.
- `indigo_test/AGENTS.md` documents automated-test layout, harness conventions, simulator integration rules, and test cleanup.
- `.factory2/tickets/` holds the open Factory tickets, including the findings imported from the earlier registry, and `.factory2/archive/` the closed ones; list them with `ticket list` instead of reading the directories.

## Properties reference

The properties reference is maintained in `indigo_docs/PROPERTIES.md`. Keep this file up to date when changing source code. Whenever a property is added to or removed from any driver or framework code, update `indigo_docs/PROPERTIES.md` in the same change. Source files for documented property sections are listed in `indigo_docs/PROPERTIES.md` at the end of each section; use those per-section source notes as the authoritative mapping.

Driver-specific custom property names must start with `X_`. When implementing, refactoring or reviewing driver properties, check this prefix in `.driver` declarations and `indigo_init_*_property()` names. Standard INDIGO properties retain their standard names.

Always look for:
- indigo_init_*_property() function calls
- indigo_init_*_item() function calls
- *_PROPERTY->count = ... statements
- *_PROPERTY->hidden = ... statements

## Formatting

- `.editorconfig` is authoritative for basic whitespace:
  - UTF-8
  - LF line endings
  - tabs for indentation
  - indent size 2
  - trim trailing whitespace
- `uncrustify.cfg` defines C-family formatting. Match nearby code when it differs in small ways.
- Use K&R-style braces as seen in existing C files: `if (...) {`, `for (...) {`, `static void callback(...) {`.
- Separate function definitions with exactly one blank line. Do not put blank lines inside function bodies. Apply this also to hand-written code in `.driver` blocks. Whitespace emitted by the generator is exempt; do not modify the generator or hand-edit generated output to enforce this rule.
- Keep every function call and macro invocation on a single line, including all arguments; do not wrap calls to meet a line-length limit. Apply this also to hand-written code in `.driver` blocks. Formatting emitted by the generator is exempt; do not modify the generator or hand-edit generated output to enforce this rule.
- Always use braces for `if`, `for`, `while`, and `do` bodies.
- Surround operators with spaces.
- Do not put whitespace before a comma; put exactly one space after a comma when followed by another token on the same line, e.g. `INDIGO_DRIVER_DEBUG(DRIVER_NAME, "'%s' MaxStep = %d", device->name, PRIVATE_DATA->info.MaxStep);`. Apply this also to hand-written code in `.driver` blocks. Formatting emitted by the generator is exempt; do not modify the generator or hand-edit generated output to enforce this rule.
- Put a space after `{` and before `}` in inline brace-enclosed initializers and compound literals, e.g. `{ 0 }` rather than `{0}`. Apply this also to hand-written code in `.driver` blocks. Formatting emitted by the generator is exempt; do not modify the generator or hand-edit generated output to enforce this rule.
- Keep preprocessor defines aligned with tabs where the surrounding file does that.
- Do not reformat unrelated code or churn generated files.

## C and C++ Conventions

- Never add platform-dependent code to drivers. Use the portable INDIGO APIs, including `indigo_uni_io` for serial data and CTS/RTS control; keep OS-specific implementation in the shared I/O layer.
- Prefer variadic command helpers for formatted device commands, following the `dmfc_command` pattern: accept a format string and arguments, forward the `va_list` through `indigo_uni_vprintf()` or `indigo_uni_vtprintf()`, and format command parameters at the send point instead of building temporary command strings at each call site. Keep protocol-specific reply requirements explicit.
- When refactoring serial drivers, prefer `indigo_uni_discard()` for discarding pending input and `indigo_uni_read_section2()` for delimited replies instead of custom drain/read loops. Set explicit first-byte and inter-byte timeouts, reserve space for the terminating NUL, and validate complete reply termination and payload according to the device protocol. Use a custom reader only when a documented protocol requirement cannot be expressed through these APIs.
- Store the device response/read buffer in the driver private data and reuse it across serialized transactions instead of allocating a separate response array in each helper. Before a nested command reuses that buffer, copy any still-needed decoded values into local scalars or operation state.
- Prefer C-compatible and portable APIs; the project builds across Linux, macOS, and Windows.
- Use `static` for file-local functions and globals.
- Use INDIGO allocation, copy, logging, timer, async, and property helper APIs where existing code does.
- Validate callback inputs with `assert()` consistently with surrounding drivers.
- Avoid forceful process exits, blocking driver entry points, or long-running work on bus callbacks.
- Use `bool` for boolean values when the surrounding file does; some legacy code uses integer or bit storage.
- Keep comments useful and specific. Preserve copyright/license and version-history header style in new source files.
- Increment the version of every driver whose behavior you fix, including simulator drivers. For generated drivers, update `version` in the `.driver` source and regenerate the checked-in outputs; for hand-written drivers, update `DRIVER_VERSION`. Before finishing, verify that every fixed driver's version is higher than its pre-change version.

- Treat values entering driver `change_property` handlers as validated by the framework. Do not duplicate generic numeric range, integer-step or finite-value validation in drivers or test it through driver suites. Validate device/SDK/transport replies and driver-specific operational constraints such as BUSY conflicts. Use the BUSY checks already supplied by `INDIGO_COPY_*_PROCESS_CHANGE`; do not duplicate them in `on_change_request`, or cancel a previous finalizer when that guard already excludes unfinished work.

## Generated Drivers

Some drivers are generated from `.driver` files by `indigo_generator`.

- Modify the generator implementation, its DSL or generated lifecycle semantics only after explicit user approval of the concrete proposed change. Authorization to implement/refactor a driver or extend its tests does not authorize generator changes. First describe the limitation, proposed generator diff and impact on existing drivers, then wait for approval. Running the unchanged generator and editing a driver's existing `.driver` input remain ordinary authorized driver work.
- If a `.driver` source exists for a generated `.c` file, edit the generator input rather than hand-editing generated output unless the task explicitly requires otherwise.
- Keep generated output and checked-in generated files synchronized when the repository convention expects both.
- After completing any driver migration, update only the status columns of the driver's row in root `MIGRATION_STATUS.md` in the same change: API, Windows, generator, async queues, retesting and automated-test availability. Preserve the Comment column exactly; it is reserved for manual user input. Distinguish simulator validation from hardware validation in the retesting status, and document unverified platforms, remaining limitations and detailed results in the Factory ticket performing the migration, not in the Comment column. Keep both files consistent.
- During refactoring, use generator defaults unless the user explicitly requests an override. In particular, never override `MAX_DEVICES` with `#undef` / `#define` to preserve a legacy capacity; check that no such override is introduced.
- Review `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` before migrating or creating generated drivers.
- Treat the `.driver` file as the source of truth once migration starts. Check in the `.driver` file together with the regenerated `.c`, `.h`, and `_main.c` files, and add the `.driver` file to project files such as Xcode groups when relevant.
- When reverse-extracting a `.driver` file from an annotated C driver, run `indigo_generator -c <target>.driver`, where `<target>.driver` is the new generator source file to create. Do not pass the existing `.c` file as the `-c` argument; the generator treats that argument as the output path and can overwrite the hand-written source. If unsure, run the extraction in a temporary directory first and inspect the result before touching repository files.
- Keep custom code in generator-owned blocks: `include`, `define`, `data`, shared `code`, `<device>.code`, `<device>.on_attach`, `<device>.on_connect`, `<device>.on_disconnect`, `<device>.on_timer`, and property `on_change` blocks. Do not preserve old hand-written boilerplate just because it existed in the source driver.
- Name low-level connection helpers exactly `<driver_name>_open(indigo_device *device)` and `<driver_name>_close(indigo_device *device)`. Serial drivers take an extra parameter, `<driver_name>_open(indigo_device *device, const char **error_message)`, and pass `error_message` to the `indigo_uni_open_serial*2()` functions; the generated connection handler shows the message in its connection failure alert. The generator wires these into generated connection handlers and owns open/close reference counting for multi-device drivers through `PRIVATE_DATA->count`.
- Connection helpers must have a transactional contract: `<driver_name>_open()` returns true with all low-level resources acquired, or false after releasing every resource acquired by that attempt. Increment shared connection references only after successful open; call `<driver_name>_close()` only for a successfully opened session, on final disconnect or rollback of subsequent connection initialization. Do not add a persistent `opened` flag merely to compensate for calling close after a failed open; fix the lifecycle ownership instead.
- In generated `on_connect` blocks, use the generator-provided `connection_result` variable. Do not introduce a separate `ok` variable for the connection result, and do not `return` from `on_connect` or `on_disconnect` blocks because that can skip generator-owned cleanup, reference counting, property definition/deletion, messages, and final connection property updates.
- Let generated property handlers own their standard prologue and epilogue. The generator may set `PROPERTY->state = INDIGO_OK_STATE` before `on_change`, appends `indigo_update_property()` for most properties, and appends `indigo_update_coordinates()` for `MOUNT_EQUATORIAL_COORDINATES`. Inside `on_change`, set `INDIGO_ALERT_STATE` only on failure where possible instead of writing `PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE`.
- Use an empty `on_change { }` block for any writable property that only needs to accept new values. The generator treats the empty block as a simple change branch: it copies values or targets, sets the property state to `INDIGO_OK_STATE`, and updates the property without scheduling a handler. Omit `on_change` entirely for inherited properties that should not get a generated change branch.
- Be aware of generator mount exceptions. For `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_MOTION_RA`, `MOUNT_MOTION_DEC`, and `MOUNT_TRACKING`, the generator inserts a parked-mount guard before user `on_change` code. Do not duplicate that guard in the `.driver` source.
- Use the handler + finalizer pattern for long-running driver operations: the queued handler starts the operation, publishes BUSY and schedules a named finalizer; the finalizer performs a bounded progress check, reschedules itself while work remains, and publishes OK/ALERT on completion. Do not wait for motion, calibration, homing or similar completion in a blocking loop or sleep inside the initiating handler. Keep the queue available for abort/disconnect and define cancellation, timeout and failure recovery for the operation. Bounded individual transport transactions remain synchronous.
- Name delayed callbacks that complete an asynchronous property operation `<operation>_finalizer` (for example `guider_ra_finalizer`), including their declarations, scheduling and cancellation references. Do not retain generic `*_timer_callback*` names for completion handlers: `_finalizer` is semantically significant to the generator. After renaming, verify that the initiating `on_change` explicitly publishes BUSY/OK/ALERT as appropriate and the finalizer publishes completion; periodic polling callbacks are not finalizers.
- Minimize custom code in `on_change`: normally leave the standard `INDIGO_OK_STATE` assignment and final update of the handled property to the generator. This rule does not apply when the block contains `_finalizer`: the generator suppresses both the OK prologue and final property update, so the handler must own the required state assignments and updates. Do not rename a finalizer merely to avoid this exception.
- Do not add `indigo_cancel_pending_handler()` as a defensive prologue to a new operation already protected by the framework BUSY guard. Retain cancellation only for an identified pending operation, such as abort, supported replacement or a shared operation entered through different properties; document or test that concrete case rather than assuming one exists.
- After adding or removing any `_finalizer` reference in an `on_change` block, inspect the regenerated handler. Remove manual OK assignments and final updates when the generator supplies them; when the reference suppresses generated completion, ensure the handler publishes exactly the required start/error/completion updates. Do not rename callbacks or add wrappers merely to manipulate this textual rule. Never duplicate checks or updates already supplied by `INDIGO_COPY_*_PROCESS_CHANGE` or the generator.
- Use explicit property updates inside `on_change` only for early returns, custom messages, or deliberately self-managed asynchronous behavior. If an `on_change` block contains `_finalizer`, the generator suppresses its final `indigo_update_property()`; use that pattern only when the handler publishes its own busy/start update and delayed completion update.
- Generated change dispatch may appear to call `indigo_execute_handler(device, handler)` on a slave logical device. INDIGO's handler queue implementation internally queues work on `device->master_device` when present while invoking the callback with the original logical device, so do not add custom generator dispatch solely to force the master queue. In handlers reached from a slave device's `change_property`, treat the `device` argument as that slave logical device and use it directly for that device's property macros, updates and shared-handle helper calls. Introduce another logical-device variable only when the handler truly needs that other device's property context.
- For generated USB hot-plug drivers, use `libusb { hotplug = true; vid = ...; pid = ...; }` or `hid { hotplug = true; ... }`; `hotplug = true` is the default. The generator emits the libusb callback registration, `MAX_DEVICES`, attach/detach arrays and shutdown cleanup. A generated `libusb` driver must provide `<driver_name>_match(libusb_device *dev, const char **name)` in the shared `code` block; that helper filters devices and returns the display-name seed. The generator stores the matched `libusb_device *` in `PRIVATE_DATA->usbdev` and calls `<driver_name>_open(indigo_device *device)` / `<driver_name>_close(indigo_device *device)` from the generated connection handler, so do not preserve a hand-written hot-plug attach/detach framework unless the generator cannot express a required behavior.
- Generated hot-plug drivers use a per-driver handler queue in addition to per-device queues. The generator queues libusb arrival/removal work and `CONNECTION` handling on that per-driver queue so SDK enumeration, attach/detach, open and close operations are serialized across all instances of the same driver.
- For SDK-based USB hot-plug drivers whose vendor SDK uses its own ids or discovery records instead of direct `libusb_device *` handles, prefer `sdk { hotplug = true; vid = ...; pid = ...; plug { ... } unplug { ... } }` over hand-written hot-plug scaffolding. The generator reads the USB descriptor and applies `vid`/`pid` filtering before the `plug` block. The generated `plug` block runs with `libusb_device *dev`, `struct libusb_device_descriptor descriptor`, `<driver_name>_private_data *private_data`, `char name[INDIGO_NAME_SIZE]` and `bool plug_result`; set `plug_result = false` to reject the event, and fill `private_data` plus `name` to attach the generated device. The generated `unplug` block runs with `indigo_device *device`, `<driver_name>_private_data *private_data` and `libusb_device *dev` before detach/free; release SDK-side reservations there.
- For inherited properties, prefer generator attributes such as `hidden`, `persistent`, `asynchronous_change`, and `preserve_values` over hand-written attach/change boilerplate. Remember that `MOUNT_EQUATORIAL_COORDINATES` is finalized by the generator with `indigo_update_coordinates()`.
- Generated public headers expose the driver entry point and normally do not keep private device-name macros from the old hand-written driver. Update tests to use generated device names or public APIs instead of relying on removed private macros.
- After editing `.driver`, run `indigo_generator`, build the driver, and run the narrowest available simulator or integration test. Also inspect `git diff` afterward: large generated diffs are expected, but verify that every behavioral difference is explained by generator semantics.
- When refactoring CCD drivers, audit countdowns for both single exposures and streaming. Use the shared CCD countdown through `indigo_ccd_exposure_setup()` for ordinary exposures instead of duplicating it in device handlers. Where a separate countdown is required (for example streaming), derive remaining time from `indigo_monotonic_time()` and round only the published countdown upward to whole seconds with `ceil()`, clamping at zero. Preserve the exact requested duration, subsecond exposure timing, and SDK/shutter deadlines independently of the displayed value; never use a rounded property value to time readout or close the shutter. Do not decrement a fractional property value once per sleep or use wall-clock time for elapsed exposure timing. Verify fractional durations longer than one second, subsecond exposures, completion, abort/restart, and shared-countdown progress while the device queue is occupied.

## Driver Testing

For CCD, mount, wheel, focuser, rotator, guider, AO and GPS validation, follow the shared scope and class standards in [Driver Testing Rules](indigo_test/DRIVER_TESTING_RULES.md).

- Every driver refactoring must include full applicable test coverage of its supported capabilities and driver-specific behavior, using the class standard as the acceptance checklist. Cover protocol commands and readback, property transitions, failure/recovery paths, lifecycle and concurrency; a smoke test alone is not completion. Record the scenario-to-test mapping and justify non-applicable cases or hardware-only gaps in the Factory ticket.
- Audit the simulator as part of each refactoring against the supplied manufacturer protocol documentation (including bundled XLSX documents). For serial simulators with motion, use `indigo_test/simulator_common/serial_motion.h` unless a documented protocol requirement cannot be represented by it. Validate elapsed-time motion, stop and sync through the simulator protocol; do not count testing the shared helper itself as driver coverage.

### Requested Test Runs

- Every requested test run is one of four modes: interactive simulator, non-interactive simulator, interactive hardware, non-interactive hardware. If the request does not identify the mode unambiguously, ask before running anything; never assume a mode.
- In all modes, run and repeat the tests according to this file, `indigo_drivers/AGENTS.md` and `indigo_test/AGENTS.md`. Every rerun after a fix repeats the full requested scope, not only the failing case, and the run is finished only when that full scope is clean.
- Non-interactive simulator run: exercise the driver through the simulator, fake SDK, fake USB or another hardware-free backend. Fix every defect found, rerun, then commit the result. Do not push.
- Interactive simulator run: same execution, but do not change code on your own. Report each defect with a proposed fix, wait for the user's approval, implement the approved fix, rerun, then commit. Do not push.
- Non-interactive hardware run: exercise the driver against the physical device. Fix every defect found, rerun, then commit the result. Do not push.
- Interactive hardware run: same execution against the physical device, with every fix proposed and approved before it is implemented, then rerun and commit. Do not push. Before the run starts, ask whether physical hot-plug (unplug/replug) is part of it; if it is, request each plug/unplug action at the point it is needed and record its result, and if it is not, state in the report that hot-plug coverage was not established.
- Each driver is a separate commit unit. Commit one driver per commit, together with that driver's fixes, its `README.md` test record and the regenerated `TEST_SUMMARY.md`; never combine several drivers in one commit, even when they were tested in the same session.
- The final run of every test run, the one whose result is recorded, is always made with `python3 tools/run_driver_test.py <driver>` (with `--hw` or `--hot-plug` for hardware), never by running test binaries or make targets by hand. Intermediate runs while a defect is being tracked down may use anything, but the verdict comes from the script, so the build, the selection of tests, the counts and the record are the same for people and agents, and a defect in the script or the harness shows up in the next ordinary run instead of staying hidden. When the script cannot run a driver's tests correctly, fix the script or the test naming rather than working around it, and say so in the report.
- The script records the run in the driver's `README.md` `## Testing` section and regenerates `TEST_SUMMARY.md`, following the recording rules in `indigo_test/AGENTS.md`. This record is the only `README.md` change that needs no separate approval. A run is not finished until both files are updated and committed with the driver's change.
- Commit the verified result of a finished run; never push it. Report defects that stay unfixed, scope that was skipped, and any hardware-only gaps instead of reporting the run as passed.

## Repository Hygiene

- Do not modify any `README.md` file without the user's explicit approval.
- Xcode projects must not reference `.factory2/` or any of its contents, including navigator groups, folder/file references and build membership. Factory metadata changes must not trigger Xcode project edits.
- Add every newly created persistent repository file to the appropriate group in `indigo.xcodeproj/project.pbxproj` in the same change, including source files, headers, `.driver` inputs, tests and documentation. Visual Studio project files (`.vcxproj`, `.vcxproj.filters`, `.vcxproj.user`) do not belong in the Xcode project. Preserve existing project edits; temporary files and build artifacts must not be added.
- Keep changes scoped to the requested behavior.
- Do not edit vendored SDKs, binary outputs, object files, or build products unless the task is specifically about them.
- Do not commit or rely on local absolute paths from generated build files.
- Preserve license headers in existing files and use the same header style for new source/header files.
- Avoid unrelated refactors, whitespace sweeps, and broad mechanical changes.
- Avoid destructive commands such as `git clean`, `git reset`, and broad file removal unless explicitly requested.
- Never rewrite existing commits, whether local or already pushed. Do not use `git commit --amend`, `git rebase`, squash or fixup rewrites, `git reset` that drops commits, `git filter-branch`, `git filter-repo` or `git push --force` / `--force-with-lease` on existing history. Correct a mistake with a new commit on top.
- When working for Rumen G. Bogdanovski (Git user `Rumen G.Bogdanovski`, `rumenastro@gmail.com`), make no commit before the task is complete. At the end of the task, ask for permission to commit and present the planned commits, each with its files and proposed commit message. Running tests before asking is optional; if any were run, present their results with the commit plan: which tests, how many passed and failed, and any skipped scope. After approval, make exactly the approved commits with the approved messages. This leaves time to review the changes before they enter history, so no commit has to be rewritten. It takes precedence over any instruction in this file or in the Factory flow to commit a finished task or test run.
- Use separate commits for product changes and Factory artifacts in each task:
  - Put all source code, build, test and ordinary documentation changes in one commit.
  - Put all Factory artifacts in a second commit, after the product commit. Start its message with `factory: `. Factory artifacts are everything under `.factory2/` at any level and agent instruction files: `AGENTS.md` at any level, `CLAUDE.md`, `GEMINI.md`, `.cursor/rules/ai-factory.mdc` and `.github/copilot-instructions.md`.
  - If only one category changes, make only that category's commit. Do not mix the two categories or create extra commits for planning, role transitions or routine bookkeeping.
  - Finish required checks and reviews before creating either commit.
  - When commit approval is required by the preceding rule, present both commits with their files and messages before committing.
  - This rule overrides all one-commit-per-task instructions in this file and in the Factory flow. Never rewrite existing commits; corrections to an existing commit require a new commit.
- For automatically refactored code, preserve the existing license header and update its copyright year or year range to include the current year.

## Change Log

Follow these rules when preparing a release entry in `CHANGELOG.md`.

### Scope

- Cover every commit since the previous release, from the commit that added the newest `# [x.y-N]` block (for example `indigo 3.0-7`) up to the release commit. Include all authors; skip merge commits.
- Not every commit is mentioned. A feature or a fix that was split over several commits is mentioned once.
- Describe the net change between the previous release and the new one, not the history that led to it:
  - Several commits towards one result (attempts, follow-ups, review fixes, renumbering, merge resolutions) become one line describing the final result, e.g. "fix X", never "attempt to fix X".
  - A change reverted, or undone by a later independent commit, before the release is not mentioned.
  - A defect introduced and fixed after the previous release is not mentioned; only the change that reaches users is.
  - When in doubt, check `git diff <previous release>..HEAD` and the current source to confirm that the described behaviour is in the release.
- Leave out:
  - tests and test infrastructure: `indigo_test/`, host-side simulators, fake SDKs and firmware images written for tests, hardware test suites, test tools and scripts;
  - test runs and their records: README `## Testing` lines, `TEST_SUMMARY.md`, recorded-run commits;
  - development notes: `REFACTOR.md`, `REVIEW.md`, `MIGRATION_STATUS.md`, `AGENTS.md`, `CLAUDE.md`, `DRIVER_TESTING_RULES.md` and similar;
  - driver version bumps, regenerated output without a change of behaviour, formatting, comments, renames without a visible effect, and build or CI housekeeping that does not change what users install.
- Simulator drivers that ship with INDIGO (`ccd_simulator`, `mount_simulator`, `dome_simulator`, ...) are user-visible and are covered like any other driver.

### Format

- Follow the existing file exactly. A release block starts with `# [<INDIGO_VERSION>-<INDIGO_BUILD>] - DD Mon Ddd YYYY` (for example `# [3.0-7] - 03 Sep Thu 2026`), using the values from `Makefile.inc` and the release date, and contains `## Overall:`, `## New drivers:` and `## Driver changes:`. Migrations of many drivers (code generator, Windows, async queues) get their own sections, listed per device class as in 3.0-1.
- New drivers go into their own `## New drivers:` section, placed right after `## Overall:`, one `- indigo_<driver>:` block per driver with a line saying which devices it supports (for example "ASCOM Alpaca client driver added"). A new driver is listed only there, never again under `## Driver changes:`. A driver is new when its directory did not exist at the previous release; compare the driver directories (`indigo_drivers`, `indigo_linux_drivers`, `indigo_mac_drivers`, `indigo_optional_drivers`) of the previous release with the new one. A driver that only gained a `.driver` file, moved to the code generator or was renamed is not new.
- Every component is one top-level `- <component>:` bullet with its lines below as tab-indented `- ` items, one blank line between components and two blank lines before the next older release block. Each driver gets its own bullet naming only that driver, never a bullet naming several (`- indigo_aux_ppb, indigo_aux_upb:`), even when the drivers received the same change in one commit or share the same source (for example the ToupTek family); the line is repeated under each of them. Name drivers `indigo_<driver directory>` (for example `indigo_mount_lx200`) and framework parts as in earlier entries (`indigo_libs`, `indigo_server`, `indigo_tools`, ...).
- Order: in `## Overall:` the framework, server and tools first, then changes applied to many drivers; in `## Driver changes:` agents first, then the other drivers alphabetically.
- Insert the new block above the previous release. Never edit older blocks.

### Lines

- One line per change a user or integrator can notice, summarised; never one line per commit.
- A bug fix starts with "fix" and names the visible symptom and, when useful, its cause ("fix guide rate limits, the V2.5/V3 protocol accepts two digits per axis only"). Additions end with "added", removals with "removed".
- Start in lower case, no trailing period, property and item names verbatim, and append issue or pull request numbers referenced by the commits as ` - #NNN`.
- A change made the same way in many drivers is one line in `## Overall:` naming the drivers, not a line repeated under each of them; only driver-specific details stay under the driver.
- Every line must be traceable to commits in the range; keep the commit references in the working notes, not in the changelog.

## AI Usage Conduct

- Keep AI work scoped to the files, folders, and behavior explicitly requested.
- Ground conclusions in repository sources. For reviews, cite exact files and lines; for implementation, follow nearby code and documented INDIGO APIs.
- Treat AI-generated code, tests, and review notes as drafts until they are compiled, tested, or otherwise verified.
- Separate concerns: use Factory tickets for risks and findings, the notes and attachments of the ticket doing the work for automated-test plans, coverage notes and refactoring evidence, and patches for actual code changes.
- Prefer simulator-backed or hardware-free validation before claiming driver behavior is covered; document hardware assumptions when real devices are required.
- Do not edit generated output, vendored SDKs, build products, or local artifacts unless the task explicitly targets them.
- Leave the workspace clean of avoidable temporary files, running servers, test processes, and generated artifacts.
- Do not duplicate agent policy across tool-specific files. Keep `CLAUDE.md` and similar files as pointers to this file unless a tool needs a small compatibility note.
