# Driver Editor Development Rules

These instructions apply to future development of the local INDIGO `.driver` editor. The parent repository `AGENTS.md` also applies.

## Scope

- Keep this tool focused on editing `.driver` definitions and inspecting generated C or C++ output from the existing `indigo_generator`.
- Do not change the generator from this subtree. If an editor feature appears to require generator changes, document the limitation and ask for explicit approval before touching generator code.
- Use Python 3 standard-library code for the backend and plain HTML, CSS and JavaScript for the frontend. Do not add required pip, npm, backend framework or external web-server dependencies.
- Keep documentation in this directory in English.

## Runtime and Security

- Serve only on `127.0.0.1`, using `http.server.ThreadingHTTPServer` or an equivalent standard-library loopback server.
- Use an automatically selected free port by default; explicit `--port` remains optional.
- Protect mutating requests with the local session token and origin/host checks.
- Restrict file operations to the opened repository context and the allowlisted `.driver` paths exposed by the editor.
- Invoke `indigo_generator` with an argument list, never through a shell. Use bounded timeouts and captured diagnostics.
- Generate previews in a temporary workspace. Opening a file, changing selection or refreshing a preview must not overwrite repository `.c`, `.h` or `_main.c` files.

## Editing Semantics

- Preserve the original `.driver` text, comments, ordering and formatting outside the exact edited range.
- Parse the DSL structurally. Regular expressions alone are not sufficient for nested blocks, strings, comments and embedded C code.
- Keep disk writes explicit. Applying edits changes only the in-memory working definition; saving writes only the `.driver` file.
- Check for external file changes before saving or writing generated outputs, and report conflicts without damaging existing files.
- Replace saved files atomically where practical. Generated implementation/header/main updates are separate file replacements, not a single transaction.
- Do not rewrite handwritten C references when renaming identifiers; make that limitation visible to users.

## User Interface

- Keep the three-panel layout: left tree, upper-right inspector and lower-right full generated output.
- The generated output panel must show the complete generated file. Highlight selected-node ranges without filtering or replacing unrelated generated code.
- Node selection alone must not trigger regeneration.
- Code block bodies are editable only by selecting their own tree node. Do not embed code block editors in parent details.
- Item settings appear only when the item is selected in the tree.
- Keep structural actions context-specific: add/remove devices, properties, items, code blocks and serial patterns only where the grammar permits them.
- Use `../template.driver` as the attribute catalog. Offer only attributes accepted in the selected node context and not already present.
- Preserve C escape sequences and string literal contents. Omit enclosing quotes in string controls and restore them when applying edits.

## Mapping and Preview

- Treat the generator as the authority for DSL semantics and output emission.
- Build output mapping from existing generated markers and structural recognition in the editor. Do not claim exact source-map provenance.
- Represent mappings with stable node identities and one-based inclusive line ranges. Identical names in different devices must remain distinct.
- A node may map to multiple disjoint generated ranges. Parent nodes include descendant ranges.
- Mark previews stale after edits. Failed generation keeps the previous generated text visible but must not present stale highlighting as current.
- Support both `.c` and `.cpp` generated implementation outputs.

## Styling

- Use the existing Bootstrap 5.3.3 CSS/JavaScript, `indigo.css` and script icon from `indigo_server/resource`.
- Match the INDIGO Server WebGUI's restrained Bootstrap styling, page background, controls and state colors.
- Do not copy or modify shared WebGUI assets.

## Tests and Evidence

- Keep editor tests hardware-free. They may use temporary `.driver` copies, temporary output directories and temporary loopback HTTP servers.
- Do not edit real driver definitions or checked-in generated outputs from tests.
- Generator-dependent tests must skip cleanly when the generator executable is unavailable.
- Record future implementation results, validation notes and remaining editor limitations in the editor's `REFACTOR.md`, not in this `AGENTS.md`.
