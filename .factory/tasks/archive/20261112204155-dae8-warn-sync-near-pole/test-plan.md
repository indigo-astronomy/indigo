# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance / preserved contract | Layer and test path | Setup | Expected result | Timing |
| --- | --- | --- | --- | --- | --- |
| T-001 | AC-001, AC-002 | unit, `indigo_test/unit/test_mount_alignment_load.c` `sync_near_the_pole_is_accepted_with_a_warning` | NEAREST_POINT, SYNC; target/raw Dec 90/90, 6/89.5, 86/40, -89/-89 | one warning each, point added | after implementation |
| T-002 | AC-003, AC-002 | same test | 60/60, 85/85, -85/-84 | no warning, point added | after implementation |
| T-003 | AC-004 | same test with the condition disabled | temporary edit, restored | test fails | after implementation |
| T-004 | AC-005 | every unit suite run individually | built library | no new failure | after implementation |

## Baseline evidence
`test_protocol_security` fails on master without the task diff; see verification.md.

## Bug reproduction
T-003 shows that without the warning no message reaches the client.

## Negative cases and permission boundaries
T-002 covers the threshold boundary and the southern hemisphere below it.

## Isolation and cleanup
The test uses the private test home of `test_runner.h` and detaches its client, device and bus.

## Deferred tests and coverage limitations
No hardware or integration run; the message is not visible in any client UI test.
