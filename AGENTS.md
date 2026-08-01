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
- `indigo_docs/PROPERTIES.md` and `indigo_libs/indigo/indigo_names.h` define standard property names.
- `indigo_drivers/*/README.md` files document driver-specific hardware, prerequisites, connection details, limitations, and operational notes.
- `indigo_test/AGENTS.md` documents automated-test layout, harness conventions, simulator integration rules, and test cleanup.

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

## Repository Hygiene

- Keep changes scoped to the requested behavior.
- Do not edit vendored SDKs, binary outputs, object files, or build products unless the task is specifically about them.
- Do not commit or rely on local absolute paths from generated build files.
- Preserve license headers in existing files and use the same header style for new source/header files.
- Avoid unrelated refactors, whitespace sweeps, and broad mechanical changes.
- Avoid destructive commands such as `git clean`, `git reset`, and broad file removal unless explicitly requested.
