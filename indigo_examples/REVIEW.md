# indigo_examples Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Baseline review complete; open findings recorded. |

## Scope

Example clients, driver-loading samples, service discovery sample, Unix Makefile, and Visual Studio project metadata under `indigo_examples/`.

Reviewed source files:

- `Makefile`
- `client.c`
- `dynamic_driver_client.c`
- `executable_driver_client.c`
- `remote_server_client.c`
- `remote_server_client_mount.c`
- `service_discovery.c`
- `remote_server_client.vcxproj`
- `remote_server_client.vcxproj.user`

Untracked local build products under `indigo_examples/Debug/` and `.DS_Store` were not treated as reviewed source.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| EXAMPLES-001 | Medium | `Makefile:20`, `Makefile:34`, `Makefile:40` | The service discovery target is misspelled as `servce_discovery`, so `make all` builds a binary with the wrong name while `make clean` tries to remove `service_discovery`. The clean rule also omits `remote_server_client_mount` and uses plain `rm`, so it fails when outputs are absent. Rename the target to `service_discovery`, update `all`, and use `rm -f` for every generated example binary. | Open |
| EXAMPLES-002 | Medium | `dynamic_driver_client.c:162`, `dynamic_driver_client.c:164`, `dynamic_driver_client.c:169` | `driver` is uninitialized when `indigo_load_driver()` fails, but `indigo_remove_driver(driver)` is still called. A missing simulator driver or load error can therefore pass an indeterminate pointer into the driver manager. Initialize `driver = NULL` and call `indigo_remove_driver()` only after a successful load. | Open |
| EXAMPLES-003 | Medium | `remote_server_client.c:31`, `remote_server_client.c:163`, `remote_server_client.c:164`, `remote_server_client.c:167`, `remote_server_client_mount.c:30`, `remote_server_client_mount.c:177`, `remote_server_client_mount.c:178`, `remote_server_client_mount.c:181` | The remote examples hard-code service/device names and never check whether `indigo_connect_server()` succeeded before waiting forever and later disconnecting `server`. If the target host is unavailable or has a different service name, the sample can hang indefinitely and then use an invalid server pointer. Accept host/service parameters or document the default clearly, check the connect result, initialize `server = NULL`, and use a bounded wait with a clear failure exit. | Open |
| EXAMPLES-004 | Medium | `dynamic_driver_client.c:113`, `dynamic_driver_client.c:114`, `dynamic_driver_client.c:115`, `executable_driver_client.c:106`, `executable_driver_client.c:107`, `executable_driver_client.c:108`, `remote_server_client.c:109`, `remote_server_client.c:110`, `remote_server_client.c:111` | Image-saving examples call `fopen()`, `fwrite()`, and `fclose()` without checking the file handle or write count. Running from an unwritable directory or hitting disk errors can crash or silently produce corrupt example output. Check `fopen()` before writing, verify `fwrite()` writes one complete BLOB, and log errors with `strerror(errno)`. | Open |
| EXAMPLES-005 | Low | `client.c:93`, `client.c:95`, `client.c:96`, `client.c:130`, `client.c:131`, `client.c:133`, `executable_driver_client.c:133`, `executable_driver_client.c:135`, `executable_driver_client.c:136`, `executable_driver_client.c:164` | The executable-driver examples model abrupt child-process cleanup with `SIGKILL` and `exit()` from the client detach callback, and they do not handle `execl()` failure in the child. `client.c` also leaves the adapter attached/released only by process exit. As examples, these teach fragile lifecycle handling. Prefer graceful detach/release/stop flow, report child exec failure, close unused pipe ends in both processes, and terminate the child only from normal shutdown code. | Open |

## Review Focus

- Correct use of public INDIGO APIs.
- Examples staying buildable and aligned with current headers.
- Avoiding unsafe patterns that users may copy into production clients or drivers.
- Clear minimal examples without hidden external requirements.
- Bounded waits and useful failure modes for examples that depend on external servers.
- Repository hygiene around generated build outputs.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Baseline review of example sources, Makefile, and Visual Studio project metadata. |
