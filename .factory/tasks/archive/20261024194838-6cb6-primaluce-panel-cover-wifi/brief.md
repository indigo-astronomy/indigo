# Task brief

- Task ID: 20261024194838-6cb6-primaluce-panel-cover-wifi
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-24

## Problem and desired outcome
The GIOTTO and ALTO have WiFi, but their drivers could not configure it (left out of their first specification). The
requester asked to look into it (2026-10-24) and then to proceed with the proposal: the focuser's `X_WIFI`, `X_WIFI_AP`
and `X_WIFI_STA` on both drivers, the mode selection shared in `primalucelib`, with a hardware check.

## Users and use cases
Users switching a panel or cover between its own access point and an observatory network, or changing credentials.

## Scope and non-goals
In scope: library WiFi selection and credential checks, the three properties on GIOTTO and ALTO, the focuser moved to
the shared code, simulators, tests, documentation. Non-goals: escaping credentials (refused instead), HTTP-side tests.

## Constraints and references
Root, driver and test AGENTS.md; `indigo_drivers/shared/primalucelib/PROTOCOL.md`; focuser REFACTOR.md D8.

## Initial questions
None.
