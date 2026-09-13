# INDIGO Driver Editor

A local browser editor for INDIGO `.driver` definitions. It lets you inspect the generated C or C++ preview, edit supported driver-definition nodes, save the `.driver` file and write generated output files.

The editor runs locally. The backend uses only Python's standard library, and the frontend uses the Bootstrap and INDIGO WebGUI assets bundled with the checkout. There are no pip or npm dependencies.

## Requirements

- Python 3.9 or newer.
- An INDIGO checkout containing `indigo_tools/template.driver`, `indigo_libs` and `indigo_server/resource`.
- A built `indigo_generator` executable for the host platform.
- A desktop browser with JavaScript enabled.

## Starting

From the repository root, start without an initial file and choose one with Open driver:

```sh
python3 indigo_tools/driver_editor/driver_editor.py
```

Or open a definition directly:

```sh
python3 indigo_tools/driver_editor/driver_editor.py indigo_drivers/wheel_manual/indigo_wheel_manual.driver
```

On Windows:

```powershell
py -3 indigo_tools\driver_editor\driver_editor.py indigo_drivers\wheel_manual\indigo_wheel_manual.driver --generator C:\path\to\indigo_generator.exe
```

The script prints a local URL, opens it in the default browser and keeps running until Ctrl+C. It listens only on `127.0.0.1` and chooses an available port by default. If you switch browsers, open the full printed URL including its session fragment.

The generator is discovered in `build/bin/indigo_generator`, `build/bin/indigo_generator.exe` or `PATH`. Use `--generator` to override it:

```sh
python3 indigo_tools/driver_editor/driver_editor.py /path/to/example.driver \
  --generator /path/to/indigo_generator \
  --repository /path/to/indigo \
  --no-browser
```

Other options include `--port PORT` and `--timeout SECONDS`. Use `--help` for the full command line.

## Editing

Select a driver, transport, device, property, item, serial pattern or code block in the left tree. The upper-right inspector shows settings for the selected node. The lower-right panel keeps the complete generated source visible.

Attributes are edited in rows with name, value and type. String values are shown without enclosing quotes; the editor restores quotes when applying edits. Boolean values use a selector. C expressions remain editable as DSL text.

All allowed attributes for the selected context remain visible. Existing attributes can be removed, and missing attributes can be added from `template.driver`.

Code block bodies are edited by selecting the code block node itself. Parent nodes show only their own attributes and actions. Leading common indentation is removed for display and restored when applying edits; internal blank lines and relative indentation are preserved.

The inspector toolbar provides context-specific actions for adding or removing devices, properties, items, code blocks and serial patterns. Inherited properties cannot contain items. Custom properties require the `X_` prefix. Identifier fields rename the declaration identifier; handwritten C references are not rewritten automatically.

## Saving and Generating

Apply edits updates the in-memory definition. Save definition writes only the `.driver` file.

Generate preview applies pending edits and runs the unchanged generator in a temporary workspace. It works with unsaved changes and does not write generated files into the repository.

Write generated files writes the current successful preview's implementation, header and main files beside the saved definition. The action is available only after the definition is saved and the preview matches it.

The editor checks for external changes before saving or writing generated outputs. Conflicts stop the operation and leave existing files intact.

## Opening Drivers

Open driver shows a searchable list of `.driver` files under `indigo_drivers` and root directories matching `indigo_*_drivers` in the configured repository. Unsaved changes require confirmation before switching. Failed opens keep the current file active.

## Preview and Mapping

The generated-source panel always shows the complete generated file. Selecting a node highlights generated ranges associated with that node and scrolls to the first range. Arrow buttons move between highlighted ranges. Search is case-insensitive.

Mapping is a best-effort editor feature based on generated markers and structural recognition. It is not an exact compiler source map. Shared boilerplate, merged code blocks or unusual custom expressions may not have unique ownership. A node without recognized output shows an explicit message while leaving the full generated source visible.

After edits, the previous preview is marked outdated and highlighting is disabled until generation succeeds again. A failed generation keeps the previous generated text visible and shows diagnostics.

## Recovery

For an external save conflict, keep the editor open, copy the working definition from the root source view if needed, reconcile it with the changed file on disk, then reload or restart the editor.

Refreshing the browser reloads the current server state, not a newly edited disk file. Unapplied page drafts are discarded only after accepting the browser warning. In-memory changes last only while the Python process is running.

Generated output filenames follow the driver identifier and first device type. Renaming those can produce new filenames; obsolete outputs are not removed automatically. Symbolic-link output destinations are rejected.
