# Specification

## Behavior
For each finding listed in [brief.md](brief.md), the defective behavior described in its registry record no longer occurs:
- AC-1: device replies, files and buffers are validated before use (exact read/write lengths, allocated capacities, in-bounds pixel access, terminator capacity), and failures publish ALERT or return failure instead of success.
- AC-2: long-running device operations end within a bounded monotonic deadline, using handler + `_finalizer`, stop the device where the protocol allows and publish ALERT on expiry; abort and disconnect stay serviceable.
- AC-3: library helpers honour their contracts (registry capacity, slave query bound, session per-client features, ASSERT_NEAR finite semantics, proper motion, cartesian conversion, MULTI_POINT translations assign outputs).
- AC-4: server and browser fixes: static UNLOAD followed by DRIVERS enable reinitializes; sexagesimal sign below one degree; fractional-hour HOST_TIME offset; gzip only served when complete.
- AC-5: existing behavior and property shapes are preserved; no property added or removed; changed drivers have a higher version.

## Verification
Each AC is verified by a hardware-free regression that fails before the fix where a harness exists; otherwise by compile/inspection with the gap stated (see test-plan.md).
