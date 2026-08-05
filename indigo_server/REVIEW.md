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
| SERVER-001 | High | `indigo_server.c:763`, `indigo_server.c:771`, `indigo_server.c:805`, `indigo_server.c:811`, `indigo_server.c:1362`, `indigo_server.c:1368`, `indigo_server.c:1374`, `indigo_server.c:1402` | RPI-management commands were assembled with client-controlled property text inside shell command strings and executed with `popen()`. SSID, password, country-code, and host-time values could contain quotes or shell metacharacters, so a remote client with access to these properties could run arbitrary shell fragments when RPI management is enabled. Resolved: added a `shell_escape()` helper and applied it to every client-controlled argument. See finding summary below. | Closed |
| SERVER-002 | High | `indigo_server.c:505`, `indigo_server.c:1889`, `indigo_server.c:1933`, `indigo_server.c:1611`, `indigo_server.c:1612` | Command-line arguments were copied into fixed `server_argv[128]` without a bounds check, and `--bonjour`/`-b` read `server_argv[i + 1]` without confirming an argument exists. A long invocation could write past `server_argv`, while a trailing `-b` could read outside the saved argument list. Resolved: `main()` now rejects too many arguments and the `-b`/`--bonjour` branch requires a following value. See finding summary below. | Closed |
| SERVER-003 | Medium | `indigo_server.c:1501`, `indigo_server.c:1534`, `indigo_server.c:1540`, `indigo_server.c:1551`, `indigo_server.c:1554` | Dynamic driver-list parsing stores the same driver name twice with two `strdup()` calls, leaking the first allocation, and increments `dynamic_drivers_count` even when the description field is missing. A malformed or truncated driver-list entry can leave `description == NULL` and later pass it into property item initialization. Store each parsed name once, require both fields before incrementing the count, and use `snprintf()` for `path`. | Open |
| SERVER-004 | Medium | `indigo_server.c:528`, `indigo_server.c:530`, `indigo_server.c:542`, `indigo_server.c:551`, `indigo_server.c:580`, `indigo_server.c:582`, `indigo_server.c:592`, `indigo_server.c:629`, `indigo_server.c:631`, `indigo_server.c:752` | Generated JSON resources use unchecked `malloc()`, `strcpy()`, and `sprintf()` into manually grown buffers. The code resizes only after writing each record, star names are copied into `desig[256]` without a length check, and JSON string content is emitted without escaping. Catalog data changes can crash startup or produce invalid JSON. Use checked allocation, `snprintf()` with remaining capacity before appending, bounded name copies, and JSON string escaping. | Open |
| SERVER-005 | Medium | `indigo_server.c:1852`, `indigo_server.c:1855`, `indigo_server.c:1863`, `indigo_server.c:1865`, `indigo_server.c:1866`, `indigo_server.c:1874`, `indigo_server.c:1876` | The POSIX signal handler performs non-async-signal-safe work, including logging, `waitpid()` loops, signal reconfiguration, and `indigo_server_shutdown()`. If a signal arrives while library locks or allocator state are held, shutdown can deadlock or corrupt state. Use a self-pipe/eventfd or atomic flag and perform shutdown/reap work from the main loop or a dedicated signal thread. | Open |
| SERVER-006 | Medium | `resource/ctrl.html:83`, `resource/mng.html:114`, `resource/guider.html:154`, `resource/imager.html:210`, `resource/mount.html:274`, `resource/script.html:112`, `resource/components.js:592`, `resource/components.js:597`, `resource/imager.html:254`, `resource/imager.html:257`, `resource/imager.html:272`, `resource/imager.html:275`, `resource/imager.html:283`, `resource/imager.html:286` | Web pages hard-code `ws://` and `http://` when connecting to the server and rendering BLOB/image URLs. This breaks when the control panel is served through HTTPS or a TLS reverse proxy because browsers block mixed-content WebSockets and images. Build URLs from `window.location.protocol` (`ws` vs `wss`, `http` vs `https`) and normalize relative BLOB paths with the `URL` API. | Open |
| SERVER-007 | Medium | `resource/components.js:141`, `resource/components.js:147`, `resource/components.js:157`, `resource/mount.html:66`, `resource/mount.html:67`, `resource/mount.html:626`, `resource/mount.html:632` | The sexagesimal number editor checks `self.ident` instead of `this.ident`. For RA/DEC fields with `ident` set, edits should stage `item.newValue` until Slew/Sync calls `setCoordinates()`, but the current code usually sends `MOUNT_EQUATORIAL_COORDINATES` immediately. Use `this.ident` and keep staged coordinate edits local until the explicit action button is pressed. | Open |
| SERVER-008 | Low | `resource/components.js:745`, `resource/components.js:750`, `resource/components.js:776`, `resource/components.js:786`, `resource/components.js:811`, `resource/components.js:813` | The WiFi setup component stores mode in `self.mode`, which resolves to the global window object in browsers, instead of component state. It works only because reads and writes share the same accidental global; multiple instances or future strict-mode/module loading would break. Define `data()` as a function returning `{ mode: ... }` and use `this.mode` consistently. | Open |

## Finding Summaries

### SERVER-001 (Closed)

The RPI-management handlers build shell command strings for `s_rpi_ctrl.sh` with printf-style
formatting and run them through `popen()` (`execute_command()`/`execute_query()`). Four handlers
interpolated client-controlled property text into those strings inside double quotes
(`"%s"`): the Wi-Fi country code (`indigo_server.c:1362`), the access-point SSID and password
(`:1368`), the infrastructure SSID and password (`:1374`), and the host time (`:1402`). Because
`popen()` runs the string through `/bin/sh`, any of these values containing a double quote,
backtick, `$(...)`, `;`, `|`, or `&` broke out of the quoting and executed attacker-chosen shell.
Any client authorized to set these properties (available whenever the server is built with
`RPI_MANAGEMENT`) could therefore achieve arbitrary command execution on the host.

Fix: added a `shell_escape()` helper (`indigo_server.c`, just before `execute_command()`) that
renders a string as a single POSIX shell token — it wraps the value in single quotes and
rewrites each embedded single quote as `'\''`, which is injection-proof because no character is
special inside single quotes. Each of the four call sites now passes its client-controlled
value through `shell_escape()` into a stack buffer sized `INDIGO_VALUE_SIZE * 4 + 3` (worst-case
4x expansion plus the two surrounding quotes and the terminator) and uses a bare `%s` in the
format string instead of `\"%s\"`. On overflow the helper truncates but keeps the quoting
balanced, so a pathological value makes the command fail rather than inject.

Scope notes: the Wi-Fi channel handler (`:1380`) uses `%d` on an `int` and was already safe.
`execute_query()` was listed in the finding, but all of its callers use constant command
strings with no client input, so it was not exploitable and needed no change; the static
`execute_command()` calls (`--enable-forwarding`, `--disable-forwarding`, `--poweroff`,
`--reboot`) are likewise safe. No behavioral change for well-formed input — SSIDs, passwords,
country codes, and dates without shell metacharacters produce the same effective command.

Verification note: `RPI_MANAGEMENT` is a Linux/Raspberry Pi build-time option and is not
compiled on the macOS review host, so this change was not build-verified locally; it should be
confirmed on an RPI build.

### SERVER-002 (Closed)

Two out-of-bounds defects in `main()`'s command-line handling:

1. **Overflow of `server_argv`.** The parse loop copied every non-option token with
   `server_argv[server_argc++] = argv[i]` into the fixed `static char const *server_argv[128]`
   with no bound. `server_argc` starts at 1 (slot 0 holds `argv[0]`), so an invocation with 128
   or more accumulated arguments wrote past the end of the array — an out-of-bounds write of
   attacker/caller-influenced pointers into adjacent static storage.

2. **Out-of-bounds read for `-b`/`--bonjour`.** Unlike every other value-consuming option (all
   guarded with `&& i < server_argc - 1`), the `-b`/`--bonjour` branch unconditionally read
   `server_argv[i + 1]`. A trailing `-b` with no following value read one slot past the used
   entries — either a zero-initialized `NULL` (which `INDIGO_COPY_NAME`/`strncpy` then
   dereferences, crashing) or, when the array was full, genuinely out of bounds.

Fix:
- Introduced `#define SERVER_ARGV_SIZE 128` and used it for both the array declaration and the
  bound. The accumulation branch is now `else if (server_argc < SERVER_ARGV_SIZE)`; on overflow
  `main()` prints `Too many arguments, at most 127 are supported` to `stderr` and returns `1`
  instead of writing past the array.
- Added `&& i < server_argc - 1` to the `-b`/`--bonjour` condition, matching the guard already
  used by `-p`, `-r`, `-i`, `-T`, `-a`, so a trailing `-b` is ignored (consistent with the other
  options) rather than reading past the argument list.

No behavioral change for valid invocations. Line numbers in the finding predate the SERVER-001
edit and have since shifted; the affected code is the `server_argv` declaration, the argument
accumulation `else` branch, and the `-b`/`--bonjour` branch in `main()`.

Verification note: not build-verified on this macOS review host (Makefile-based project, no
Xcode diagnostic service for this C file); the changed paths compile on all platforms and should
be confirmed by a normal build.

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
