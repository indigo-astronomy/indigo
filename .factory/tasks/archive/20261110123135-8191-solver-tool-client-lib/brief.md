# Task brief

- Task ID: 20261110123135-8191-solver-tool-client-lib
- Owner: Rumen G. Bogdanovski
- Created: 2026-11-10

## Problem and desired outcome
Owner request (2026-11-10): "make indigo_solver link against libindigoclient". The command line solver linked the
full libindigo (libusb, libudev, driver framework) although it is a standalone tool.

## Users and use cases
Users of the `indigo_solver` command line tool on Linux, macOS and Windows.

## Scope and non-goals
- Owner decisions (2026-11-10): the framework modules the tool needs beyond the client library are built into the
  tool, the client library itself stays as it is on Linux/macOS; Windows switches too, including the repair of the
  Windows client library project it needs.
- Delivered scope: the Windows client library repair (project, solution entry, installer) only.
- Owner decision (2026-11-10, after review): commit the Windows client-library repair on its own; the solver
  build changes (Makefile, `indigo_solver.vcxproj`) were reverted and move to a separate task that builds
  `indigo_solver` from INDIGO sources without a client library dependency.
- Non-goals: other tools, changes to the client library sources or the Linux/macOS client library contents.

## Constraints and references
Root AGENTS.md (Xcode project only for new persistent files; none added), Windows not buildable on this host.

## Initial questions
Resolved with the owner, see above.
