# Specification

Status: approved

## Problem and outcome
A framework sync close to a celestial pole is accepted as before, and the client is warned that its alignment point can shift the right ascension of every other target.

## In scope
`indigo_mount_change_property()`, MOUNT_EQUATORIAL_COORDINATES with MOUNT_ON_COORDINATES_SET.SYNC, when the alignment point is added (SINGLE_POINT or NEAREST_POINT mode).

## Out of scope
Refusing or altering the sync; CONTROLLER mode, where drivers sync the controller themselves; slews; the alignment math.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A sync whose requested declination or raw declination (MOUNT_RAW_COORDINATES) is farther than 85 degrees from the equator, north or south, sends one BUSY-property message mentioning the celestial pole | unit test `sync_near_the_pole_is_accepted_with_a_warning` |
| AC-002 | The warned sync is still accepted: the alignment point is added exactly as before | same test, point count after every request |
| AC-003 | A sync at 85 degrees or closer to the equator sends no such message | same test, cases 60/60, 85/85, -85/-84 |
| AC-004 | The test fails when the warning is removed | temporary removal, recorded in verification.md |
| AC-005 | No other unit suite regresses | all unit suites run individually |

## Failure behavior and edge cases
The warning is sent after the coordinates update, so clients see the accepted sync first. The rejected paths (CONTROLLER mode, too many points) send no warning. Exactly 85 degrees is not warned.

## Permissions, data, and security requirements
No new input, permission or data path; the message text is constant.

## Compatibility and nonfunctional requirements
No property, API or ABI change; PROPERTIES.md is unaffected. All platforms: only `fabs()` and `indigo_send_message()` are used.

## Assumptions and open questions
Threshold 85 degrees: at 85 degrees an RA error of one hour moves the sync point by 1.3 degrees on the sky, yet shifts equatorial targets by 15 degrees. Low risk; adjustable later.

## Owner decisions and requirement changes
2026-11-12, Peter Polakovic (chat): add a warning through `indigo_send_message()` in `indigo_mount_driver` for a sync close to the pole. Warning only, no refusal.
