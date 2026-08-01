# indigo_tests Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Baseline review complete; open findings recorded. |

## Scope

Legacy shell-based compliance testing routines under `indigo_tests/`.

Reviewed files:

- `INDIGO_TEST_FRAMEWORK.md`
- `indigo_test_framework.sh`
- `ao_compliance.sh`
- `filter_wheel_compliance.sh`
- `focuser_compliance.sh`
- `gps_compliance.sh`
- `guider_compliance.sh`
- `rotator_compliance.sh`

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| TESTS-001 | High | `gps_compliance.sh:67`, `gps_compliance.sh:79`, `gps_compliance.sh:85`, `gps_compliance.sh:189` | The GPS script labels `UTC_TIME`, `GPS_STATUS`, and `GPS_ADVANCED` as optional, but tests them with `test_property_exists()`, which records failures when absent. The script also prints the summary without exiting based on `TESTS_FAILED`, so failed GPS compliance can still return success. Treat optional properties as skips when absent and add the same final `if [ $TESTS_FAILED -eq 0 ]; then exit 0; else exit 1; fi` block used by the other scripts. | Open |
| TESTS-002 | Medium | `indigo_test_framework.sh:104`, `indigo_test_framework.sh:170`, `indigo_test_framework.sh:239`, `indigo_test_framework.sh:396`, `indigo_test_framework.sh:479`, `indigo_test_framework.sh:507`, `indigo_test_framework.sh:576`, `indigo_test_framework.sh:726`, `indigo_test_framework.sh:754`, `indigo_test_framework.sh:790` | Framework commands expand `$INDIGO_PROP_TOOL` and `$REMOTE_SERVER` unquoted and usually pipe output through `grep` without `pipefail`. Tool paths containing spaces break, remote arguments cannot be represented safely as an argv array, and command failures can be hidden when `grep` matches text from stderr. Store remote options in an array, invoke `"${INDIGO_PROP_TOOL}"`, and capture command status before filtering output. | Open |
| TESTS-003 | Medium | `ao_compliance.sh:53`, `ao_compliance.sh:133`, `filter_wheel_compliance.sh:70`, `filter_wheel_compliance.sh:196`, `focuser_compliance.sh:53`, `focuser_compliance.sh:377`, `guider_compliance.sh:53`, `guider_compliance.sh:207`, `rotator_compliance.sh:53`, `rotator_compliance.sh:438` | Most compliance scripts save `WAS_CONNECTED` from `test_connection_battery()` and then ignore it, always disconnecting the target device at the end. Manual runs against already-connected hardware will leave devices disconnected and may disrupt an active setup. Restore the original connection state like `gps_compliance.sh` attempts to do, and fix the framework comment at `indigo_test_framework.sh:527` because the function returns original state, not final state. | Open |
| TESTS-004 | Medium | `focuser_compliance.sh:199`, `focuser_compliance.sh:201`, `focuser_compliance.sh:206`, `rotator_compliance.sh:231`, `rotator_compliance.sh:233`, `rotator_compliance.sh:238` | Abort-motion tests assume a long-running move is still `BUSY` after a fixed two-second sleep. Fast devices can finish before abort is sent and fail the required `BUSY -> OK` abort transition even though the driver is valid; slow devices can also leave a background `indigo_prop_tool` process running in the rotator test. Use a generated long move known to remain busy, wait until the motion property reports `BUSY`, send abort immediately, and wait/reap any background helper process. | Open |
| TESTS-005 | Medium | `indigo_test_framework.sh:720`, `indigo_test_framework.sh:726`, `indigo_test_framework.sh:737`, `indigo_test_framework.sh:765`, `indigo_test_framework.sh:801` | Property and range parsing is based on unanchored regular expressions over human-readable `indigo_prop_tool` output. Item checks can match substrings from unrelated items, and range extraction takes the first bracketed expression from all output instead of the requested item line. Parse exact item names from structured tool output if available, or filter to the exact property item with fixed-string matching before extracting state, values, and ranges. | Open |
| TESTS-006 | Low | `INDIGO_TEST_FRAMEWORK.md:47`, `INDIGO_TEST_FRAMEWORK.md:592`, `indigo_test_framework.sh:71`, `indigo_test_framework.sh:72`, `indigo_test_framework.sh:75` | Framework documentation is out of sync with the implementation: `print_test_summary()` now counts skipped tests, but the early example and prose still describe only passed/failed counts. Update the documentation so manual test operators understand skipped optional properties and do not mistake skips for missing coverage. | Open |

## Review Focus

- Driver-class compliance assumptions and property expectations.
- Shell portability and dependency assumptions.
- Clear failure diagnostics and reliable exit codes.
- Deterministic waits for asynchronous device behavior.
- Preservation of hardware state after manual compliance runs.
- Consistency with automated simulator tests under `indigo_test/`.
- Opportunities to migrate stable behavior into hardware-free unit or simulator integration tests.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Baseline review of legacy compliance framework, scripts, and framework documentation. |
