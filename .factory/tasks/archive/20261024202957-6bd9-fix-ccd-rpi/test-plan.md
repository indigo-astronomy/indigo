# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | make in indigo_optional_drivers/ccd_rpi | libcamera 0.7.2 | builds | after implementation |
| T-002 | AC-002, AC-005 | rpi-hw-checks.sh | RPi 3B+ with OV5647, test server on port 7700 | all checks PASS | after implementation and after review fixes |
| T-003 | AC-003 | rpi-hw-checks.sh plus abort sweeps and back-to-back exposures (verification.md) | same | ALERT after abort/disconnect, ReadFrame released, 10/10 OK | same |
| T-004 | AC-004, AC-006 | code review and connect cycles | same | review pass, three cycles OK | same |

Autofocus, monochrome and RAW paths have no hardware here; they are review-only.
