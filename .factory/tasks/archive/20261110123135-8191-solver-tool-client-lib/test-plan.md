# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | `ldd build/bin/indigo_solver` | Linux build | only libindigo_client of INDIGO, no libusb/libudev | after change |
| T-002 | AC-002 | `make test-indigo-solver-tool` | synthetic index and frames | 32/32 | after change |
| T-003 | AC-002 | manual solves | format matrix JPEG/FITS/RAW, two DNG files | solved | after change |
| T-004 | regression | `make test-agent-platesolver` | none | 29/29 | after change |
| T-005 | AC-003, AC-004 | XML parse of the changed projects, solution GUID/path check | none | well formed, no duplicates, files exist | after change |

## Baseline evidence
Before: `ldd` listed libindigo.so, libudev.so.1, libusb-1.0.so.0.

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Not applicable.

## Isolation and cleanup
No files left by the checks.

## Deferred tests and coverage limitations
No Windows or macOS build on this host; the Windows projects are reviewed statically only.
