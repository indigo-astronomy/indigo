# Test and regression plan

Status: final

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | manual hardware check | SESTO SENSO 3 and an Arduino Micro attached | `auto:///dev/ttyUSB0` selected | done |

## Baseline evidence
Not applicable.

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
The Arduino Micro on `/dev/ttyACM0` is the non-matching port.

## Isolation and cleanup
The server was stopped after the check.
