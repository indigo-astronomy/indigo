# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `ldd build/bin/indigo_solver` | Linux build | libc, libm, libz, libstdc++, libgcc_s only | after change |
| T-002 | AC-002 | `make test-indigo-solver-tool` (includes `structured_output_escapes_the_file_name`) | synthetic | 32/32 | after change |
| T-003 | AC-003 | manual solves and JSON/XML parse | format matrix and DNG files | solved, documents parse | after change |
| T-004 | AC-004 | XML parse of the project, independent static review | none | well formed, no findings left | after change |

## Baseline evidence
Before: `ldd` listed libindigo.so, libudev.so.1, libusb-1.0.so.0; suite 32/32.

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Escaping of `"&<>\` covered by the existing suite case.

## Isolation and cleanup
None needed.

## Deferred tests and coverage limitations
No Windows or macOS build here.
