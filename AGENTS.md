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
- `indigo_docs/SERIAL_DEVICE_SIMULATORS.md` documents the host-side serial simulator contract, ready-file convention, and refactored simulator inventory.
- `indigo_drivers/*/README.md` files document driver-specific hardware, prerequisites, connection details, limitations, and operational notes.
- `indigo_test/AGENTS.md` documents automated-test layout, harness conventions, simulator integration rules, and test cleanup.
- `REVIEW.md` indexes incremental automatic code review state and links to folder-level review files.

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
