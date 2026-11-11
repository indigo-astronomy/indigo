# Task brief

- Task ID: 20261110232507-0f97-installer-clean-build
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-10

## Problem and desired outcome
Owner request (2026-11-10): the Windows installer build script should always make a clean build.

## Scope and non-goals
- In scope: `tools/build_windows_installer.sh`.
- Non-goals: `SKIP_BUILD=1`, which keeps packaging an existing output folder.
