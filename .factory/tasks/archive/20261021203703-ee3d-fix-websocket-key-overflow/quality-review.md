# Task quality review

Verdict: pass
Reviewer and independence: single agent, quality-reviewer role; the same agent wrote the change, disclosed.
Reviewed revision and working-tree state: task/20261021203703-ee3d-fix-websocket-key-overflow on f1a28a893 plus the uncommitted task diff.

## Scope and method
Reviewed the final diff of indigo_libs/indigo_server_tcp.c, the new indigo_test/integration/test_server_websocket.c, the indigo_test/Makefile rules, the Xcode project entries, the findings register edits and the corrections to task 003 records. Checked the change against the specification, repository formatting rules and nearby code.

## Quality findings
None blocking. Observations:
- The fix is the smallest one that removes the overflow: one length check, one flag and one 400 reply that follows the existing 404 reply pattern. The magic GUID is named once instead of repeated.
- Formatting follows the root rules: tabs, K&R braces, braces on every if, calls on one line, no blank lines inside the function body.
- The test exercises the production server through real TCP, asserts exact accept values computed independently, and was shown to fail on the unfixed source. Its first draft used a wrong RFC accept constant from memory and a close check that misread the reply body; both were corrected before the final runs, and the final test was rerun against the base source.
- The sanitize build compiles the server source into the test and links the static library, so only one copy of the server code exists in that binary.
- The task 003 correction replaces a false pass claim with the recorded result and says so in place.

## Fix reinspection
The flag is recomputed for every Sec-WebSocket-Key header line, so a later valid key header replaces an earlier over-long one and the reverse. A key of exactly 219 bytes fills the buffer to 255 bytes plus NUL. The 400 path jumps to the existing failure label, which releases the handle, the BLOB lock and the buffers as before.

## Quality verdict
Pass. Remaining limitation: no independent reviewer.
