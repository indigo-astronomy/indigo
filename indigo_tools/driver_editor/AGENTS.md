# Driver editor — requirements and implementation plan

## Purpose and scope

Build a cross-platform local application for conveniently viewing and editing `.driver` files and inspecting the C code produced from them by the existing `indigo_generator`.

Use Python 3 with its standard library for the backend, and HTML, CSS, and JavaScript for the frontend. Use the same bundled Bootstrap and shared styles as `indigo_server/resource`, as explicitly requested by the user. Do not add backend web frameworks, an external web server, or required pip/npm packages. The application should run on Linux, macOS, and Windows with Python installed and the generator built for the respective platform.

This document records the current requirements and plan; it does not imply that these features already exist. The parent repository rules in `AGENTS.md` also apply.

Write and maintain all documentation in English.

## User interface

A single browser page with three panels and adjustable splitters:

1. **Left:** a tree of the `.driver` structure down to the property level. Include other relevant definition blocks; the exact hierarchy must follow the actual grammar.
2. **Top right:** settings for the selected node. Use appropriate form controls for attributes and a text editor for handwritten code blocks. Display attributes vertically, one row per attribute, with its label, value, and type. Omit enclosing quotes from string literal controls and restore them when applying edits; preserve C escape sequences. Display code block bodies and node source without their outer indentation and restore it when applying edits, preserving relative indentation. Hide leading and trailing whitespace-only lines in code block and node source controls while retaining the source block boundaries when applying edits.
3. **Bottom right:** the entire generated `.c` file, read-only, with syntax highlighting. Sections associated with the selected node have a distinct background. All other code remains visible and accessible through scrolling and search; do not filter the listing or replace its contents with the selected section.

A single node may correspond to multiple disjoint ranges in `.c`, such as property initialization, a change handler, and cleanup. Highlight all these ranges simultaneously. Provide navigation to the previous and next range and scrolling to the first range when selecting a node. For a parent node, include its descendants' ranges. Changing the selection alone must not trigger regeneration.

## Proposed behavior

- Launch the script with a path to a `.driver` file; allow an explicit generator path.
- Use `http.server.ThreadingHTTPServer` to serve the frontend and a small JSON API on `127.0.0.1` with an automatically assigned available port. Use `webbrowser` to open the page.
- Use `json`, `pathlib`, `subprocess`, and other standard-library modules. The frontend communicates through `fetch()`; WebSocket is unnecessary.
- Python reads and modifies `.driver` files and invokes the existing C generator. Do not reimplement C generation in Python or JavaScript.
- Preserve the original `.driver` text, comments, ordering, and formatting outside the specific edited range. The parser must distinguish the DSL, strings, comments, and embedded C code; regular expressions alone are insufficient for nested blocks.
- Distinguish the saved file, pending edits, and the revision used for generation. The preview and its mapping must always come from the same revision. On failure, mark the previous preview as outdated and show diagnostics.
- Generate previews in a temporary workspace while preserving required inputs. Opening a file or changing the selection must not overwrite existing `.c`, `.h`, or `_main.c` files; writing generated outputs is a separate user action.
- Represent mappings as a node identifier and a list of line ranges, using one-based line numbers with inclusive endpoints. Node identities must distinguish identical names in different devices.
- First investigate the existing `//+` and `//-` markers in generated code. Their completeness for mapping every part of a node has not yet been verified. Do not present an incomplete mapping as exact.
- Do not change the generator under any circumstances. This is an explicit user constraint. Implement mapping entirely in the editor using existing markers and the structure of generated C. Document ambiguity and unsupported mappings instead of changing the generator or claiming exact source-map provenance.
- Serialize writes and generation. Check for external file changes before saving. Restrict the backend to the opened working context, check the origin of mutating requests, and use a local session token. Invoke the generator with an argument list without a shell, with a timeout and captured diagnostics.

## Atomic steps

Complete the steps incrementally. Each has its own deliverable and verification; mark a step complete only after verification. Do not make changes unrelated to the editor.

- [x] **1. Investigate the input format.** Read the relevant sections of `README.md`, `indigo_docs/DRIVER_GENERATOR_MIGRATION.md`, `indigo_docs/DRIVER_DEVELOPMENT_BASICS.md`, and `indigo_tools/indigo_generator.c`. Record the grammar constructs and representative existing `.driver` files to use for parser verification.
- [x] **2. Verify mapping markers.** Compare markers in representative outputs against declarations, attachment code, handlers, and cleanup. Document coverage and specific gaps. Supplement markers with structural mapping in the editor; do not modify the generator.
- [x] **3. Define the data model and API.** Describe a tree node, attribute, input text range, C output range, revision, and diagnostic. Define requests and responses for loading, editing, saving, and generation, including errors.
- [x] **4. Create a minimal server.** Add an executable Python script, arguments, a loopback server, serving of specific static files, and browser launch. Verify startup, available port allocation, and clean shutdown.
- [x] **5. Create the interface skeleton.** Implement the three panels and adjustable splitters using sample data. Verify a long tree and a long C file without losing access to the panels.
- [x] **6. Implement `.driver` tokenization.** Preserve exact token positions and distinguish comments, strings, and nested blocks. Verify braces in comments and strings, escape sequences, and embedded C code.
- [x] **7. Implement the structural parser.** Build a tree and attributes with ranges in the original text. Verify representative existing definitions and clear errors with positions for invalid input; unsupported constructs must not cause text loss.
- [x] **8. Populate the tree with real data.** Connect file loading and node selection. Verify that identically named properties in different devices are distinguished and that selection survives data refreshes.
- [x] **9. Display node details.** Initially display attributes and blocks as read-only. Verify that values and code match the selected node and that all source content is safely displayed as text.
- [x] **10. Implement in-memory edits.** Add attribute and block editing, precise text replacement, and reparsing. Verify that surrounding comments, whitespace, and unrelated blocks remain unchanged; indicate unsaved changes.
- [x] **11. Implement `.driver` saving.** Add explicit saving with external-change detection and atomic file replacement. Verify success, conflicts, and write failures without damaging the original file.
- [x] **12. Connect the generator for previews.** Run it in a temporary workspace and return C output with its revision and diagnostics. Verify paths containing spaces, invalid input, a missing generator, timeouts, and temporary-data cleanup.
- [x] **13. Display the entire C file.** Add line numbers, basic syntax highlighting, and search without external JavaScript packages. Verify a large real output, accurate line numbering, and literal display of HTML characters.
- [x] **14. Implement mapping.** Based on step 2, build lists of ranges for nodes. Verify multiple disjoint ranges for one property, parent nodes, overlapping ranges, and a node with no output.
- [x] **15. Connect selection to highlighting.** Apply a colored background to all associated sections without removing other C code. Add navigation between sections and verify that rapid node selection changes neither alter the contents nor invoke the generator.
- [x] **16. Connect editing to preview refresh.** Add explicit preview refresh from the working definition. Discard outdated responses and retain the last successful preview with a clear status on failure. Verify successive edits during generation.
- [x] **17. Add writing of generated files.** Through a separate action, write the outputs of successful generation from the currently saved definition to their proper locations. Verify matching revisions, external changes to outputs, and reporting of write failures.
- [x] **18. Verify the complete workflow.** Open a real `.driver`, select a property, edit an attribute and code, refresh the preview, inspect highlighting, save, and reopen. Also verify generation failure and recovery. Keep tests within the editor's scope; read `indigo_test/AGENTS.md` before making changes under `indigo_test/`.
- [x] **19. Verify platforms and document usage.** Add a local `README.md` covering requirements, startup, the generator path, saving, and limitations. Record the operating systems and browsers actually verified; mark others as unverified. Inspect the diff and clean up temporary files and processes.

## Completion criteria

The user can open an existing `.driver`, edit a node through its detail panel, save the definition, and produce outputs using the existing generator. The bottom-right panel always displays the entire C file and highlights all mapped sections of the selected node with a distinct background. The editor preserves unrelated definition text, distinguishes current and outdated previews, reports errors without damaging files, and uses the repository’s Bootstrap assets without a backend web framework.

## Progress record

2026-09-10: Steps 1–19 are complete. All 60 existing definitions under `indigo_drivers` and `template.driver` parse successfully; every real definition also generates in isolation. The 26-test suite passed, including temporary localhost HTTP tests. Browser checks covered full-source highlighting, custom property/item forms, attribute and C-block editing, saving, publication, search, generator failure/recovery, and splitter resizing. A 2,342-line PlayerOne preview was verified. See `DESIGN.md`, `README.md`, and `VALIDATION.md` for the implementation contract, usage, evidence, and mapping/platform limitations. Linux, Windows, and standalone browsers remain explicitly unverified. Test tabs, servers, and temporary driver copies have been cleaned up. The generator has not been modified.

## WebGUI styling

Use the existing Bootstrap 5.3.3 CSS and JavaScript, `indigo.css`, and the script icon directly from `indigo_server/resource`. Match the WebGUI navbar, gray page background, light cards, Bootstrap controls, and INDIGO state colors. Keep the three-panel layout and editor behavior. Do not copy or modify the shared assets.

## Attribute catalog

Use `../template.driver` as the sole catalog for Add an attribute. Offer only attributes accepted in the selected node context and not already present, with a separate name/value/type/Add row for every available attribute, all listed vertically. Keep its `@type` annotations synchronized with the unchanged generator parser. Arbitrary driver constants remain available through node source.

Provide a delete button for existing attributes. Remove the declaration from the working definition and return catalog attributes to Add an attribute. Preserve neighboring code and comments, and keep disk writes explicit.

## Current UI scope

The Edit node source section has been removed at the user’s request. Do not expose raw node source editing in the UI. Earlier references to that section record previous implementation steps; structural changes and new custom constants now require external .driver editing. Code block controls fill the space below the help text.

All available attributes now remain visible alongside existing attributes; there is no collapsible Add an attribute section. Rows share the same layout and use × for existing attributes and + for missing attributes. These actions and the C preview navigation buttons use the same fixed width.

Tree branches, code block editors, and header actions remain visible. Generator diagnostics is a separate always-visible panel with an initial content height of three lines, vertical scrolling, and manual height resizing. Keep scrolling for long content and wrapping for narrow windows.

## Driver selection

The CLI file argument is optional. Provide Open driver in the toolbar with a searchable popup listing repository-relative .driver paths recursively under indigo_drivers and root indigo_*_drivers directories. Only listed paths may be opened through HTTP. Protect unsaved edits, preserve the current editor on failed opens, and clear stale preview/selection state after successful switches.

## Device, property and item editing

Provide context-specific inspector actions to add/remove devices, properties and items. Show items in the tree. Provide an Identifier field for properties/items, distinct from the name attribute. Validate grammar contexts, duplicate identifiers and the X_ custom-property prefix. Inherited properties cannot have items. Preserve selection through renames; structural edits remain in memory until explicitly saved. The earlier external-only structural editing limitation no longer applies to these supported operations.

Show item settings only when that item is selected in the tree; do not embed item forms in the parent property detail.

Code block bodies are editable only by selecting their own tree node; never embed their editors in parent details. Show all code blocks in the tree, including property blocks. Add code block on root/device/property nodes uses missing block kinds from template.driver; Remove code block appears on code nodes.

Provide Add pattern on serial nodes and Remove pattern on pattern nodes. Allow repeated pattern blocks with distinct node identities and preserve other patterns when removing one.
