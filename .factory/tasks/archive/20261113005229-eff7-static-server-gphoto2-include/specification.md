# Specification

Status: approved by owner request 2026-11-13

## Problem and outcome
See brief.md.

## Current-state audit
`indigo_server.c` lines 223-225 include the obsoleted driver header and lines 278-280 list `indigo_ccd_gphoto2` in `static_drivers[]`, both under `INDIGO_LINUX`. No other maintained file refers to the driver.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `indigo_server.c` compiles with `STATIC_DRIVERS` on Linux | `gcc -fsyntax-only -DSTATIC_DRIVERS`, before and after |
| AC-002 | The ordinary (dynamic) server build is unchanged | `gcc -fsyntax-only` without `STATIC_DRIVERS`; the removed lines are inside `#ifdef STATIC_DRIVERS` |

## Failure behavior and edge cases
None.

## Permissions, data, and security requirements
None.

## Compatibility and nonfunctional requirements
Static builds no longer offer the obsoleted gphoto2 driver, which they could not build anyway.

## Assumptions and open questions
None.

## Owner decisions and requirement changes
None.
