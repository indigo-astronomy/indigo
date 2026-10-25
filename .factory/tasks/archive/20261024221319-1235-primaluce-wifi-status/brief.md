# Task brief

- Task ID: 20261024221319-1235-primaluce-wifi-status
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-25

## Problem and desired outcome
The requester asked how to find the IP address to connect to a PrimaLuce device over WiFi, then asked to improve it
(2026-10-25): the drivers show the address the controller reports.

## Users and use cases
Users who connect once over USB, read the address and switch the port to `http://<address>`.

## Scope and non-goals
In scope: library status parsing, read-only `X_WIFI_STATUS` on GIOTTO, ALTO and the focuser, simulators, tests, docs.
Non-goals: discovery of devices on the network, mDNS.

## Constraints and references
Root, driver and test AGENTS.md; PROTOCOL.md; hardware captures of the SESTO SENSO 3, GIOTTO and ALTO1 full state.

## Initial questions
None.
