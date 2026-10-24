# Verification report

Verdict: pass
Scope note: hardware is a Raspberry Pi 3B+ with an OV5647 colour sensor without autofocus; untested paths are listed below
Reviewer: implementing agent (Claude Code, claude-opus-5-5); independent diff review recorded in quality-review.md
Date: 2026-10-24
Revision and working-tree state: base 923bca9a214d4b5d9ba2982e2964e982f26c007a on branch task/20261024202957-6bd9-fix-ccd-rpi, uncommitted changes to indigo_optional_drivers/ccd_rpi/Makefile and indigo_ccd_rpi.cpp
Review independence: checks were designed and run by the implementer.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| Builds against libcamera 0.7.2 | pass | `make` in indigo_optional_drivers/ccd_rpi, no errors; one pre-existing -Wsign-compare warning in the unchanged CCD_BIN handler |
| Exposures complete (libcamera 0.7 request controls) | pass | rpi-hw-checks.sh: all exposures OK, no "Overwriting Request::controls()" errors |
| Full sensor resolution | pass | CCD_INFO 2592x1944 (was 2560x1600, cropped to 16:10); FITS 2592x1944x3 |
| Binning 1-4 and CCD_BIN to CCD_MODE mapping | pass | BIN 2 selects BGR24_2, image 1296x972 |
| Subframe (ScalerCrop) | pass | 800x600 at 200,100 gives 800x600; with BIN 2 gives 400x300; CCD_FRAME with BITS_PER_PIXEL=24 is accepted (OK) |
| RGB24 / BGR24 channel order | pass | both modes give the same channel ordering of plane means (R > G > B on the same scene) |
| X_CCD_ADVANCED applied to every exposure | pass | SATURATION=0 gives grey images (R-G difference < 0.3) on two consecutive exposures |
| Countdown | pass | 3 s exposure reports 3, 2, 1, 0 then OK |
| Abort mid-exposure | pass | abort after 1 s: ALERT, no late OK; next exposure OK |
| Abort while ReadFrame waits (FND-000021) | pass | 1 ms exposures aborted after 0.15-0.25 s: log shows `ReadFrame -> -1` within 1-4 ms of StopCamera, state ALERT |
| Disconnect during exposure | pass | "Exposure aborted by disconnect", reconnect leaves CCD_EXPOSURE not BUSY, next exposure OK |
| Connect cycles (FND-000026) | pass | three disconnect/connect cycles, X_CCD_ADVANCED defined each time |
| Back-to-back exposures | pass | 10 consecutive 0.01 s exposures with `-w OK`, all OK, no timer reference errors |
| Server shutdown | pass | SIGINT with camera connected: server exits (previous driver build had to be killed) |
| Autofocus, lens position, focuser name, monochrome, RAW | not verified | no such hardware; source review only |
| Frame timeout path | not verified | requires a camera that drops a frame |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make` | indigo_optional_drivers/ccd_rpi, gcc 14, libcamera 0.7.2+rpt20261009 | 0 | builds |
| `build/bin/indigo_server -p 7700 -b- -u- -vv build/drivers/indigo_ccd_rpi` | repository build | - | test server, stopped with SIGINT |
| `RPI_TEST_DIR=<dir> INDIGO_BIN=build/bin bash rpi-hw-checks.sh` | OV5647 on CSI | 0 | `RESULT PASS` on the final build after the review fixes, and earlier after test-timing corrections (prop_tool `-t 0` sends nothing; background sends used for abort and disconnect cases) |
| abort timing sweeps with 1 s and 1 ms exposures | same server | - | see Acceptance evidence |

`python3 tools/run_driver_test.py ccd_rpi` was not run: the driver has no automated tests and no simulator.

## Baseline comparison and regressions
Before the change the driver did not build against libcamera 0.7.2. With only the build fix, connection worked and every exposure failed with "Can't queue request". The old driver also hung on server shutdown while connected.

## Manual checks and limitations
- The first exposure after a server start can be dark with CCD_GAIN 0 (automatic gain): each exposure restarts the camera with a single request, so the gain control has no frames to converge on. Not changed.
- Autofocus cannot complete with one request per exposure (quality-review.md finding 3).
- `rpicam-hello` from rpicam-apps 1.13.0 fails after the libcamera-dev install upgraded libcamera0.7 to rpt20261009 (`undefined symbol ... Span`); rpicam-apps 1.14.0 is available. System package state, unrelated to the driver.
- The OV5647 exposes no LensPosition/AfMode controls, no monochrome formats and no RAW modes in this driver.

## Register reconciliation
FND-000021 to FND-000027 and FND-20261024121920-3bd2-repository-audit-030 are addressed in source; hardware evidence above covers FND-000021, 000022 (exposure start failure path by review only), 000025, 000026 and audit-030 (attach/detach on every server start). FND-000023, 000024 and 000027 are verified by review only.
