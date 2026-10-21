# Plan

1. Add indigo_test/integration/test_server_websocket.c covering AC-001..AC-003 against the production server on a loopback ephemeral port, plus an AddressSanitizer variant that compiles indigo_server_tcp.c into the test for AC-004.
2. Run both before the fix and record the expected failures.
3. In indigo_server_tcp.c, accept the key only when it leaves room for the GUID and NUL, otherwise answer 400 Bad Request and close.
4. Rebuild libindigo, rerun both tests, the isolated reproducer, the unit subset and the related server integration suite.
5. Register the files in the Xcode project, complete reviews, close FND-000045 with evidence, and commit once.
