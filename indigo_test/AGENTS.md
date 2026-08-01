# Automated Test Guidelines

## Scope

This directory contains the automated, hardware-free INDIGO test suite. Keep this guide focused on tests only. General repository coding rules, driver conventions, and build-system guidance live in the repository root `AGENTS.md`.

## Test Layout

- `Makefile` defines all automated test targets.
- `test_runner.h` provides the local dependency-free C test runner and assertion macros.
- `unit/` contains fast deterministic tests for library helpers, property helpers, protocol adapters/parsers, tokens, RAW helpers, math helpers, and similar code.
- `integration/` contains in-process bus and simulator-driver tests.
- `integration/simulator_test_common.h` contains shared simulator lifecycle helpers, the in-process client, property cache, wait helpers, and compliance-style assertions.
- `fixtures/protocol/` contains fixed XML and JSON protocol parser fixtures.
- `CHANGES.md` summarizes current coverage and deferred automated-test work.

## Running Tests

Run `make all` from the repository root first if `build/lib/libindigo` or the required simulator driver archives are missing.

- `make -C indigo_test test` runs all automated unit and integration tests.
- `make -C indigo_test test-unit` runs unit tests only.
- `make -C indigo_test test-integration` runs bus and simulator-driver integration tests only.
- `make -C indigo_test test-clean` removes generated `indigo_test/build` binaries and dSYM files.

After validating changes that build tests, run `make -C indigo_test test-clean` unless the user asks to keep build artifacts.

## Harness Conventions

- Use `test_runner.h` for new C tests.
- Keep tests dependency-free unless a dependency is explicitly approved and compatible with the project license.
- Each test executable must return `0` on success and non-zero on failure.
- Prefer small named test functions registered in the `indigo_test_case tests[]` array.
- Use the existing assertion macros, adding new macros only when they improve clarity across multiple tests.
- Print enough context before assertions when a helper failure would otherwise hide the property, item, fixture, or device under test.
- Avoid long sleeps. Use bounded polling helpers for asynchronous bus or simulator behavior.
- Never change any file outside indigo_test folder.

## Unit Test Rules

- Unit tests should be deterministic and hardware-free.
- Link against the built INDIGO library instead of including production `.c` files directly.
- Use fixed expected values and stable fixtures.
- Keep protocol tests fixture-driven when parsing serialized input.
- Put new protocol fixtures under `fixtures/protocol/` and keep them minimal.
- Avoid environment-dependent tests unless the environment is fully controlled by the test.

## Integration Test Rules

- Integration tests must stay hardware-free.
- Exercise public bus APIs and public driver entry points.
- Do not launch `indigo_server` in the normal integration target.
- Do not open network sockets in the normal integration target.
- Simulator tests should validate driver metadata, `INDIGO_DRIVER_INIT`, property enumeration, connection/disconnection, representative property changes, and `INDIGO_DRIVER_SHUTDOWN`.
- Prefer one integration executable per simulator driver archive.
- For multi-device simulator drivers, cover each exposed logical device in the relevant simulator test file.
- Always cleanly disconnect simulator devices before shutdown.

## Simulator Compliance

- Use `simulator_test_common.h` for simulator compliance checks.
- Use `DRIVER_TESTING_RULES.md` for all simulator devices.

## Makefile Rules

- Add new unit executables to `UNIT_TESTS`.
- Add new integration executables to `INTEGRATION_TESTS`.
- Keep test-specific linker flags local and explicit.
- Do not add generated binaries or `build/` artifacts to the repository.
- Keep `test-clean` able to remove all generated test outputs.

## Documentation

- Update `CHANGES.md` when adding meaningful new test coverage or deferring known work.
- Update `DRIVER_TESTING_RULES.md` when base driver property visibility, simulator compliance coverage, or driver-class test scenarios change.
