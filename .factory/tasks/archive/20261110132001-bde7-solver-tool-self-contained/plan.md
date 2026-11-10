# Development plan

Status: complete

## Approach and affected components
Tool-local objects for the needed framework sources (static pattern rule with `-DINDIGO_CLIENT`), local `escape()`
with `json_escape()`/`xml_escape()` in the tool, the Windows project compiling the same sources with
`INDIGO_WINDOWS_DLL` and `INDIGO_CLIENT` for the whole project and linking wininet, ws2_32, pthreadVC3, zlib and libraw.

## Baseline
- Commands and environment: Linux aarch64, gcc; `ldd build/bin/indigo_solver` listed libindigo.so, libudev, libusb.
- Existing failures: none in the tool suite (32/32).
- Repository revision / working-tree state: master 31d3cc5a2, untracked owner folders only.

## Ordered steps
| Step | Change and allowed paths | Acceptance IDs | Dependencies | Completion check | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | local escaping in `indigo_tools/indigo_solver.c` | AC-002 | None | tool suite | done |
| 2 | `indigo_tools/Makefile` | AC-001, AC-003 | 1 | ldd, suite, real files | done |
| 3 | `indigo_tools/indigo_solver.vcxproj` | AC-004 | 1 | review | done |

## Test strategy
Tool suite (covers JSON/XML escaping), ldd, real files, JSON/XML parse of the outputs, static review of the project.

## Security risks and mitigations
Escaping keeps the five XML entities and the JSON escapes of the library; control characters in file names are
replaced by the existing sanitizing.

## Dependencies and architecture decisions
The minimal source set was found by trial linking: indigo_json.c and indigo_xml.c pull in the client protocol layer
and service discovery (avahi/dns_sd), hence the local escaping.

## Migration, rollout, and rollback
Revert the commit.

## Deviations and owner decisions
None.
