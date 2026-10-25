# Test and regression plan

Status: executed

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001 | manual review, `apt-cache policy libcamera-dev` on the RPi | Debian 13 | package exists | after edit |
| T-002 | AC-002 | directory-to-entry comparison | all driver roots | no built driver missing | after edit |

## Baseline evidence
Before the edit 30 built drivers had no entry (verification.md).

## Bug reproduction
Not applicable.

## Negative cases and permission boundaries
Not applicable (documentation only).

## Isolation and cleanup
No build or test artifacts.
