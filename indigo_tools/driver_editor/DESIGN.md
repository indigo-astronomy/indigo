# Driver editor design

## Input and grammar audit

The authoritative inputs are `../template.driver` and the parser in `../indigo_generator.c`. The structural parser was exercised against all 60 `.driver` files under `indigo_drivers` and the template on 2026-09-10. Representative fixtures include manual wheel (minimal inherited property), Lacerta focuser (serial, request handlers), SX CCD (multiple devices), PlayerOne CCD (SDK, discovery retries, conditional attachment), Astroasis wheel (architecture restriction), and NexStar AUX mount (serial without port enumeration).

A definition has one named `driver` root, attributes of the form `identifier = expression;`, typed device blocks, named properties, named items, transport blocks, repeated serial patterns, and opaque C blocks. Shorthand declarations ending in `;` are supported. C blocks can repeat and contain nested braces. The tree stops at properties; items and property code blocks appear in the detail panel. Source offsets are Python Unicode character offsets with exclusive ends. The browser never slices using these offsets; the server sends the relevant text so astral Unicode characters do not cause JavaScript UTF-16 offset errors.

The template is the sole catalog for allowed attribute names, help, and accepted value types (`@type` comments), grouped by node and item context. The current generator also accepts `supported_architecture`, `attach_if`, `name_value`, `no_ports`, `exact_match`, `discovery_retries`, property `handle`, `handler`, `pointer`, item `handle` and `name`, property `code` and `on_change_request`, and SDK `unplug_match`. The template now includes these attributes and blocks, with attribute names and accepted types checked against the unchanged generator parser. Driver-level arbitrary constant attributes are allowed. Unknown structural content is retained, and the source editor remains available; the generator is the authority on semantic validity. Parsing does not imply valid compilable C.

## Mapping audit and constraints

The generator must not be changed. Existing `//+` and `//-` markers identify handwritten code blocks, not all generated boilerplate. Generated property macros, private-data fields, allocation, item initialization, definition/deletion, dispatch, persistence, and release are unmarked. A property detach block is currently labeled `device.PROPERTY.on_attach` by the emitter; the editor resolves it using the enclosing detach function. Repeated handwritten blocks share a marker and cannot be uniquely distinguished after the generator merges them.

The editor combines marker ranges with structural C recognition: dedicated property handlers, property and item identifiers outside handwritten blocks, device function context, initialization failure blocks, and dispatch branches. Parent selections include descendant ranges. This is an editor-side structural association, not a compiler source map. Marker ambiguity and nodes without recognized ranges are reported. Shared generated scaffolding and arbitrary custom expressions can lack unique ownership. Handwritten references to another property are not treated as that property's generated code.

No output is rewritten to introduce markers. Driver-root selection includes the entire C file. Selections never regenerate code. All ranges use one-based inclusive line numbers and refer to the exact preview revision. The UI must retain the full C file even when no ranges match.

## Generation and persistence

Generation runs in a temporary repository-shaped workspace. The unchanged generator opens `../../indigo_libs/indigo_<type>_driver.c` relative to its working directory to discover base property metadata, so copy the relevant base files into that layout. No driver SDK or hardware is needed to generate text. Output filenames come from the parsed driver name and first device type, not necessarily from the input filename. Only expected `.c`, `.h`, and `_main.c` files are accepted.

Keep three states: disk fingerprint, in-memory revision, and successful preview revision. Edits use optimistic revision checks. Save detects external changes and atomically replaces the definition. Preview uses pending edits without saving. Publish requires a successful preview of the saved revision and checks destination fingerprints before writing. Individual output replacement is atomic; multiple files cannot be replaced in a single portable filesystem transaction, so stage files first, roll back replacements on failure, and report rollback failures explicitly.

A shared reentrant lock protects edits, saving, and publishing. A separate generation lock serializes generator processes; generation snapshots the state, releases the state lock while running, and accepts the result only if the revision is still current. Failed or superseded generation cannot silently replace a current preview. A timeout terminates and reaps the generator. Temporary workspaces are removed on success and failure.

## HTTP contract

The server is loopback-only. Static assets are an explicit allowlist, including the unchanged `bootstrap.min.css`, `bootstrap.min.js`, `indigo.css`, and `script.png` served directly from `indigo_server/resource`; it never exposes a general filesystem browser. Requests must use the expected Host. JSON mutation requests require a per-session token and the server's own Origin. The token is passed to the browser in the URL fragment, removed from the address bar, and stored in sessionStorage. Read API requests also require the token. Responses disable caching and external script execution. The open endpoint accepts only a relative path present in the repository driver allowlist: indigo_drivers and root indigo_*_drivers directories. Resolved files must stay within their listed driver directory. File switching is serialized with mutations; document identifiers prevent stale requests from targeting another driver.

- `GET /api/drivers`: sorted allowed repository-relative driver paths.
- `POST /api/open`: `{file, revision, document, discard?}`. Open a listed driver, rejecting unsaved changes unless discard is explicitly true. A failed parse leaves the current editor intact.
- `GET /api/state`: file name, source, tree with node snippets, revision, saved status, preview, diagnostics, and schema.
- `POST /api/edit`: `{revision, node, changes: [{field, value, name?}]}`. Fields are `attribute`, `code`, or `source`; source replacement also permits structural edits. Return updated state. Invalid structure leaves the previous working state untouched.
- `POST /api/save`: `{revision}`. Save the working definition after an external-change check; return state.
- `POST /api/generate`: `{revision}`. Generate a preview and mapping without changing repository files; return state.
- `POST /api/publish`: `{revision}`. Write the three generated files from the saved current revision after conflict checks; return state and written filenames.

Errors return JSON with `error`; invalid input uses 400, token/origin failures 403, unknown routes 404, revision/file conflicts 409, and generation failures 422. All mutating responses and errors are surfaced in the UI. Limits apply to request bodies and generator execution time.

Without a CLI file, state contains a null tree and revision zero until a driver is opened. Responses include a document identifier, sent by the frontend with each mutation. Repository, generator, and timeout settings persist across opens.

## Structural editing

POST /api/structure accepts revision, document, node, action (add/remove), kind and name. It validates parent/child kinds, C identifiers, custom-property X_ prefixes, and case-insensitive sibling duplicates. Shorthand parents expand into blocks. Removal preserves neighboring comments and source. The returned selected ID identifies the new node or removed node’s parent. Attribute-edit requests also accept field=identifier; responses include old/new node ID mappings to preserve selection. Pending descendant edits are flushed before ancestor renames. Explicit attributes and handwritten code are preserved, not refactored after identifier changes.
