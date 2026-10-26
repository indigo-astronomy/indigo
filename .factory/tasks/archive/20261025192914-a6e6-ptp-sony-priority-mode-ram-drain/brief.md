# Task brief

- Task ID: 20261025192914-a6e6-ptp-sony-priority-mode-ram-drain
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
Owner request (2026-10-25): verify on the ILCE-7RM4 two follow-ups from the Sony open-source research note (`.factory/tasks/archive/20261025134403-8427-fix-sony-capture-deadline/sony-open-source-research.md`, candidates 1 and 2): writing PriorityMode 0xD25A = 1 after SDIOConnect(3), and draining leftover objects at 0xffffc001 before the press.

## Users and use cases
Clients of NEW_API Sony bodies that reconnect, and every Sony body after an interrupted capture.

## Scope and non-goals
In scope: `indigo_drivers/ccd_ptp/` Sony path, `indigo_test/hardware/test_ccd_ptp_hw.c`, `indigo_test/integration/test_ccd_ptp_sdk.c`, fake build flags, records. Non-goals: measuring the effect of PriorityMode on ignored presses (owner decided to write it without measuring), other vendors, hot-plug.

## Constraints and references
Non-interactive hardware mode; conserve the camera battery (small targeted probes, one recorded run after review).

## Initial questions
None.
