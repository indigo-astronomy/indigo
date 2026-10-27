# Test plan and scope

Software acceptance: compare original OAKCL acquisition, compile the actual Darwin auto-claim wrapper with failure injection, verify ownership success/failure and no duplicate reattachment, release/cleanup guards, and existing public capture regressions. Run canonical `python3 tools/run_driver_test.py ccd_pentax`, ASan/UBSan, native builds and configured Factory checks. Validate source dependency selection by forcing the Darwin prerequisite newer in a make dry run.

Task diagnostics before repair: one grouped hardware harness and one actual sudo-server connection/status attempt, each explicitly owner-authorized after wake confirmation. Both failed interface claim; retained logs and recorded results must stay failed. These do not establish current3.0.0.4 behavior.

No hardware use during repair or delivery. Current version physical connection, image capture and exact OS binding restoration remain deferred. Future testing must notify the owner to wake K-m, minimize battery use, avoid physical USB unplug and use exact Terminal sudo server diagnostics followed by canonical hardware acceptance when possible. Temporary command snapshots are evidence of preparation, not proof of execution or portable launch tools.
