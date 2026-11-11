# Specification

Status: approved by owner instruction ("fix the wrong points", 2026-11-11)

## Problem and outcome
The ToupTek SH20 profile of `indigo_mount_lx200` contradicts the ATLAS SH20 Developer Protocol V1.3 in nine places. After this task it follows V1.3 for each of them.

## Current-state audit (summary)
- **Driver.** The generated driver is `indigo_mount_lx200.driver` (version 83, `0x03000053`). The ToupTek profile is selected by `X_MOUNT_TYPE.TOUPTEK` or autodetected from `:GVP#` SH20.
  - The profile code is in `meade_init_touptek_mount()` and `meade_update_touptek_state()`.
  - It also has branches in the shared helpers: `meade_slew`, `meade_sync`, `meade_set_site`, `meade_set_guide_rate`, `meade_unpark`, `meade_home`, and the MOUNT_HOME, MOUNT_PARK and abort handlers.
- **Protocol notes and simulator.** `PROTOCOL.md` documented the TMount/INDI derived dialect. The host simulator `mount_lx200_simulator.c` (`touptek_dispatch`) modelled it.
- **Tests.** `test_mount_lx200_simulator.c` has 13 SH20 cases.
- **Defects against V1.3**, from the source audit against the PDF:

| ID | Defect |
| --- | --- |
| D1 | An accepted goto (`:MS#` → `1#`) was reported as "Slew failed" ALERT, and a rejected one (`0#`) as started. |
| D2 | The guide rate was sent as `:Sgr0.50#`; V1.3 takes `:Sgr0.n#`. |
| D3 | `X_TOUPTEK_MERIDIAN_LIMIT` sent offsets of −15…+15; V1.3 accepts `+00` only. |
| D4 | `X_TOUPTEK_MERIDIAN.TRACK_PASSED_MERIDIAN` was labelled ZWO-style; on the SH20 it means "resume tracking after the flip". |
| D5 | The site was sent with `.000` fractional seconds; V1.3 site format has whole seconds. |
| D6 | `MOUNT_HOME.SEARCH` sent `:MFH#`, which V1.3 does not have. |
| D7 | The home completion was read from `H` in `:GU#`; V1.3 says `:GHS#` 2. |
| D8 | `:MP0#` was sent without checking `:GPS#` = 2, and the unpark was never confirmed. |
| D9 | Comments and PROTOCOL.md described `e<code>#` errors, `:CM#` → `N/A#` and `:MS#` → `0#` as the protocol. |

- **Hardware test decision:** no hardware is available in this session. No hardware validation is claimed.

## In scope
D1–D9, plus the simulator, the tests and the documentation that encode them: `PROTOCOL.md` and `indigo_docs/PROPERTIES.md`.

## Out of scope
The V1.3 features the driver lacks (see brief.md). The driver README text about the Search item needs owner approval, so it is not changed by this task.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | A goto answered `1#` goes BUSY and ends OK without "Slew failed". A goto answered `0#` ends ALERT with "Goto rejected by the mount". An `e<code>#` reply still ends ALERT with the code. | `lx200_touptek_goto_sync_and_refusals` |
| AC-002 | A guide rate of 70 % is sent as `:Sgr0.7#` and never as `:Sgr0.70#`. | `lx200_touptek_rates_buzzer_and_meridian` |
| AC-003 | `X_TOUPTEK_MERIDIAN_LIMIT` no longer exists, and every `:STa` sends `+00`. A rejected `:STa` ends ALERT and shows the settings read back from the mount. | `lx200_touptek_rates_buzzer_and_meridian`, `lx200_touptek_connect_publishes_the_device_state` |
| AC-004 | `X_TOUPTEK_MERIDIAN` has the items `AUTO_FLIP` ("Flip automatically at the meridian") and `RESUME_TRACKING` ("Resume tracking after the flip"), read from `:GTa#` at connect. | same cases |
| AC-005 | The site is sent as `:St±DD*MM:SS#` and `:Sg±DDD*MM:SS#` with no fraction. | `lx200_touptek_site_and_time_signs` |
| AC-006 | `MOUNT_HOME` has two items (HOME, AWAY), and `:MFH#` is never sent. | `lx200_touptek_connect_publishes_the_device_state`, `lx200_touptek_home` |
| AC-007 | A home completes on `:GHS#` 2. `:GHS#` 0 shows the mount away even when `:GU#` reports `H`. A home that never starts ends ALERT. An aborted home ends OK and AWAY. | `lx200_touptek_home`, `lx200_home_touptek` |
| AC-008 | An unpark reads `:GPS#`, sends `:MP0#` only at `2#` and is confirmed by `:GPS#` 0. An unconfirmed unpark ends ALERT with "not confirmed" and the mount shown parked. | `lx200_touptek_park_unpark_and_park_position` |
| AC-009 | Comments, PROTOCOL.md and PROPERTIES.md describe V1.3. | review (quality-review.md) |
| AC-010 | The other lx200 profiles are not regressed. | the full `test_mount_lx200_simulator` suite |
| AC-011 | The driver version is raised from 83 to 84 (`0x03000054`). | generated `.c` |

## Failure behavior and edge cases
- **`:MS#` reply.** Any reply other than `1` fails the goto. An `e` reply reads the code, a `0` reply sends "Goto rejected by the mount", and the generic "Slew failed" follows in both cases.
- **Unpark:**
  - A failed `:GPS#` read fails the unpark.
  - `0#` is already unparked and succeeds without sending `:MP0#`.
  - Any reply other than `2#` (parking) refuses with a message.
  - After `:MP0#`, at most 10 `:GPS#` reads 100 ms apart run in the park handler; without a `0#` among them the unpark fails.
- **Home:**
  - `:GHS#` 1/3/4/5 marks the home as moving.
  - `0#` after the home was seen moving ends it with ALERT.
  - The existing 5 s start timeout is kept.

## Permissions, data, and security requirements
No change to the trust boundaries. Serial replies stay untrusted and are only compared character by character.

## Compatibility and nonfunctional requirements
SH20 firmware before V1.3, which answered `:MS#` with `0#` for success, will now report a successful goto as failed. This follows the V1.3 document the owner supplied.

The renamed items and the removed `X_TOUPTEK_MERIDIAN_LIMIT` change client-visible names. The SH20 profile has not been in a release yet: it is not in CHANGELOG.md.

## Assumptions and open questions
- Assumption, low risk: V1.3 firmware answers `:Sd`, `:Sr`, `:Te` and `:Td` with `1#`. The driver reads only the first character.
- Open, non-blocking: no hardware check of the `:MS#` polarity on firmware 0.5.2 has been run.

## Owner decisions and requirement changes
2026-11-11: the owner instructed "fix the wrong points" (D1–D9), following the analysis in this conversation.
