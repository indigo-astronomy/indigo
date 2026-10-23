# Specification

FND-000047 reports that /dev/null produces ENODEV on macOS rather than ENOTTY, making the exact non-serial error assertion fail and preventing subsequent unit suites from running.

- AC-001: Opening an isolated regular file through indigo_uni_open_serial2 fails with a NULL handle and exactly "Not a serial port" on the available POSIX host; preserve the same expectation for Linux.
- AC-002: The fixture uses a unique temporary pathname, closes all descriptors/handles and removes its file before contract assertions can return from the test.
- AC-003: The complete test-unit target succeeds, including suites after test_uni_io.

Production APIs, error mappings, properties and drivers remain unchanged. Complexity: low, with one public API and existing test/build infrastructure; risk: low, confined to fixture setup/cleanup. Uncertainty: Linux execution is unavailable and must not be claimed. No unresolved product questions.
