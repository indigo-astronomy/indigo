# Verification

Verdict: pass
Reviewed source: working-tree candidate over `a3d9a78fc`. The task branch was fast-forwarded from `d25dcabf2` to origin/master `1cb0ab81c` by a `git pull --ff-only` at 23:40 and to `a3d9a78fc` before the commit; the incoming commits touch `ccd_rpi`, the PrimaLuce drivers, their tests and records, `PROPERTIES.md`, `TEST_SUMMARY.md` and the Xcode project, not the framework or the generator. The fake suite, the generator test and the 120-definition comparison were rerun on `a3d9a78fc`. Final source identities are in process.json. Runtime verification was performed by the implementing agent; an independent read-only agent reviewed the diff (quality-review.md).

## Environment
- macOS 27 (Darwin 27.0.0) arm64, Apple clang, working directory `/Users/polakovic/Development/indigo`, tests run from `indigo_test/`.
- Raspberry Pi 3 Model B+, Debian 13 (trixie), kernel 6.18.50+rpt-rpi-v8 aarch64, gcc 14, libcamera 0.7.2+rpt20261009, Raspberry Pi Camera Module 1 (OV5647), checkout `~/indigo` on branch `task/20261024210959-0eba-ccd-libcamera-driver` created from origin/master `d25dcabf2` after the CCD RPI kamera session finished; working-tree files copied from the Mac checkout.

## Acceptance results
| ID | Result | Evidence |
| --- | --- | --- |
| AC-001 | pass | fake: discovery cases; hardware: device `OV5647` for model `ov5647` |
| AC-002 | pass | fake: metadata/open-failure cases; hardware: CCD_INFO 2592 x 1944, 1.4 µm, 4 modes, reconnect x3 |
| AC-003 | pass | fake: payload/ROI/Bayer/per-mode limit assertions; hardware: every mode, subframe 100,50,300x200, 2 s exposure (2.7 s), gain 1..63.94 |
| AC-004 | pass | fake: abort/disconnect cases; hardware: abort after 0.52 s, no frame in the following 3 s, reacquire |
| AC-005 | pass | fake: finite/indefinite streaming; hardware: 10 frames in 1.5 s, indefinite stream aborted after 20 frames |
| AC-006 | pass | fake: "Bridge failures and recovery", "Busy rejection and overlap" |
| AC-007 | pass | 120/120 definitions identical (old vs new generator, last run after the final generator change); `make -C indigo_test test-generator-architecture` all passed |
| AC-008 | pass | RPi `make -C indigo_linux_drivers/ccd_libcamera -f ../../Makefile.drv all` without warnings (libcamera); macOS same command (stub bridge) builds |

## Commands and results
- `python3 tools/run_driver_test.py ccd_libcamera` (macOS): 15/15 OK, recorded `2026-10-25 00:08 3.0.0.1 mac arm64 fake SDK 15/15 OK`.
- `python3 tools/run_driver_test.py ccd_libcamera --hw` (RPi): 10/10 OK, recorded `2026-10-25 00:04 3.0.0.1 linux arm64 ov5647 10/10 OK`, repeated in daylight at the owner's request and recorded `2026-10-25 08:23 3.0.0.1 linux arm64 ov5647 10/10 OK` (same source as commit 2fde3af2e; the later commit changes only test comments and records).
- `python3 tools/run_driver_test.py ccd_libcamera` (RPi): 15/15 OK, recorded `2026-10-25 00:08 3.0.0.1 linux arm64 fake SDK 15/15 OK`.
- `make -C indigo_test test-ccd-libcamera-sdk-sanitize` (macOS): 15/15, no ASan/UBSan report (driver object instrumented; libindigo not instrumented; leak detection off for the harness).
- All recorded runs above were repeated in full after the review fixes (defects 9-13); the two fake-SDK runs were recorded again at 00:08 after the SER-stream assertion was added to the test (test-only change, verified to fail with the stream finalization removed) on the fast-forwarded base `1cb0ab81c` plus the candidate; earlier recorded runs at 23:55/23:56 were replaced.
- `./.factory/scripts/check`, `./.factory/scripts/security`: exit 0. `./.factory/scripts/test`: exit 0, 188 tests. `./.factory/scripts/findings check`: valid.
- `python3 tools/make_test_summary.py`: TEST_SUMMARY.md regenerated with the three records.

## Intermediate runs (not acceptance evidence)
The first hardware runs failed 9/10 (defect 4), then exposed defects 5-7; each fix was followed by a full rerun of the hardware suite. Bridge behaviour was investigated with a scratch probe program on the RPi (`~/lcam_probe`, not part of the repository).

## Limitations
One camera model (OV5647, colour) on one board (Pi 3 B+, VC4 pipeline). Pi 5 (PiSP, 16-bit/compressed raw), mono sensors, multiple physical cameras and other libcamera pipelines are covered only by the fake bridge or not at all. The first recorded runs were made in a dark room (dark frames); a repeated recorded run on 2026-10-25 08:23 in daylight passed 10/10 (`2026-10-25 08:23 3.0.0.1 linux arm64 ov5647 10/10 OK`, 2 s exposure signal 687 vs 29 short). Windows/macOS runtime are not applicable (Linux-only driver). The RPi checkout's branch was created from `d25dcabf2`; the commits up to `1cb0ab81c` touch only `ccd_rpi`, its task records and the Xcode project, none of which the hardware run uses.
