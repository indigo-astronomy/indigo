# Task brief

- Task ID: 20261025004416-0995-xml-client-named-request-protocol
- Owner: Rumen G. Bogdanovski (requester)
- Created: 2026-10-25

## Problem and desired outcome
`indigo_prop_tool get_state` printed OK for a property the driver published IDLE (`X_WIFI_STATUS` of
`indigo_aux_giotto` 3.0.0.7). The requester asked to fix the tool (2026-10-25).

## Users and use cases
`indigo_prop_tool` and any XML client whose first request names a device or a property.

## Scope and non-goals
In scope: the XML client's request (`indigo_libs/indigo_xml.c`), a unit case, PROTOCOLS.md. Non-goals: the legacy
`Idle` reading for INDI servers, the server side.

## Constraints and references
Root AGENTS.md; `indigo_docs/PROTOCOLS.md`.

## Initial questions
None.
