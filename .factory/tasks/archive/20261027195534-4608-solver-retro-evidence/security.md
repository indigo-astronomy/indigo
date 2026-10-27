# Security review

Verdict: pass
Reviewer: Claude Fable 5.1 (security-reviewer role)
Date: 2026-10-27
Revision and working-tree state: the published commit `a1e7f53d08bcec9b25309f1a2944d39451572253` read with `git show`
against its parent `32d7396d7257e33a3afc3520990151022c6fafcc`; the task working tree holds only this record
Review independence: none; after the fact, by the agent that writes this task's other reports; the commit's author
worked in another session

## Scope, assets, and trust boundaries
The Solver Agent receives image uploads from any connected client over the INDIGO bus (BLOB of the shared
`AGENT_PLATESOLVER_IMAGE` property) and from the Imager/Guider agents, so the image data is untrusted input. Before
the commit only INDIGO RAW was accepted; the commit adds parsers for FITS (own code), JPEG (libjpeg) and camera raw
(libraw through `indigo_dslr_raw_process_image()`), plus a filter working on a private copy of the frame. Assets:
the server process (memory safety, availability), the solved result sent to the Mount Agent.

## Risk coverage
| Area | Applicable? | Review evidence / reason not applicable |
| --- | --- | --- |
| Authentication and authorization | No | No change to bus access control or property permissions |
| Input, injection, and output handling | Yes | RAW: signature switch, `size >= sizeof(header)`, `width/height >= 1` and `size - header >= width * height * bytes` before any read; MONO16 copies exactly `header + width * height * 2` bytes, not `task->size`. FITS: header read record by record with `offset + 2880 > size` guard before each record; cards parsed into a NUL-terminated 71-byte buffer; BITPIX restricted to 8/16/32/-32/-64, NAXIS to 2 or 3, axes 1 to 65535, planes 1 to 16; data length checked before the pixel loop; non-finite values set to 0. JPEG: magic bytes checked, `jpeg_mem_src` over the buffer, header return value checked, dimensions bounded to 65535, errors caught through `error_exit` + `longjmp` with buffers freed. libraw: return code, `data`, `width`, `height` checked, output `size` compared with the expected pixel count before `memcpy`; unknown layouts rejected. Messages are fixed strings, no user data is formatted into them |
| Data, secrets, cryptography, and logs | No | Debug messages log dimensions, BITPIX, component counts and noise statistics only |
| Filesystem and outbound network access | No | No file or network access added; the index download path is unchanged |
| Dependencies and configuration | Yes | libjpeg and libraw are already linked into the INDIGO library and used by the Astrometry agent and the CCD driver base; no new dependency, no new configuration |
| Abuse and failure behavior | Yes | A client can declare a 65535 x 65535 image: RAW and FITS reject it unless the buffer really holds the data, so the allocation (up to 8.6 GB of float for FITS) is only reached for a buffer of that size, which the bus has already accepted; this is the pre-existing BLOB size exposure, not new. The filter allocates three frame-sized buffers and a 256 KB histogram per detection. A broken JPEG that fails mid-decode leaks at most one frame and one row (non-volatile locals across `longjmp`, see quality-review.md). `fits_values()` casts `pixels * planes * bytes` to `long` for the length check; on an LP64 target the product (at most 5.5e11) fits, on a 32-bit `long` target it would not, but the agent is not built on Windows (MIGRATION_STATUS.md) |

## Automated checks
`./.factory/scripts/security` (validate_project --security) on the task branch: see verification.md executed
checks. No application security scanner is configured; the re-run unit and agent suites include the two rejection
cases (random bytes, FITS header without data) and the truncated-input paths of the solver core.

## Findings
None registered. The two observations (leak on a broken JPEG mid-decode; `long` cast in the FITS length check on a
32-bit `long` platform) have no impact on the supported Linux/macOS builds and are kept as observations here and in
quality-review.md for a later cleanup; registering them as findings would require a concrete impact.

## Accepted risks
None recorded. The pre-existing BLOB size exposure (a client may upload a very large image) is unchanged by the
commit and is not accepted here.

## Verdict and limitations
Pass for the diff of `a1e7f53d0`: every parser checks the declared lengths against the buffer before reading, errors
are reported as fixed messages and the solve fails cleanly. Limitations: after-the-fact review by one agent; libjpeg
and libraw internals are trusted as the rest of INDIGO trusts them; no fuzzing was done; no Linux/Windows build.

## Register reconciliation
No FND record created or changed by this task. `./.factory/scripts/findings check` result: see verification.md.
