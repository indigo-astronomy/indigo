# Task brief

- Task ID: 20261025082424-64fe-readme-driver-list
- Owner: Peter Polakovic
- Created: 2026-10-25

## Problem and desired outcome
The root README does not mention the optional libcamera build dependency of the new `ccd_libcamera` driver, and its driver list misses many existing drivers. The owner approved editing README.md (chat, 2026-10-25) and asked to check the list for further missing drivers.

## Users and use cases
People building INDIGO from source and reading the supported-driver list.

## Scope and non-goals
README.md only: optional libcamera prerequisite for Debian/Raspberry Pi OS and Fedora, missing drivers appended to the driver list. Non-goals: renumbering or rewording existing entries, agents (the list never contained them), drivers excluded from build/install (`ccd_pentax`, `system_alpaca`), packaging dependencies.

## Constraints and references
Root AGENTS.md README approval rule; driver directories under indigo_drivers, indigo_linux_drivers, indigo_mac_drivers, indigo_optional_drivers.

## Initial questions
None.
