# Specification

Status: approved by owner request (2026-10-25)

## Problem and outcome
README.md documents how to build the libcamera driver and lists every built driver.

## In scope
Optional libcamera prerequisite; driver list completed against the driver directories.

## Out of scope
Existing entries (including the duplicate CloudWatcher/Lunatico items), agents, excluded drivers, packaging.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | Debian and Fedora prerequisites name the optional libcamera development package and the effect of its absence. | review; `apt-cache policy libcamera-dev` on Debian 13 |
| AC-002 | Every driver directory that is built and installed has an entry in the driver list. | directory-to-entry comparison |

## Failure behavior and edge cases
Not applicable (documentation).

## Permissions, data, and security requirements
No sensitive data.

## Compatibility and nonfunctional requirements
Existing numbering kept; new entries appended.

## Assumptions and open questions
Fedora package name `libcamera-devel` (standard Fedora naming); not verified on a Fedora host.

## Owner decisions and requirement changes
2026-10-25: owner approved the README change and asked for the completeness check (chat).
