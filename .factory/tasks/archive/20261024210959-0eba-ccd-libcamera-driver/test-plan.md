# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | integration `test_ccd_libcamera_sdk` "Discovery names and duplicates", "Discovery capacity", "No cameras and unavailable library"; hardware "Discovery and identity" | fake bridge profiles; OV5647 | one device per camera named after the sensor, `#n` suffix for duplicates, capacity capped | after implementation |
| T-002 | AC-002 | "Metadata and property contract", "Capability profile without gain", "Open failure and recovery"; hardware "Discovery and identity", "Reconnect" | fake; OV5647 | CCD_INFO/CCD_MODE/limits published, open failure ALERT with no resources held | after implementation |
| T-003 | AC-003 | "Exposure and image contract", "Colour subframe, mode and Bayer", "Mono subframe without Bayer"; hardware "Every mode full frame", "Subframe", "Long exposure", "Gain control" | fake deterministic pattern; OV5647 | exact arguments, payload, geometry, BAYERPAT, per-mode limits | after implementation |
| T-004 | AC-004 | "Abort and reacquire", "Disconnect during exposure and shutdown"; hardware "Abort and reacquire" | fake hold gate; OV5647 | no late frame, reacquire OK, no calls after close | after implementation |
| T-005 | AC-005 | "Finite and aborted streaming"; hardware "Finite streaming", "Indefinite streaming, refusals and abort" | fake; OV5647 | exact count, abort stops frames | after implementation |
| T-006 | AC-006 | "Bridge failures and recovery", "Busy rejection and overlap" | fake fault flags | ALERT, no BUSY left, recovery | after implementation |
| T-007 | AC-007 | 120-definition regeneration comparison; `test_generator_architecture` "SDK discovery without USB" | old vs new generator | identical output; new path emits no libusb; invalid combinations refused | before driver work |
| T-008 | AC-008 | build on RPi with libcamera; build on macOS without libcamera (stub) | RPi, macOS | both build | after implementation |

## Baseline evidence
New driver: no previous driver baseline. Generator baseline: all 120 tracked `.driver` files generated with the unmodified generator (`orig.c` copy of `d25dcabf2`) as reference output.

## Bug reproduction
Hardware-found defects 4-7 (REFACTOR.md) were reproduced by the hardware suite before their fixes (9/10 failing, then timing/limit observations) and are guarded by hardware assertions; defect 5 is also modelled in the fake profile. Defects inside the C++ bridge cannot be reproduced through the fake bridge, which replaces it; this is recorded in REFACTOR.md.

## Negative cases and permission boundaries
Bridge start/read failures, malformed frame geometry, readout timeout, open failure, rejected changes while BUSY, rejected SHUTDOWN while connected, invalid generator definitions.

## Isolation and cleanup
Tests use `indigo_test_use_private_home()`; fake state reset per case; hardware runs only on the explicitly assigned RPi. `make -C indigo_test test-clean` after validation.
