# First HTTP increment verification

Date: 2026-11-04. Worktree: `/private/tmp/indigo-mcp-agent-20261103191737`. Candidate: uncommitted source/test/documentation changes over synchronized master `67a7e06fd`; no new task commit. Owner requested inspection before commit. First-increment developer verification and independent quality/security review are complete; owner inspection is pending. Full MCP feature verification is not claimed.

## Environment and baseline

macOS native host; system clang. Existing host universal `libindigo.a` from `/Users/polakovic/Development/indigo/build/lib` supplies unchanged dependencies. Changed `indigo_server_tcp.c` is compiled directly into test binaries, including ASan/UBSan; no claim that the whole framework was rebuilt. Worktree configuration regenerated with `make -B Makefile.inc`, not copied from another checkout. Physical hardware, Windows/Linux builds and a real MCP client are outside this increment. No new dependency or property.

Pre-change network-suite baseline was not captured before editing. Existing regression assertions remain unchanged and are rerun against the changed TCP source. Initial build lacked generated host headers and failed on `hidapi/hidapi.h`; adding the host build include path resolved it. Initial sandbox run could not bind local sockets (`Operation not permitted`); this was not a product test result. Approved local-only runs then executed the suites.

## Executed checks

All commands below run from the worktree root. Build commands use `BUILD_LIB=/Users/polakovic/Development/indigo/build/lib`, `CFLAGS=-g -O1 -arch arm64 -std=gnu11 -DINDIGO_MACOS -I/Users/polakovic/Development/indigo/build/include`, and `LDFLAGS=-L/Users/polakovic/Development/indigo/build/lib -framework Cocoa -framework CoreFoundation -framework IOKit -framework ImageCaptureCore -framework IOBluetooth -lobjc`.

- `cc -std=gnu11 -DINDIGO_MACOS -Iindigo_libs -I/Users/polakovic/Development/indigo/build/include -fsyntax-only -Wall -Wextra indigo_libs/indigo_server_tcp.c`: passed; one existing signedness warning in the unchanged file-download loop.
- `make -C indigo_test build/security/test_transport_http_security` with overrides above, followed by `indigo_test/build/security/test_transport_http_security`: **23/23 passed**, ASan/UBSan instrumented relevant core units including TCP. Covers BLOB upload framing, compression, file cleanup, slow headers and legacy handler/resource retirement. See [build log](http-security-build.log.gz) and [run log](http-security-test.log.gz).
- `make -C indigo_test build/integration/test_server_websocket_sanitize` with the same overrides, followed by `indigo_test/build/integration/test_server_websocket_sanitize`: **5/5 passed**, instrumented changed TCP source. See [build log](websocket-build.log.gz) and [run log](websocket-test.log.gz).
- `./.factory/scripts/check`: passed diff whitespace, central findings references, ADR index and project boundaries. See [log](factory-check.log.gz).
- `./.factory/scripts/security`: passed configured local Factory boundary validation. See [log](factory-security.log.gz); this is not product security certification.

## Acceptance and limits

AC-001: existing HTTP/WebSocket regressions and mixed old/new routing/PUT-ownership tests passed. AC-002/003 and transport portion of AC-004: 13 focused tests passed natively and with ASan/UBSan, including complete maximum-body byte sum, explicit error codes, admission saturation, pipeline closure and route withdrawal proven during callback/body drain. Remaining MCP ACs deferred to later increments. Callback response-size/runtime bounds are explicit handler obligations; no generic callback preemption or streaming API is claimed. Content-Length body limits and admission bound cover buffered new-style requests, not all legacy server work.

Independent quality/security review completed with scoped pass in quality-review.md and security.md; no unresolved blocking finding. Reviewer inspected final content identities and independently ran structural security and whitespace checks; network evidence is attributed to the executing agents. Parent implemented production changes and performs developer verification, so parent checks are not independent. Committed-range process validation, archival, integration and publication are not performed: feature remains unfinished and owner inspection is required before any commit.

## Final focused-suite evidence

The test writer ran `make -C indigo_test test-server-http-handler test-server-http-handler-sanitize BUILD_LIB=/Users/polakovic/Development/indigo/build/lib 'LDFLAGS=-L/Users/polakovic/Development/indigo/build/lib -framework IOKit -framework CoreFoundation'`: both **13/13 passed**, exit 0. The direct-source targets compile the changed TCP implementation; body deadlines are accelerated to 0.25-second progress and 1-second total for bounded adversarial tests. Native binary is universal (Makefile default); sanitizer runs native arm64.

Parent then independently retained run logs from the final binaries using these root-relative commands (each suite reports all tests passed):

```sh
indigo_test/build/integration/test_server_http_handler > .factory/tasks/20261103191737-ccab-mcp-agent/http-handler-test.log 2>&1
indigo_test/build/integration/test_server_http_handler_sanitize > .factory/tasks/20261103191737-ccab-mcp-agent/http-handler-sanitize-test.log 2>&1
indigo_test/build/security/test_transport_http_security > .factory/tasks/20261103191737-ccab-mcp-agent/http-security-test.log 2>&1
indigo_test/build/integration/test_server_websocket_sanitize > .factory/tasks/20261103191737-ccab-mcp-agent/websocket-test.log 2>&1
```

Before this retained run, existing regression targets were rebuilt after the final generic-method dispatcher adjustment with the build overrides above; see final-regression-build.log. All four final logs show PASS counts 13,13,23,5 and no sanitizer diagnostic. The test writer corrected one initial expectation independently reviewed by parent: raw `/http-test#fragment` does not equal `/http-test`, so legacy-prefix fallback remains valid; a fragment in the query of an exact route is rejected. This does not weaken the exact-match contract.

No full application suite, Factory workflow unittest suite, hardware or unavailable platform execution is claimed. Configured check/security helpers passed; source-focused network checks are the relevant executable validation for this increment. Final committed-range/process evidence is deliberately deferred until the owner's pre-commit inspection; process.json remains an unfinished whole-feature record.
