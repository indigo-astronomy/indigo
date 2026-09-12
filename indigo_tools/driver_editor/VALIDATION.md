# Driver editor validation

Validated on 2026-09-10. All writes used temporary driver copies. The existing generator and the original `.driver` / generated C files were not modified.

## Automated checks

Command: `python3 -B -m unittest discover -s indigo_tools/driver_editor -v`.

Result: **26 tests passed**, including real generator execution and temporary localhost HTTP servers. The suite takes approximately three seconds on the tested host. A restricted sandbox initially prevented socket binding; the HTTP tests passed when run with loopback access.

| Area | Evidence |
|---|---|
| Existing format coverage | All 60 `.driver` files in `indigo_drivers` and `template.driver` parse; stable unique node identities and lossless no-op edits are checked. |
| Lexical correctness | Nested blocks and expressions, braces in strings/comments, escapes, continued line comments, Unicode, CRLF, and invalid-input positions. |
| Attribute catalog | Every template attribute is available, plus attributes accepted by the newer generator parser. |
| Editing | Attribute and code-body edits preserve surrounding source; unknown structural content is retained; invalid edits leave working state unchanged. |
| Save behavior | Changes remain in memory until saved; reopening reads the saved definition; stale revisions, external changes, and failed atomic replacement are checked. |
| Generation | All 60 real definitions generate successfully in isolation; preview output matches invocation from a normal driver working directory; all expected outputs are collected. |
| Generator failures | Missing executable, nonzero exit, missing outputs, mocked timeout, and actual timed-out/reaped process with temporary workspace cleanup. |
| Revision handling | Edits during generation invalidate the result; failed generation retains the old preview and successful retry clears the stale state. |
| Output publication | Saved/current preview requirement, successful three-file publication, destination conflict, rollback after a partial replacement, and successful retry. |
| Property mapping | Macros, private fields, allocation failure branches, items, handlers, dispatch, define/delete, persistence, release, and the existing detach-marker naming anomaly. |
| Scope and mapping ambiguity | Same property names in different devices, exclusion of unrelated handwritten references, serial pattern indices, parent unions, nodes with no output, and repeated block warnings. |
| Mapping corpus | Every declared property in all 60 generated drivers has recognized ranges; all ranges fit the actual C file. This checks coverage and bounds, not unique provenance for every generated line. |
| HTTP boundary | Static allowlist, path traversal rejection, token and Host checks, Origin checks for mutations, revision conflicts, malformed payloads, and successful save. |

## Browser workflow

Verified in the Codex in-app browser on macOS at its default 1280 × 720 viewport:

- Temporary Lacerta focuser definition: all 573 generated C lines remain rendered when selecting `FOCUSER_POSITION`; multiple separated ranges receive the background highlight.
- Changing `preserve_values` through the boolean control and applying edits marks the definition unsaved, retains the C text, and clears stale highlighting.
- Saving, regenerating, writing all three generated files, and searching the full source complete successfully.
- An invalid version expression causes a visible generator error while retaining the prior 573-line preview. Correcting it restores a current preview.
- Editing the handwritten `on_change` body, generating, saving, and publishing retains the inserted test comment in the full 575-line generated result.
- Temporary PlayerOne CCD definition: the complete 2,342-line C output is rendered; custom properties map to multiple ranges; `X_PRESETS` exposes all four item forms. Keyboard adjustment of both splitters changes panel dimensions while keeping the source scrollable.

No production driver file was saved through the browser. The displayed test comment and changed values existed only in temporary copies.

## WebGUI styling follow-up

The editor serves Bootstrap 5.3.3 CSS/JavaScript, `indigo.css`, and `script.png` directly from `indigo_server/resource`. Both HTTP tests passed after adding byte-for-byte checks against these shared assets. JavaScript syntax validation passed with `node --check`.

Browser verification at 1280 × 720 confirmed the shared gray background, light cards, Bootstrap controls, and the complete 2,342-line PlayerOne preview with ten highlighted ranges for `X_PRESETS`. At 800 × 720, the Bootstrap navbar toggle exposes the editor actions and all three panels remain usable. No browser warnings or errors were reported. No driver edits were applied, saved, or published during this styling check.

## Platform limits

| Platform / runtime | Status |
|---|---|
| macOS, Python 3.9.6, existing native generator | Automated suite passed. |
| Codex in-app browser on macOS | Workflow and visual checks above passed. |
| Linux | Implementation uses portable standard-library APIs; execution not verified on a Linux host. |
| Windows | Implementation accepts a native `.exe` generator and uses portable process/path APIs; execution not verified on a Windows host. |
| Standalone Safari, Chrome, Firefox, Edge | Not separately verified. |
| Hardware / SDK execution and compilation of edited drivers | Outside this editor test scope; text generation does not establish hardware correctness. |

## Known limitations

Mapping uses markers and structural associations without any generator changes. Shared scaffold ownership and arbitrary custom handle expressions may remain ambiguous. Duplicate handwritten blocks merged by the generator share a range. Unknown or future DSL constructs are preserved but may need source editing and can be rejected by the generator. The application is a local single-definition tool; it has no project-wide file browser, native GUI wrapper, C language server, or multi-user collaboration.

## Attribute row follow-up

Verified vertical label/value/type rows in the browser using the PlayerOne definition. String controls omit enclosing quotes; applying a value containing newly typed quotes produces a correctly escaped string in the in-memory node source. No file was saved. JavaScript syntax and focused conversion checks passed for empty strings, quoted text, backslashes, C escape sequences, booleans, integers, decimal numbers, and expressions. Concatenated strings and macro expressions retain expression editing.

## Code indentation follow-up

JavaScript syntax and focused conversion checks passed for common tab/space indentation, nested relative indentation, inline blocks, blank blocks, mixed indentation, CRLF, unchanged round trips, edited code, and redisplaying pending edits. The conversion applies only to code block controls; raw node source remains unchanged.

Code block boundary trimming checks passed for leading/trailing empty and whitespace-only lines, preserved internal blank lines, empty and inline blocks, LF/CRLF, unchanged round trips, edited content, and redisplaying pending edits. Source boundary whitespace is restored when applying edits.

Node source controls now use the same boundary trimming, with indentation reduction disabled. Focused checks passed for LF/CRLF, preserved source indentation and internal blank lines, unchanged round trips, edits, and pending-edit display. JavaScript syntax validation passed.

Node source indentation follow-up: outer indentation is now removed from display, accounting for parser ranges starting at the node keyword rather than its line indentation. Focused checks passed for nested nodes, tabs/spaces, LF/CRLF, inner blank lines, shorthand declarations, unchanged round trips, edits, and pending-edit display. JavaScript syntax validation passed.

## Filtered attribute catalog follow-up

Updated template attributes, accepted-type annotations, and missing property/SDK blocks from the unchanged generator parser. All 27 tests passed, including per-context catalog/type comparison and the 60-driver generation corpus. JavaScript syntax passed. Browser checks confirmed that the driver offered only missing `supported_architecture`, added it as a quoted string in memory, and hid the empty add section afterward. An inherited property omitted non-inherited-only attributes and switched to a boolean selector for `persistent`. No production file was saved.

## Attribute deletion

All 28 tests passed, including removal and re-addition with LF/CRLF, inline declarations, preserved neighboring comments, and rejection of missing attributes. Browser verification removed PlayerOne version, confirmed it returned to Add an attribute, re-added it, and confirmed it disappeared from that selector. No production file was saved.

Available attribute list follow-up: JavaScript syntax and focused DOM-stub checks passed for rendering all missing attributes as separate name/value/type/Add rows, boolean controls, and independent string/integer Add actions. The inspector help now explains that Write generated files replaces the generated .c, .h and _main.c files beside the saved definition using the current preview.

Code textarea sizing: verified in the browser that selecting a code node uses the available inspector height, expanding from 110 px to approximately 130 px after keyboard splitter adjustment. The node source summary remains visible with the normal bottom padding. JavaScript syntax validation passed.

## Optional startup file and repository picker

All 29 tests passed. New HTTP checks cover empty-session state, accepted directory families, excluded directories and escaping symlinks, invalid paths, opening, stale document identifiers, dirty-definition protection, failed-open retention, explicit discard, and unchanged disk files. Browser checks covered no-file startup, search, opening SynScan and generating its 2,565-line preview, then switching to PlayerOne with the old preview cleared. No browser errors were reported; no driver files were saved.

## Structural editing and identifiers

Browser verification added a wheel device, a number property and an item to the in-memory PlayerOne definition, displayed the item in the tree, renamed both identifiers while preserving selection, generated the new property/item macros, and removed the property. No production file was saved; no browser warnings or errors were reported. Automated checks cover LF/CRLF, shorthand expansion, adjacent source preservation, duplicate and invalid names, unsupported contexts, inherited-item rejection, stale revisions, subtree removal, descendant ID remapping, preview invalidation, and unchanged disk input.

The complete 31-test suite passed after correcting spacing in shorthand expansion; JavaScript syntax validation also passed.

## Code block toolbar and tree-only editing

All 33 tests passed. Added tests compare template code block kinds with generator contexts and cover add/edit/generate/remove at root, device and property levels, duplicate and invalid-context rejection, and unchanged disk input. Browser checks confirmed no embedded code editors in root/property details, a root dialog offering only missing on_shutdown, selection of its new tree node, Remove code block availability, editing and generated output. No production file was saved.

## C++ generated output

All 35 tests passed. The editor tries `.c` and then `.cpp` in the isolated
generator workspace, displays/maps the implementation actually produced and
tracks both destination extensions for external-change protection. The new
regression covers C++ preview, publishing its header/main outputs, conflict
rejection and preservation of an existing C file. The real-driver corpus test
also passes for the QHY C++ definition. No generator or driver files were
changed by this editor update.
