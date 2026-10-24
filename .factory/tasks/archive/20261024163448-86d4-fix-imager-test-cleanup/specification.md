# Specification

- AC-001: All started agent work reaches a bounded terminal state and connected simulator devices disconnect before their driver is shut down.
- AC-002: Every shutdown, observer detach and bus stop result is checked; cleanup continues after a failed stage and makes the test process fail.
- AC-003: A case pass record is emitted only after setup, assertions and cleanup succeed. A failed teardown produces a fail record with no pass for that case.
- AC-004: Deterministic fault injection proves failure propagation without suppressing real resource release; full normal imager suite passes with no attached-device bus-stop warning.
- AC-005: Preserve existing behavior/assertions, private HOME, per-case process isolation, watchdog and accurate selected-case planning.

Complexity: medium (ordered asynchronous stop/disconnect and test-result lifecycle). Risk: low; test-only change with synthetic simulator resources. No public API/property/dependency change or product ambiguity. Hardware is excluded.
