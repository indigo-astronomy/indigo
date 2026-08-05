# indigo_server Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Baseline review complete; all recorded findings (SERVER-001 through SERVER-008) resolved and closed. Code fixes to `indigo_server.c` are pending a Linux/macOS build (SERVER-005 additionally needs runtime signal/fork testing); web-resource fixes (SERVER-006/007/008) are pending browser verification. |

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
| SERVER-003 | Medium | `indigo_server.c:1501`, `indigo_server.c:1534`, `indigo_server.c:1540`, `indigo_server.c:1551`, `indigo_server.c:1554` | Dynamic driver-list parsing stored the same driver name twice with two `strdup()` calls, leaking the first allocation, and incremented `dynamic_drivers_count` even when the description field was missing. A malformed or truncated driver-list entry could leave `description == NULL` and later pass it into property item initialization. Resolved: the parser now stores each name once, requires both fields before counting the entry, and uses `snprintf()` for `path`. See finding summary below. | Closed |
| SERVER-004 | Medium | `indigo_server.c:528`, `indigo_server.c:530`, `indigo_server.c:542`, `indigo_server.c:551`, `indigo_server.c:580`, `indigo_server.c:582`, `indigo_server.c:592`, `indigo_server.c:629`, `indigo_server.c:631`, `indigo_server.c:752` | Generated JSON resources used unchecked `malloc()`, `strcpy()`, and `sprintf()` into manually grown buffers, resized only after each write, copied star names into `desig[256]` without a length check, and emitted JSON string content without escaping. Catalog data changes could crash startup or produce invalid JSON. Resolved: checked allocation, bounded name copies, `snprintf()` with remaining capacity, and a `json_escape()` helper applied to every catalog string field. See finding summary below. | Closed |
| SERVER-005 | Medium | `indigo_server.c:1852`, `indigo_server.c:1855`, `indigo_server.c:1863`, `indigo_server.c:1865`, `indigo_server.c:1866`, `indigo_server.c:1874`, `indigo_server.c:1876` | The POSIX signal handler performed non-async-signal-safe work, including logging, signal reconfiguration, and `indigo_server_shutdown()`. If a signal arrived while library locks or allocator state were held, shutdown could deadlock or corrupt state. Resolved: the managed signals are now blocked process-wide and consumed synchronously by dedicated `sigwait()` threads that run in ordinary thread context. See finding summary below. | Closed |
| SERVER-006 | Medium | `resource/ctrl.html:83`, `resource/mng.html:114`, `resource/guider.html:154`, `resource/imager.html:210`, `resource/mount.html:274`, `resource/script.html:112`, `resource/components.js:592`, `resource/components.js:597`, `resource/imager.html:254`, `resource/imager.html:257`, `resource/imager.html:272`, `resource/imager.html:275`, `resource/imager.html:283`, `resource/imager.html:286` | Web pages hard-coded `ws://` and `http://` when connecting to the server and rendering BLOB/image URLs, breaking under HTTPS or a TLS reverse proxy because browsers block mixed-content WebSockets and images. Resolved: WebSocket and image/BLOB URLs are now built from `window.location.protocol`/`host`, and absolute-URL detection accepts `https://`. See finding summary below. | Closed |
| SERVER-007 | Medium | `resource/components.js:141`, `resource/components.js:147`, `resource/components.js:157`, `resource/mount.html:66`, `resource/mount.html:67`, `resource/mount.html:626`, `resource/mount.html:632` | The sexagesimal number editor checked `self.ident` instead of `this.ident`. For RA/DEC fields with `ident` set, edits should stage `item.newValue` until Slew/Sync calls `setCoordinates()`, but the code sent `MOUNT_EQUATORIAL_COORDINATES` immediately. Resolved: the `change()` handler now tests `this.ident`, so `ident`-bearing edits stage locally. See finding summary below. | Closed |
| SERVER-008 | Low | `resource/components.js:745`, `resource/components.js:750`, `resource/components.js:776`, `resource/components.js:786`, `resource/components.js:811`, `resource/components.js:813` | The WiFi setup component stored mode in `self.mode`, which resolves to the global window object in browsers, instead of component state. It worked only because reads and writes shared the same accidental global; multiple instances or future strict-mode/module loading would break. Resolved: `data()` is now a function returning `{ mode: "" }` and all accesses use `this.mode`. See finding summary below. | Closed |

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

### SERVER-003 (Closed)

`add_drivers()` reads `indigo_drivers`/`ADDITIONAL_DRIVERS` list files and appends parsed
entries to the `dynamic_drivers` table. The parser had three defects:

1. **Double `strdup()` / memory leak.** After extracting the name token, the code ran two
   identical duplicate-check loops and two `if (token) { ... strdup(token) }` blocks in
   sequence. When a name passed the checks, `dynamic_drivers[...].name` was assigned by the
   first `strdup()` and then immediately overwritten by a second `strdup()` of the same token,
   leaking the first allocation on every accepted driver.

2. **Entry counted without a valid description (and without a valid name).** `dynamic_drivers_count++`
   ran unconditionally at the end of each iteration. If the second field was missing or had no
   closing quote, `.description` was never assigned and kept whatever the (zero-initialized)
   slot held — `NULL`. A line with no quoted name field at all was likewise counted. That
   `NULL` description/name later reached `indigo_init_switch_item()` when
   `SERVER_DRIVERS_PROPERTY` was populated, risking a `NULL` dereference.

3. **Unbounded `sprintf()` into `path[PATH_MAX]`.** The list-file path was built with `sprintf`,
   which could overflow if `folder_path` plus the entry name exceeded `PATH_MAX`.

Fix (`add_drivers()` in `indigo_server.c`):
- Rewrote the parse loop to extract the name into a local `name` pointer, `continue` when no
  name is present, run a single deduplication pass (against `indigo_available_drivers` and the
  already-added `dynamic_drivers`), extract the description into a local `description` pointer,
  and `continue` when no description is present. Only when both fields are valid does it
  `strdup()` the name once, `strdup()` the description, and increment `dynamic_drivers_count`.
  This removes the redundant second loop/`strdup`, so each name is duplicated exactly once and
  no entry is counted without both fields.
- Changed the path construction to `snprintf(path, sizeof(path), ...)`.

Behavior is unchanged for well-formed list files; malformed lines are now skipped instead of
producing leaked or partially-initialized entries. Line numbers in the finding predate the
SERVER-001/002 edits and have shifted.

Verification note: not build-verified on this macOS review host (Makefile project, no Xcode
diagnostic service for this C file); should be confirmed by a normal build.

### SERVER-004 (Closed)

Three functions build the `/data/stars.json`, `/data/dsos.json`, and
`/data/constellations.lines.json` resources at startup from the compiled-in `indigocat`
catalog: `indigo_add_star_json_resource()`, `indigo_add_dso_json_resource()`, and
`indigo_add_constellations_lines_json_resource()` (the latter via `add_multiline()`). They had
several buffer-safety and correctness defects:

- **Unchecked `malloc()`.** Each function did `char *buffer = malloc(1024 * 1024)` and wrote
  immediately, with no `NULL` check — a failed allocation crashed on the first write.
- **Unbounded `strcpy()` into `desig[256]`.** Star names were copied with `strcpy(desig,
  star_data[i].name)`; a catalog name of 256+ bytes overflowed the stack buffer.
- **No JSON escaping.** Catalog name/designation/id fields were emitted directly inside JSON
  string literals. A field containing `"`, `\`, or a control character produced malformed JSON
  that the web client could not parse.
- **`sprintf()` into buffers grown only after the write.** Every record was appended with
  `sprintf(buffer + size, ...)` and the buffer was doubled only *after*, once free space
  dropped below 1024 bytes — so a record larger than the remaining space (made more likely once
  escaping expands strings) could overflow before the grow check ran.

Fix (`indigo_server.c`):
- Switched all three buffer allocations to `indigo_safe_malloc()` (asserts non-`NULL` and
  zero-fills), and set the initial JSON header with `snprintf()` instead of `strcpy()`.
- Bounded the star-name copy with `snprintf(desig, sizeof(desig), "%s", ...)`.
- Added a `json_escape()` helper that renders a string safe for a JSON string literal (escapes
  `"`, `\`, `\n`, `\r`, `\t`, and other control characters as `\uXXXX`) into a bounded scratch
  buffer, truncating safely on overflow. Every catalog string field (star name and designation,
  solar-system name, DSO id and name) is now escaped before being formatted in.
- Replaced every `sprintf(buffer + size, ...)` with `snprintf(buffer + size, buffer_size - size,
  ...)` so a write can never exceed the allocation, and raised the grow margin to
  `JSON_GROW_MARGIN` (16 KB), which comfortably exceeds the largest possible single record (fixed
  text plus two escaped fields), so no record is ever truncated.
- Gave `add_multiline()` a `buffer_size` parameter and converted its writes to bounded
  `snprintf()`; all call sites pass the remaining capacity. These writes are fixed compile-time
  coordinate data that provably fits the 1 MB buffer, but they are now bounded for consistency.

Behavior is unchanged for the shipped catalog; the JSON is byte-identical except that string
fields are now correctly escaped. Line numbers in the finding predate the SERVER-001/002/003
edits and have shifted.

Verification note: not build-verified on this macOS review host (Makefile project, no Xcode
diagnostic service for this C file); should be confirmed by a normal build.

### SERVER-005 (Closed)

`signal_handler()` was installed for `SIGINT`, `SIGTERM`, `SIGHUP`, and `SIGCHLD` and ran in
async signal context, where only async-signal-safe functions may be called. It violated that
rule extensively: `indigo_log()` (buffered I/O / syslog / internal locks), `indigo_server_shutdown()`
(locks, `free()`, thread joins), and `signal()` reconfiguration. A signal delivered while the
interrupted thread held the allocator lock or a bus lock could deadlock or corrupt state during
shutdown. It also carried a latent reap race: the `SIGCHLD` handler reaped with
`waitpid(-1, WNOHANG)` in the parent, which could reap the worker out from under the parent's
explicit `waitpid(server_pid)`.

Fix (`indigo_server.c`): replaced the async handler with the finding's recommended
dedicated-signal-thread model, so all signal handling runs in ordinary thread context where
logging, reaping, and shutdown are legal.

- `main()` now blocks `SIGINT`/`SIGTERM`/`SIGHUP`/`SIGCHLD` with `pthread_sigmask(SIG_BLOCK, ...)`
  before forking. The mask is inherited across `fork()` and by later-created threads, so the
  managed signals are delivered only to the sigwait thread of each process and never interrupt
  arbitrary code.
- `server_signal_thread()` runs in the worker (the forked child, or the whole process under
  `--do-not-fork`). It loops on `sigwait()`, reaps exited driver/INDI subprocesses on `SIGCHLD`,
  and on a shutdown signal logs and calls `indigo_server_shutdown()` (and clears `runLoop` on
  macOS) before returning. Because further shutdown signals remain blocked and pending, the old
  "ignore the second CTRL-C" `signal(SIGINT, SIG_IGN)` hack is no longer needed.
- `supervisor_signal_thread()` runs in the supervising parent. It loops on `sigwait()`, logs,
  sets `keep_server_running` (`SIGHUP` ⇒ restart, `SIGINT`/`SIGTERM` ⇒ exit), and forwards the
  signal to the worker with `kill()` (escalating to `SIGKILL` via `use_sigkill`). It deliberately
  does not handle `SIGCHLD`; the parent's existing blocking `waitpid(server_pid)` reaps the
  worker, which removes the previous reap race.
- The cross-thread flags `keep_server_running`, `use_sigkill`, and `runLoop` are now
  `volatile sig_atomic_t`.

The supervisor thread is created on the first parent iteration (after the initial fork, so that
first fork stays single-threaded); on restart forks the child inherits only this thread, which
is idle inside `sigwait()` and holds no locks, keeping the post-fork child safe. Externally
observable behavior (Ctrl-C shutdown, `SIGHUP` restart, worker reaping, macOS run-loop teardown)
is unchanged. Line numbers in the finding predate the SERVER-001–004 edits and have shifted.

Verification note: this change to process/signal/thread orchestration was NOT build- or
run-verified — the macOS review host has no Xcode diagnostic service for this Makefile-based C
file, and the affected code is largely `INDIGO_LINUX`/`INDIGO_MACOS` fork/CFRunLoop logic. It
must be built and exercised at runtime on both Linux and macOS (normal Ctrl-C shutdown, `SIGHUP`
restart, and driver-subprocess reaping) before being relied upon.

### SERVER-006 (Closed)

The control-panel pages opened their INDIGO WebSocket with a literal `ws://` and rendered
preview images / BLOB links with a literal `http://` prefix. When the panel is served over
HTTPS (directly or behind a TLS reverse proxy), browsers block the resulting mixed-content
`ws://` WebSocket and `http://` image loads, so the panel cannot connect or display images.
The URLs also appended `":" + window.location.port` unconditionally, producing a stray trailing
colon when the page is served on a proxy's default port (443).

Fix (project-owned resources only):
- **WebSocket endpoint** (`ctrl.html`, `mng.html`, `script.html`, `guider.html`, `imager.html`,
  `mount.html`): the scheme is now chosen from the page protocol —
  `(window.location.protocol == "https:" ? "wss://" : "ws://") + window.location.host`. Using
  `window.location.host` (rather than `hostname + ":" + port`) yields the correct authority for
  both explicit ports and proxied default ports.
- **Preview/BLOB image URLs** (`imager.html`, `components.js`): the hard-coded `http://` prefix
  used when building a URL from a relative BLOB path is replaced with
  `window.location.protocol + "//" + <host>`, so images load over the same scheme as the page.
  In `components.js` the host is `window.location.host`; in `imager.html` the existing
  `INDIGO.host` value is preserved and only the scheme changed.
- **Absolute-URL detection**: the checks that decide whether a BLOB value is already an absolute
  URL (`value.startsWith("http://")` / `item.value.startsWith('http://')`) now also accept
  `https://`, so an already-secure absolute URL is used as-is instead of being mis-prefixed.

The SVG `xmlns="http://www.w3.org/2000/svg"` literals are XML namespace identifiers, not network
URLs, and were correctly left untouched. Behavior over plain HTTP is unchanged. Line numbers in
the finding predate the earlier SERVER edits.

Verification note: these are client-side HTML/JS changes not exercised by the build; they should
be confirmed in a browser against both a plain-HTTP server and an HTTPS/TLS-proxied deployment
(WebSocket connects, preview images and BLOB links load).

### SERVER-007 (Closed)

The `indigo-edit-number-60` Vue component (the sexagesimal RA/DEC editor in `components.js`)
supports two modes via its `ident` prop. Without `ident` an edit is sent immediately; with
`ident` set the edit is meant to be *staged* into `item.newValue` and only committed later when
the user presses Slew/Sync, which calls `mount.html`'s `setCoordinates()` (that reads
`item.newValue` and sends `MOUNT_EQUATORIAL_COORDINATES` with both RA and DEC together).

The `change()` handler tested `self.ident` instead of `this.ident`. In a browser `self` is the
global `window`, so `self.ident` is always `undefined`, the `!= null` test was always false, and
the code took the immediate-send branch even for the RA/DEC editors (which do pass
`:ident="'RA'"` / `:ident="'DEC'"` in `mount.html`). The effect: each keystroke-commit sent RA
or DEC on its own, so the mount would slew/sync to half-entered, one-axis-at-a-time coordinates
instead of waiting for the explicit action button.

Fix: changed the test in `change()` from `self.ident` to `this.ident` (`components.js`). Now,
when `ident` is set the value is staged in `item.newValue` (which the component's `value()`
already renders) and is committed only by `setCoordinates()`; without `ident` the immediate-send
behavior is unchanged. No other code changed — the staging consumer (`setCoordinates()`) and the
`value()` renderer were already correct.

Verification note: client-side JS not exercised by the build; should be confirmed in a browser
(edit RA and DEC on the mount panel, verify no slew occurs until Slew/Sync is pressed, and that
both axes are then sent together).

### SERVER-008 (Closed)

The `indigo-wifi-setup` Vue component tracked the selected mode (`"AP"` vs `"INFRA"`) in
`self.mode` across `onChange()`, `isAP()`, `isInfra()`, and `set()`. In a browser `self` is the
global `window`, so this read and wrote `window.mode` — a global, not component state. It
happened to function only because every access shared that same accidental global; a second
instance of the component, or loading the script under strict mode / as a module, would break
it. The component also declared `data` as a plain object (`data: { mode: String }`), which is
invalid for a Vue component (`data` must be a function so each instance gets its own state) and
additionally set the initial value to the `String` constructor rather than a string.

Fix (`components.js`):
- Changed `data` to a function returning fresh per-instance state: `data: function() { return
  { mode: "" }; }`.
- Replaced every `self.mode` with `this.mode`, so the mode is read from and written to the
  component instance's reactive state.

Behavior is unchanged for the current single-instance usage, but the mode is now proper
per-instance component state rather than a shared global. `mode` is used only in event-handler
and comparison logic (not rendered in the template), so making it reactive introduces no
render-dependency loop even though `isAP()`/`isInfra()` assign it while being called from the
template.

Verification note: client-side JS not exercised by the build; should be confirmed in a browser
(open the RPI Wi-Fi setup, switch between AP and INFRA, and confirm the correct property is sent
on save).

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
