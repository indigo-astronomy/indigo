# Test plan

AC-001: synthetic simulator profiles with long application, long web and combined firmware; compare the first INDIGO_VALUE_SIZE - 1 bytes and subsequent serial metadata. Existing metadata covers ordinary versions.
AC-002: library peer golden frames for both setters, quote/backslash/control/UTF-8/empty values, maximum property sized control values, and oversized encoding refusal without a request; simulator public AP/STA updates inspect escaped wire frames.
AC-003: full focuser simulator suite and motion suite through tools/run_driver_test.py, generator reproducibility, Factory check/test/security and committed process check. Baseline runs precede production edits. Synthetic credentials only.
