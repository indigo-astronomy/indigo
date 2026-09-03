# Repository Guidelines

## Scope

- These instructions apply to the whole INDIGO repository. More specific instructions may exist in subdirectories. 
- For automated tests under `indigo_test/`, read `indigo_test/AGENTS.md` and follow it in addition to this file.

## Developer References

Read the relevant documentation before changing behavior:

- `README.md` covers platform requirements and top-level build commands.
- `TESTING.md` documents manual and hardware-oriented validation.
- `indigo_docs/DEVELOPMENT.md` introduces the bus, device, client, and property model.
- `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` is the main reference for driver lifecycle, property semantics, INDIGO 3.0 APIs, portable I/O, and handler queues.
- `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` documents generated-driver migration.
- `indigo_docs/MAKEFILES.md` explains the makefile layers.
- `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` documents the host-side serial simulator contract, ready-file convention, and refactored simulator inventory.
- `indigo_drivers/*/README.md` files document driver-specific hardware, prerequisites, connection details, limitations, and operational notes.
- `indigo_test/AGENTS.md` documents automated-test layout, harness conventions, simulator integration rules, and test cleanup.
- `REVIEW.md` indexes incremental automatic code review state and links to folder-level review files.

## Properties reference

The properties reference is maintained in `indigo_docs/PROPERTIES.md`. Keep this file up to date when changing source code. Source files for documented property sections are listed in `indigo_docs/PROPERTIES.md` at the end of each section; use those per-section source notes as the authoritative mapping.

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
- Always use braces for `if`, `for`, `while`, and `do` bodies.
- Surround operators with spaces.
- Keep preprocessor defines aligned with tabs where the surrounding file does that.
- Do not reformat unrelated code or churn generated files.

## C and C++ Conventions

- Prefer C-compatible and portable APIs; the project builds across Linux, macOS, and Windows.
- Use `static` for file-local functions and globals.
- Use INDIGO allocation, copy, logging, timer, async, and property helper APIs where existing code does.
- Validate callback inputs with `assert()` consistently with surrounding drivers.
- Avoid forceful process exits, blocking driver entry points, or long-running work on bus callbacks.
- Use `bool` for boolean values when the surrounding file does; some legacy code uses integer or bit storage.
- Keep comments useful and specific. Preserve copyright/license and version-history header style in new source files.

## Generated Drivers

Some drivers are generated from `.driver` files by `indigo_generator`.

- If a `.driver` source exists for a generated `.c` file, edit the generator input rather than hand-editing generated output unless the task explicitly requires otherwise.
- Keep generated output and checked-in generated files synchronized when the repository convention expects both.
- Review `indigo_docs/DRIVER_GENERATOR_MIGRATION.md` before migrating or creating generated drivers.
- Treat the `.driver` file as the source of truth once migration starts. Check in the `.driver` file together with the regenerated `.c`, `.h`, and `_main.c` files, and add the `.driver` file to project files such as Xcode groups when relevant.
- Keep custom code in generator-owned blocks: `include`, `define`, `data`, shared `code`, `<device>.code`, `<device>.on_attach`, `<device>.on_connect`, `<device>.on_disconnect`, `<device>.on_timer`, and property `on_change` blocks. Do not preserve old hand-written boilerplate just because it existed in the source driver.
- Name low-level connection helpers exactly `<driver_name>_open(indigo_device *device)` and `<driver_name>_close(indigo_device *device)`. The generator wires these into generated connection handlers and owns open/close reference counting for multi-device drivers through `PRIVATE_DATA->count`.
- In generated `on_connect` blocks, use the generator-provided `connection_result` variable. Do not introduce a separate `ok` variable for the connection result, and do not `return` from `on_connect` or `on_disconnect` blocks because that can skip generator-owned cleanup, reference counting, property definition/deletion, messages, and final connection property updates.
- Let generated property handlers own their standard prologue and epilogue. The generator may set `PROPERTY->state = INDIGO_OK_STATE` before `on_change`, appends `indigo_update_property()` for most properties, and appends `indigo_update_coordinates()` for `MOUNT_EQUATORIAL_COORDINATES`. Inside `on_change`, set `INDIGO_ALERT_STATE` only on failure where possible instead of writing `PROPERTY->state = ok ? INDIGO_OK_STATE : INDIGO_ALERT_STATE`.
- Use an empty `on_change { }` block for any writable property that only needs to accept new values. The generator treats the empty block as a simple change branch: it copies values or targets, sets the property state to `INDIGO_OK_STATE`, and updates the property without scheduling a handler. Omit `on_change` entirely for inherited properties that should not get a generated change branch.
- Be aware of generator mount exceptions. For `MOUNT_EQUATORIAL_COORDINATES`, `MOUNT_MOTION_RA`, `MOUNT_MOTION_DEC`, and `MOUNT_TRACKING`, the generator inserts a parked-mount guard before user `on_change` code. Do not duplicate that guard in the `.driver` source.
- Use explicit property updates inside `on_change` only for early returns, custom messages, or deliberately self-managed asynchronous behavior. If an `on_change` block contains `_finalizer`, the generator suppresses its final `indigo_update_property()`; use that pattern only when the handler publishes its own busy/start update and delayed completion update.
- Generated change dispatch may appear to call `indigo_execute_handler(device, handler)` on a slave logical device. INDIGO's handler queue implementation internally queues work on `device->master_device` when present while invoking the callback with the original logical device, so do not add custom generator dispatch solely to force the master queue. In handlers reached from a slave device's `change_property`, treat the `device` argument as that slave logical device and use it directly for that device's property macros, updates and shared-handle helper calls. Introduce another logical-device variable only when the handler truly needs that other device's property context.
- For inherited properties, prefer generator attributes such as `hidden`, `persistent`, `asynchronous_change`, and `preserve_values` over hand-written attach/change boilerplate. Remember that `MOUNT_EQUATORIAL_COORDINATES` is finalized by the generator with `indigo_update_coordinates()`.
- Generated public headers expose the driver entry point and normally do not keep private device-name macros from the old hand-written driver. Update tests to use generated device names or public APIs instead of relying on removed private macros.
- After editing `.driver`, run `indigo_generator`, build the driver, and run the narrowest available simulator or integration test. Also inspect `git diff` afterward: large generated diffs are expected, but verify that every behavioral difference is explained by generator semantics.

## Repository Hygiene

- Keep changes scoped to the requested behavior.
- Do not edit vendored SDKs, binary outputs, object files, or build products unless the task is specifically about them.
- Do not commit or rely on local absolute paths from generated build files.
- Preserve license headers in existing files and use the same header style for new source/header files.
- Avoid unrelated refactors, whitespace sweeps, and broad mechanical changes.
- Avoid destructive commands such as `git clean`, `git reset`, and broad file removal unless explicitly requested.
- For automatically refactored code, preserve the existing license header, update its copyright year or year range to include the current year, and append a notice after the license header stating which agent refactored it.

## AI Usage Conduct

- Keep AI work scoped to the files, folders, and behavior explicitly requested.
- Ground conclusions in repository sources. For reviews, cite exact files and lines; for implementation, follow nearby code and documented INDIGO APIs.
- Treat AI-generated code, tests, and review notes as drafts until they are compiled, tested, or otherwise verified.
- Separate concerns: use `REVIEW.md` files for risks and findings, `indigo_test/CHANGES.md` for automated-test plans and coverage notes, and patches for actual code changes.
- Prefer simulator-backed or hardware-free validation before claiming driver behavior is covered; document hardware assumptions when real devices are required.
- Do not edit generated output, vendored SDKs, build products, or local artifacts unless the task explicitly targets them.
- Leave the workspace clean of avoidable temporary files, running servers, test processes, and generated artifacts.
- Do not duplicate agent policy across tool-specific files. Keep `CLAUDE.md` and similar files as pointers to this file unless a tool needs a small compatibility note.

## Incremental Code Review Notes

- Use `REVIEW.md` as the top-level index for automatic review status.
- Only perform automatic incremental reviews for folders explicitly listed in top-level `REVIEW.md`; add an index row and folder-level `REVIEW.md` before reviewing any other folder.
- Folder-level `REVIEW.md` files record the last reviewed commit, current findings, review focus, and reviewed ranges for that subtree.
- When asked for a review, first identify the affected folder review file and read its recorded `Last reviewed commit`.
- Review incrementally with `git diff <last-reviewed-commit>..<target-commit> -- <folder>` unless the user asks for a different range.
- Record findings with stable IDs, severity, file reference, summary, and status in the folder-level review file.
- Advance a folder's `Last reviewed commit` only after that folder has been reviewed through the target commit.
- After advancing a folder-level file, update the matching row in top-level `REVIEW.md`.
- Do not mark a commit reviewed if the review was partial, skipped changed generated files, or depends on unresolved assumptions.
- Keep review notes separate from test plans: `REVIEW.md` files track risks/findings, while `indigo_test/CHANGES.md` tracks automated-test coverage and deferred test work.
