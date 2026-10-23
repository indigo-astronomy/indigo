# Test plan

- New `make -C indigo_test test-server-websocket` (in INTEGRATION_TESTS) and `test-server-websocket-sanitize` (ASan/UBSan, real server source compiled in). Loopback ephemeral port only, private HOME, Bonjour off.
- Pre-fix baseline: the over-long and one-byte-too-long cases must fail (server answers 101), and the sanitize build must abort with a stack-buffer-overflow. RFC sample and 219-byte boundary cases must pass before and after.
- Post-fix: both binaries pass. Rerun the isolated reproducer against the old operations for reference, then the server-backed `test-detach-abort` suite and the unit subset as regressions.
- No hardware and no driver test mode is involved.
