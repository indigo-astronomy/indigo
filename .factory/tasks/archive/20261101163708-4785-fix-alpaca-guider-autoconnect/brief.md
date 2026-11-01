# Task brief

- Task ID: 20261101163708-4785-fix-alpaca-guider-autoconnect
- Owner: Peter Polakovic
- Created: 2026-11-01

## Problem and desired outcome
With `indigo_agent_alpaca` loaded, connecting a mount by any client also connects its `<mount> (guider)` device, even with no Alpaca client present. The Guider Agent then refuses it with "'<mount> (guider)' is busy or in use and can not be selected." The agent must connect the guider only for an Alpaca client.

## Users and use cases
Users who keep the Alpaca Agent loaded and select a mount and its guider in the Mount and Guider agents. Forum report: https://bb.cloudmakers.eu/viewtopic.php?t=1413. Whether the reporter has the Alpaca Agent enabled is unconfirmed; this task fixes the reproduced defect, not that unknown.

## Scope and non-goals
Write scope: `indigo_drivers/agent_alpaca/`, `indigo_test/integration/test_agent_alpaca_mapping.c`, this task record.
Non-goals: A1, the misleading `auto://` connection message, the filter reselection race, README changes, forum replies.

## Constraints and references
Owner instruction 2026-11-01: "Yes, fix it" to connecting the guider only for an Alpaca client. Pairing was added by 234dcbcfe. The guider of `system_alpaca` attaches only while its mount connects, and that case must keep working.

## Initial questions
None blocking.
