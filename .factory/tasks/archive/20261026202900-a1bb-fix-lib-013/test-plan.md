# Test plan

- AC-001: Gate a synthetic attach callback, start a concurrent enumeration and record any enumeration before initialization. Use condition-variable handshakes and bounded waits; record scheduling limitations honestly. Run before and after fix.
- AC-002: Publish a synthetic property from inside attach and observe its client callback. Bound deadlocks with a process watchdog.
- AC-003: A synthetic CONFIG.LOAD callback asks another thread to enumerate and requires its completion before returning. This detects holding the mutex past attach, without real config files.
- AC-004: Preserve and rerun original bus lifecycle cases and native unit suite.
- AC-005: Run complete rotator_lunatico and focuser_lunatico hardware-free suites using tools/run_driver_test.py, with no filters. Record README testing sections and TEST_SUMMARY. No hardware, Windows or Linux execution.

All worker threads are joined and lifecycle resources cleaned before assertions. Tests use synthetic devices/private configuration only. Test designer is separate from production implementer; runtime verification is performed by the coordinator (same-agent limitation recorded).
