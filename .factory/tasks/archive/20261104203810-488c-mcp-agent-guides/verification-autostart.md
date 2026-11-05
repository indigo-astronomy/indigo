# MCP load-driven lifecycle verification

Owner explicitly requested removing the redundant MCP CONNECTION toggle: loading the agent makes the endpoint available immediately; unloading removes/drains it. Assigned test writer owns only `indigo_test/integration/test_agent_mcp.c` and this report. Original checkout/current task branch; no product-source, build-definition, documentation, user configuration, commit or integration writes by this assignment.

## Baseline before production changes

Working directory `/Users/polakovic/Development/indigo`, current working tree of task `20261104203810-488c-mcp-agent-guides`.

- `make -C indigo_test build/integration/test_agent_mcp`: exit 0; `/private/tmp/mcp-autostart-baseline-build.log`.
- `indigo_test/build/integration/test_agent_mcp`: exit 0, 34/34; `/private/tmp/mcp-autostart-baseline.log`.

The run used the existing synthetic bus/private-HOME fixture and authorized local sockets. No physical hardware, external network or real client was used. Coordinator notified after this baseline and before production edits.

## Requirement-to-scenario changes

| Owner contract | Updated coverage |
| --- | --- |
| INIT immediately serves; repeated INIT is idempotent | Lifecycle case pings immediately after initialization, repeats INIT twice and pings again without any CONNECTION write |
| SHUTDOWN removes endpoint, repeated SHUTDOWN is harmless, reINIT immediately serves | Same lifecycle case checks 404 after shutdown and 200 after reinitialization |
| No visible usable MCP CONNECTION toggle | Existing self-unload case checks MCP property inventory omits CONNECTION; get/set CONNECTION return tool errors; a failed DISCONNECTED write leaves ping serving |
| No second connect step for initialize/read/help or empty-cwd reload | Removed the enable/disable helper; all cases use initialization-only ensure_fixture. Empty-cwd shutdown/reinit retrieves every embedded guide without enumeration/connect writes |
| Existing lifetime/admission boundaries remain | Preserve max-eight saturation/503, accepted-body shutdown drain, replacement route proof, same-stack self-unload BUSY, transport/read/write/help/Unicode regressions |
| Changed driver behavior increments version | Explicit expectation updated from 3.0.0.1 to 3.0.0.2 |

These replace obsolete disabled-by-default and CONNECTION-disable expectations under the owner's explicit changed specification. They do not relax request admission, parser, visibility, dispatch or shutdown assertions. The original 34-case count is retained; lifecycle and former self-disable cases are renamed to reflect their actual contract.

Final build/run results and content identities will be recorded after the coordinator's implementation is ready.

## Final verification

Coordinator declared production version 3.0.0.2 and regenerated embedded documentation ready before these runs. Working directory `/Users/polakovic/Development/indigo`; existing task working tree, no test-writer commit. Native build is universal x86_64/arm64, executed on this macOS arm64 host; sanitizer execution is arm64.

- `make -C indigo_test build/integration/test_agent_mcp build/integration/test_agent_mcp_sanitize`: exit 0; `/private/tmp/mcp-autostart-final-build.log`.
- `indigo_test/build/integration/test_agent_mcp`: exit 0, 34/34 passed; `/private/tmp/mcp-autostart-final-native.log`.
- `indigo_test/build/integration/test_agent_mcp_sanitize`: exit 0, 34/34 passed, no ASan/UBSan diagnostics; `/private/tmp/mcp-autostart-final-sanitize.log`.
- `git diff --check -- indigo_test/integration/test_agent_mcp.c`: exit 0.

All updated cases passed on their first run against the ready autostart implementation. Max-eight overload still produces 503; disabled-state 503 checks were removed only because the owner removed that state. Accepted-body shutdown drain and replacement-route proof pass. Same-thread self-unload still returns BUSY and leaves the endpoint serving. No successful MCP CONNECTION write remains in the tests. Immediate ping, initialize and all embedded-help reads work without a second connection operation, including all-resource reads after reinitialization in an empty cwd.

Final SHA-256 identities:

- Test source: `c95f21cd49237b098de8b29ca48308262fc127b04e3e26804b4792f22db1d915`.
- Agent source: `2bf287e4b37b3efe9e10b86e6865cd42e04fd55c9edf1ae2f54e858d167db6f9`.
- Embedded help header: `8e6a232d1faf029f84526e04f2308f1cc7cf823765f673830d83fff610af89be`.

Test file and report released to coordinator after the runs. Binaries retained for the coordinator's mandatory recorded driver run and independent review; coordinator owns final test cleanup. No physical hardware or real-client run is claimed for this lifecycle adjustment. Changed by this assignment only: the integration test and this report; all 34 cases retained.
