# Test plan

- Fake USB: strict packed/nonlegacy and ordered legacy framing, fixed ISO parameters, selected ISO on connection, delayed setting readback, original physical B with0/0 shutter, command/status faults, capture/abort/recovery and ownership cleanup.
- Timing: inject100ms argument staging and200ms trigger readiness; request1.2s and assert trigger-to-close interval within±30ms. Inject readiness longer than request and assert prompt close after acknowledgement. Measure dispatch endpoints only; no physical curtain accuracy claim.
- Run normal and ASan/UBSan USB suites on macOS and canonical fake suite on Linux arm64. Run25 recorder fixtures, Factory checks/tests/security, diff whitespace and committed process gate after latest master synchronization.
- Hardware evidence obtained before final offline timing repair: M controls/timed JPEG/abort/recovery, B JPEG/abort/recovery, one DNG and one PEF decoded with LibRaw. Keep full-class failure distinct from partial profiles. Do not reopen camera for final timing repair.
- Inspect retained logs for native payload dumps; keep image bytes outside Git. Verify all source identities after final review and archive before commit.
