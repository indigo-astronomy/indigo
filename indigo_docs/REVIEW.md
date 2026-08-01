# indigo_docs Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Baseline review complete; open findings recorded. |

## Scope

Developer and user documentation under `indigo_docs/`, including Markdown guides, documentation index files, and referenced image/PDF assets.

Reviewed files:

- `HOME.md`
- `README.md`
- `DEVELOPMENT.md`
- `DRIVER_DEVELOPMENT_BASICS.md`
- `DRIVER_GENERATOR_MIGRATION.md`
- `CLIENT_DEVELOPMENT_BASICS.md`
- `INDIGO_SERVER_AND_DRIVERS_GUIDE.md`
- `MAKEFILES.md`
- `PROPERTIES.md`
- `PROPERTY_MANIPULATION.md`
- `PROTOCOLS.md`
- `INDIGO_SCRIPT_GUIDE.md`
- User guides for guiding, imaging, plate solving, polar alignment, raw image format, camera saved images, multi-instance support, and access control.

Image and PDF assets were checked for presence when referenced, not content-reviewed.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| DOCS-001 | Medium | `INDIGO_SERVER_AND_DRIVERS_GUIDE.md:11`, `INDIGO_SERVER_AND_DRIVERS_GUIDE.md:20`, `INDIGO_SERVER_AND_DRIVERS_GUIDE.md:26`, `INDIGO_SERVER_AND_DRIVERS_GUIDE.md:28`, `indigo_server/indigo_server.c:1912`, `indigo_server/indigo_server.c:1913`, `indigo_server/indigo_server.c:1918`, `indigo_server/indigo_server.c:1923` | The server guide embeds a 2021 `indigo_server -h` output that no longer matches the current help text. It omits `-d-` / `--disable-blob-buffering`, `-C` / `--enable-blob-compression`, `-vvb` / `--enable-trace-bus`, and conditional `-f` / `--enable-rpi-management`, while retaining old version text and typos. Refresh this block from the current server help and keep the option descriptions below it in sync. | Open |
| DOCS-002 | Medium | `CLIENT_DEVELOPMENT_BASICS.md:271`, `CLIENT_DEVELOPMENT_BASICS.md:273` | The "Handling Properties Asynchronously" section contains two consecutive versions of the same paragraph, one typo-heavy and one corrected. This makes the guidance look contradictory or accidentally duplicated in a core developer document. Remove the obsolete first paragraph and keep one edited version. | Open |
| DOCS-003 | Low | `DEVELOPMENT.md:27`, `CLIENT_DEVELOPMENT_BASICS.md:440` | Some developer-facing links point to names or paths that do not exist in the current tree. `DEVELOPMENT.md` links `indigo_libs/indigo_bus.h`, but the header is under `indigo_libs/indigo/indigo_bus.h`; `CLIENT_DEVELOPMENT_BASICS.md` displays `indigo_examples/service_ddiscovery.c` even though the actual file is `indigo_examples/service_discovery.c`. Correct the link targets and visible labels so readers can navigate locally and on GitHub. | Open |
| DOCS-004 | Low | `MAKEFILES.md:11`, `MAKEFILES.md:17`, `MAKEFILES.md:21`, `DRIVER_GENERATOR_MIGRATION.md:87`, `Makefile.drv:112`, `Makefile.drv:113` | Build documentation explains generated `Makefile.inc` and generator integration only briefly and contains spelling errors in names developers will copy (`Makefie.inc`, `usuall`). It also does not explain that `Makefile.drv` invokes `$(BUILD_BIN)/indigo_generator`, so a developer must build the generator before relying on automatic `.driver` regeneration. Clarify prerequisites and correct the names. | Open |
| DOCS-005 | Low | `CLIENT_DEVELOPMENT_BASICS.md:509`, `remote_server_client.c:31`, `remote_server_client.c:163`, `remote_server_client_mount.c:30`, `remote_server_client_mount.c:177` | Client documentation says the remote-server example is a working sample but only warns that the simulator driver must be loaded. The actual examples hard-code `W11` and `indigosky` service/device names and have no bounded failure path when the service is absent. Either document those defaults and failure behavior explicitly or update the docs after the examples are made parameterized and bounded. | Open |

## Review Focus

- Accuracy against current driver, server, library, and tool behavior.
- Correct property names, state semantics, and API paths.
- Build, testing, and platform instructions.
- Generated-driver migration guidance.
- Broken or stale local links and references.
- Avoiding obsolete workflows or examples that mislead driver/client developers.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Baseline review of documentation guides, index files, and referenced assets. |
