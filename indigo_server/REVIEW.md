# indigo_server Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Baseline review complete; open findings recorded. |

## Scope

INDIGO server executable, build rules, Visual Studio project metadata, and checked-in web resources under `indigo_server/resource/`.

Reviewed source files:

- `Makefile`
- `indigo_server.c`
- `indigo_server.vcxproj`
- `indigo_server.vcxproj.filters`
- `resource/*.html`
- `resource/indigo.js`
- `resource/components.js`
- page-owned scripts in `resource/imager.html`, `resource/mount.html`, `resource/guider.html`, `resource/script.html`, `resource/ctrl.html`, and `resource/mng.html`

Generated build output, object files, generated `.data` resources, and bundled minified third-party libraries were excluded from deep review. The minified libraries were only scanned for obvious integration risks.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| SERVER-001 | High | `indigo_server.c:763`, `indigo_server.c:771`, `indigo_server.c:805`, `indigo_server.c:811`, `indigo_server.c:1362`, `indigo_server.c:1368`, `indigo_server.c:1374`, `indigo_server.c:1402` | RPI-management commands are assembled with client-controlled property text inside shell command strings and executed with `popen()`. SSID, password, country-code, and host-time values can contain quotes or shell metacharacters, so a remote client with access to these properties can run arbitrary shell fragments when RPI management is enabled. Replace shell-formatted `popen()` with argv-based execution, or strictly escape each argument before building the command. | Open |
| SERVER-002 | High | `indigo_server.c:505`, `indigo_server.c:1889`, `indigo_server.c:1933`, `indigo_server.c:1611`, `indigo_server.c:1612` | Command-line arguments are copied into fixed `server_argv[128]` without a bounds check, and `--bonjour`/`-b` reads `server_argv[i + 1]` without confirming an argument exists. A long invocation can write past `server_argv`, while a trailing `-b` can read outside the saved argument list during `server_main()`. Reject too many arguments while parsing `main()` and validate every option that consumes a following value. | Open |
| SERVER-003 | Medium | `indigo_server.c:1501`, `indigo_server.c:1534`, `indigo_server.c:1540`, `indigo_server.c:1551`, `indigo_server.c:1554` | Dynamic driver-list parsing stores the same driver name twice with two `strdup()` calls, leaking the first allocation, and increments `dynamic_drivers_count` even when the description field is missing. A malformed or truncated driver-list entry can leave `description == NULL` and later pass it into property item initialization. Store each parsed name once, require both fields before incrementing the count, and use `snprintf()` for `path`. | Open |
| SERVER-004 | Medium | `indigo_server.c:528`, `indigo_server.c:530`, `indigo_server.c:542`, `indigo_server.c:551`, `indigo_server.c:580`, `indigo_server.c:582`, `indigo_server.c:592`, `indigo_server.c:629`, `indigo_server.c:631`, `indigo_server.c:752` | Generated JSON resources use unchecked `malloc()`, `strcpy()`, and `sprintf()` into manually grown buffers. The code resizes only after writing each record, star names are copied into `desig[256]` without a length check, and JSON string content is emitted without escaping. Catalog data changes can crash startup or produce invalid JSON. Use checked allocation, `snprintf()` with remaining capacity before appending, bounded name copies, and JSON string escaping. | Open |
| SERVER-005 | Medium | `indigo_server.c:1852`, `indigo_server.c:1855`, `indigo_server.c:1863`, `indigo_server.c:1865`, `indigo_server.c:1866`, `indigo_server.c:1874`, `indigo_server.c:1876` | The POSIX signal handler performs non-async-signal-safe work, including logging, `waitpid()` loops, signal reconfiguration, and `indigo_server_shutdown()`. If a signal arrives while library locks or allocator state are held, shutdown can deadlock or corrupt state. Use a self-pipe/eventfd or atomic flag and perform shutdown/reap work from the main loop or a dedicated signal thread. | Open |
| SERVER-006 | Medium | `resource/ctrl.html:83`, `resource/mng.html:114`, `resource/guider.html:154`, `resource/imager.html:210`, `resource/mount.html:274`, `resource/script.html:112`, `resource/components.js:592`, `resource/components.js:597`, `resource/imager.html:254`, `resource/imager.html:257`, `resource/imager.html:272`, `resource/imager.html:275`, `resource/imager.html:283`, `resource/imager.html:286` | Web pages hard-code `ws://` and `http://` when connecting to the server and rendering BLOB/image URLs. This breaks when the control panel is served through HTTPS or a TLS reverse proxy because browsers block mixed-content WebSockets and images. Build URLs from `window.location.protocol` (`ws` vs `wss`, `http` vs `https`) and normalize relative BLOB paths with the `URL` API. | Open |
| SERVER-007 | Medium | `resource/components.js:141`, `resource/components.js:147`, `resource/components.js:157`, `resource/mount.html:66`, `resource/mount.html:67`, `resource/mount.html:626`, `resource/mount.html:632` | The sexagesimal number editor checks `self.ident` instead of `this.ident`. For RA/DEC fields with `ident` set, edits should stage `item.newValue` until Slew/Sync calls `setCoordinates()`, but the current code usually sends `MOUNT_EQUATORIAL_COORDINATES` immediately. Use `this.ident` and keep staged coordinate edits local until the explicit action button is pressed. | Open |
| SERVER-008 | Low | `resource/components.js:745`, `resource/components.js:750`, `resource/components.js:776`, `resource/components.js:786`, `resource/components.js:811`, `resource/components.js:813` | The WiFi setup component stores mode in `self.mode`, which resolves to the global window object in browsers, instead of component state. It works only because reads and writes share the same accidental global; multiple instances or future strict-mode/module loading would break. Define `data()` as a function returning `{ mode: ... }` and use `this.mode` consistently. | Open |

## Review Focus

- Command-line option parsing and default behavior.
- Driver loading/unloading and process lifecycle.
- Client/server protocol integration.
- Network/socket behavior, timeouts, and error handling.
- Shutdown cleanup and signal handling.
- Web control-panel connection handling and client-side property workflows.
- Interaction with automated and manual validation in `TESTING.md`.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Baseline review of server source, build metadata, and project-owned HTML/JS resources. |
