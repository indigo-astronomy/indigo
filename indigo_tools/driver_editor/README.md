# INDIGO driver editor

A local, three-panel editor for `.driver` definitions and the C source produced by the existing INDIGO generator. The backend uses only the Python standard library; the frontend uses HTML and JavaScript with the same bundled Bootstrap 5.3.3 and `indigo.css` as the INDIGO Server WebGUI. There are no pip/npm dependencies or build steps for the editor.

## Requirements and startup

- Python 3.9 or newer.
- An INDIGO checkout containing `indigo_tools/template.driver`, the base driver sources in `indigo_libs`, and the shared WebGUI assets in `indigo_server/resource`.
- An existing `indigo_generator` executable built for the host platform.
- A modern desktop browser with JavaScript enabled.

From the repository root, start without a file and use **Open driver**:

```sh
python3 indigo_tools/driver_editor/driver_editor.py
```

Or open a file directly:

```sh
python3 indigo_tools/driver_editor/driver_editor.py indigo_drivers/wheel_manual/indigo_wheel_manual.driver
```

On Windows, for example:

```powershell
py -3 indigo_tools\driver_editor\driver_editor.py indigo_drivers\wheel_manual\indigo_wheel_manual.driver --generator C:\path\to\indigo_generator.exe
```

The editor serves Bootstrap CSS/JavaScript, `indigo.css`, and the script icon directly from the checkout; no CDN, asset download, or separate frontend installation is needed. Its navbar, cards, controls, and state colors match the server WebGUI. Editor actions remain visible and wrap on narrow windows.

The script prints a local URL, opens it in the default browser, and stays running until Ctrl+C. It listens only on `127.0.0.1` and chooses an available port. Open the full printed URL, including its session fragment, when switching browsers. Closing a browser tab does not stop the Python process.

The generator is discovered in `build/bin/indigo_generator`, `build/bin/indigo_generator.exe`, or PATH. Use `--generator` to override it. Building the generator remains part of the normal INDIGO build described in the repository README; this editor does not modify or rebuild it.

```sh
python3 indigo_tools/driver_editor/driver_editor.py /path/to/example.driver \
  --generator /path/to/indigo_generator \
  --repository /path/to/indigo \
  --no-browser
```

Other options are `--port PORT` (default: automatic) and `--timeout SECONDS` (default: 30, maximum: 300). Use `--help` for the full command line. One definition is active at a time. **Open driver** opens a searchable dialog listing all `.driver` files recursively under `indigo_drivers` and root directories matching `indigo_*_drivers` in the configured repository. Select a path to switch definitions. Unsaved form or applied changes require explicit confirmation before discarding; failed opens retain the current definition. Switching clears the previous preview and does not write files.

## Working with the editor

1. Select a driver, transport, device, property, or code block in the left tree. The tree can be filtered; all branches remain expanded. Items are selectable in the tree and also appear in their property’s detail panel.
2. Edit attributes in the upper-right panel. Each attribute occupies one row with its label, value, and type. String literals omit their enclosing quotes; the editor restores them when applying edits. C escape sequences remain editable as written. Other values retain their original C expressions. Literal booleans have a selector. Hover over fields for help derived from `template.driver`.
3. All allowed attributes are always visible in one list. Existing attributes have a × button; missing attributes from `template.driver` have a + button. Each row shows its name, value, and type. Boolean values have a selector; string values omit enclosing quotes. C expressions retain DSL syntax. Adding or deleting an attribute applies pending edits to the working definition; Save definition writes these changes to disk. New custom driver constants require editing the .driver file externally.
4. Code block editors are always visible in the selected node details. The inspector toolbar provides Add device at the root, Remove device and Add property on devices, Remove property and Add item on custom properties, and Remove item on items. Inherited properties cannot contain items. Dialogs select the device/property type and identifier; custom properties require the X_ prefix. Property and item identifiers are editable in the Identifier row, separately from the name attribute. Removing a node also removes its children after confirmation. Structural changes apply pending edits and remain in memory until saved. Renaming changes the declaration identifier; handwritten C references and explicit handle/name overrides are not rewritten. Creating arbitrary code blocks still requires external editing.
5. **Apply edits** applies pending form edits across nodes to the in-memory definition. It preserves unrelated source text. **Save definition** applies pending edits and writes only the `.driver` file.
6. **Generate preview** applies pending edits and invokes the unchanged generator in a temporary workspace. It works with unsaved changes and does not write generated files into the repository.
7. **Write generated files** writes the successful preview's `.c`, `.h`, and `_main.c` outputs beside the saved definition. This action is enabled only after the current definition is saved and has a current successful preview.

Pending form edits survive selection changes. **Discard pending edits** discards form drafts only; previously applied in-memory edits remain. The browser warns before closing a page with pending or unsaved changes. In-memory changes last only as long as the Python process.

The lower-right panel always retains the complete generated C file. Selecting a node highlights its associated sections with a colored background and scrolls to the first section. Arrow buttons navigate between sections; they never regenerate or filter the file. Parent nodes include their descendants' sections. Search is case-insensitive and finds matching lines; Enter or the adjacent button advances to the next matching line. Splitters support dragging and arrow keys when focused.

## Preview accuracy and mapping limits

The generator remains the authority on DSL semantics and C emission. The editor's parser preserves text and checks structural syntax; it does not compile the handwritten C or validate hardware behavior. Generate and then build/test the driver normally before relying on a behavioral change.

Mapping combines existing handwritten-block markers with structural recognition of generated property handlers, declarations, item initialization, lifecycle operations, persistence, dispatch, device code, serial patterns, and hot-plug scaffolding. It is not an exact compiler source map. Shared boilerplate or unusual custom expressions may lack unique ownership. Repeated code blocks merged by the generator share a highlighted range. The mapping footer describes the method, and its tooltip reports detected marker ambiguities. A node without recognized output shows an explicit message while retaining the whole file.

Any applied edit makes the previous preview outdated. Its C text remains available, but node highlighting is disabled until successful regeneration. A failed generation retains the previous preview and displays diagnostics. Results from a revision superseded during generation are discarded. The generator's relative base-source reads are reproduced in the temporary workspace; no SDK or attached hardware is required for text generation.

## Files and recovery

Saving checks whether the definition has changed externally. Publishing checks the generated output files too; conflicts stop the operation. The definition is replaced atomically. Generated outputs are staged before replacement; each replacement is atomic, but a three-file update is not one filesystem transaction. If a replacement fails, the editor attempts to restore already replaced outputs and reports any incomplete rollback.

For an external conflict, copy the working definition from the root node's source editor before stopping the process. Reconcile it with the external changes, then restart the editor. Refreshing the browser reloads the current server state, not a newly edited disk file. Unapplied drafts in a page are discarded only if you accept its navigation warning.

Output filenames follow the driver identifier and first device type. Renaming those can produce new filenames; obsolete outputs are not removed automatically. Symbolic-link output destinations are rejected. Opening a definition through a symbolic link resolves its target at startup.

## Verification

Run the hardware-free tests from the repository root:

```sh
python3 -B -m unittest discover -s indigo_tools/driver_editor -v
```

The tests use temporary copies and temporary localhost servers. They read `.driver` files under `indigo_drivers`, but do not edit them or their checked-in generated outputs. Generator-dependent tests are explicitly skipped if the executable is missing. The tests require permission to bind a loopback socket; restricted sandboxes may need to allow this.

See `DESIGN.md` for the format/API contract, `VALIDATION.md` for the tested scenarios and platform limits, and `AGENTS.md` for the completed implementation checklist.

Code block editors remove the common leading indentation from nonblank lines for display and restore it when applying edits. Leading and trailing whitespace-only lines are hidden in code block editors. Internal blank lines and relative indentation remain visible. Applying edits restores the original block boundaries and line-ending style.

When a code block is selected in the tree, its textarea fills the available inspector height and follows splitter resizing, leaving room for the fixed help footer. Manual textarea resizing remains available.
