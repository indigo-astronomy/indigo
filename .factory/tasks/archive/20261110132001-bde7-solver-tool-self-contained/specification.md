# Specification

Status: approved by the owner request of 2026-11-10

## Problem and outcome
`indigo_solver` is built from the INDIGO sources it needs and links no INDIGO library.

## In scope
- Framework sources compiled into the tool with `INDIGO_CLIENT`: indigo_bus, indigo_uni_io, indigo_token,
  indigo_raw_utils, indigo_timer, indigo_align, indigo_dslr_raw; static libraw and libjpeg (Linux/macOS).
- Local JSON and XML escaping replacing indigo_json_escape_b / indigo_xml_escape_b.
- Windows project: the same sources, no INDIGO import library.

## Out of scope
Other tools and libraries.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The Linux binary depends on no INDIGO shared library | `ldd` |
| AC-002 | Output unchanged: text, JSON, XML (escaping of `"&<>'\`, newline, carriage return, tab), WCS; index management | tool suite |
| AC-003 | JPEG, FITS, INDIGO RAW and camera raw files are solved | real files |
| AC-004 | The Windows project compiles the same sources and links no INDIGO library | static review |

## Failure behavior and edge cases
Unchanged.

## Permissions, data, and security requirements
Escaping must keep the documents well formed; the file name sanitizing for control characters stays.

## Compatibility and nonfunctional requirements
macOS uses the same Makefile rules; unverified here.

## Assumptions and open questions
Windows not buildable on this host.

## Owner decisions and requirement changes
None beyond the request.
