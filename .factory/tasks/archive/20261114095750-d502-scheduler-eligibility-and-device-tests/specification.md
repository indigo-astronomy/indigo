# Specification

Status: approved by owner request and answers 2026-11-14

## Problem and outcome
See brief.md.

## Current-state audit
Integration covered below-horizon, window not open, window too short, "no time left" in the plan. Not covered end to end: altitude, airmass, hour angle (wrapped), twilight, moon separation (Lorentzian) and illumination, nightly windows, duration against setting, unknown site, @enabled no, score terms, text/count limits through the agent, repeat/retry pauses. The server acceptance test had no filters, frame types or pointing checks and no device-step failures.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Each sky and window constraint: a task that waits with the documented reason and one that runs; the waiting task starts when the site moves where applicable | integration cases altitude_limit_decides_and_follows_the_sky, airmass_limit_decides_and_follows_the_sky, hour_angle_ranges_including_wrapped, twilight_limit_follows_the_sun, moon_separation_and_brightness, nightly_windows, duration_must_fit_before_the_target_sets, unknown_site_blocks_only_sky_constraints, disabled_in_the_header_stays_disabled |
| AC-002 | Score: setting term outweighs a level difference of 20, level decides without it, list order without both, altitude term prefers the higher target | sky_bonuses_order_equal_priorities |
| AC-003 | Limits: 16 KiB text stored, restored and run, 16385 bytes rejected; 128 tasks, 129th and an urgent task rejected, a delete frees a place | largest_task_text_is_stored_and_restored, task_count_limit |
| AC-004 | Repeat and retry pauses of one minute observed | repeat_interval_and_retry_delay |
| AC-005 | Device tasks: LRGB with named filters, FILTER/IMAGETYP/EXPTIME/OBJCTRA/OBJCTDEC, dark and bias; priority order with pointing per frame and a repeated task; missing filter and camera fail with one retry, no frames, next task runs | server acceptance cases 9-11 |
| AC-006 | START right after uploads starts the highest-priority task (defect 1 fixed) | targets_in_priority_order (device), start_waits_for_pending_estimates (guard) |
| AC-007 | Docs, version 0x03000009, MIGRATION_STATUS 71 / 0, finding for defect 2 | review, recorded run, findings check |

## Failure behavior and edge cases
See criteria.

## Permissions, data, and security requirements
None new.

## Compatibility and nonfunctional requirements
The fix may delay an ordinary start by the duration of pending estimates, bounded by the estimate timeout; urgent tasks are not delayed.

## Assumptions and open questions
The CCD simulator records a zero exposure as 0.1 s; the bias check allows it.

## Owner decisions and requirement changes
- 2026-11-14: report and wait for defects; defect 1 fixed; defect 2 recorded as a finding.
