# Agent profile-name test evidence

Verdict: pass

## Baseline

Working directory `/Users/polakovic/Development/indigo`, branch `task/20261105074950-dd62-hide-agent-profile-names`. The earlier MCP guide/autostart work was stashed by the coordinator and was not edited or restored by this test writer.

Before production changes:

- `make -C indigo_test build/unit/test_profile_selection build/integration/test_agent_mcp`: exit 0; `/private/tmp/agent-profile-baseline-build.log`.
- `indigo_test/build/unit/test_profile_selection`: exit 0, 1/1; `/private/tmp/agent-profile-baseline-unit.log`.
- `indigo_test/build/integration/test_agent_mcp`: exit 0, 30/30; `/private/tmp/agent-profile-baseline-mcp.log`.

MCP uses authorized synthetic loopback sockets. Unit tests use private configuration and no hardware/network. Coordinator was notified before source implementation.

## Pre-fix reproducer

Extended only existing `test_profile_selection.c`, preserving the existing case and adding three class/control cases. `make -C indigo_test build/unit/test_profile_selection` passed. `indigo_test/build/unit/test_profile_selection` then exited 1, 2/4 passed; `/private/tmp/agent-profile-expected-fail.log`.

Pure AGENT and mixed AGENT|CCD|AUX failed the hidden/definition/enumeration and guessed-write boundaries: PROFILE_NAME was visible, exposed, renamed and persisted. Existing physical profile selection/persistence and the new non-agent rename/select/persist control passed. Every helper cleaned up its device, observer client, bus and synthetic .common file before reporting failure. This is a focused reproduction of the owner-requested class behavior, not a hardware driver defect claim.

The test's initial compilation used a nonexistent copy-name helper; it was corrected to the repository's INDIGO_COPY_NAME macro before the pre-fix behavioral run. No product-source adjustment was needed for the fixture.

Final results will follow the coordinator's central fix and eleven-agent version update. The MCP test expected-version assertion is the only integration-suite change authorized here; its default-disabled/connection contract is preserved.

## Final focused verification

Coordinator completed the central hidden-flag assignment and all eleven agent version increments, then rebuilt the native shared library before final tests. Test results map to S1/S2 (class visibility and physical control), S3 (expected MCP version only) and S4 (preserved regressions and sanitizers).

Working directory `/Users/polakovic/Development/indigo` unless noted:

- `make -C indigo_test build/unit/test_profile_selection build/integration/test_agent_mcp build/integration/test_agent_mcp_sanitize`: exit 0; `/private/tmp/agent-profile-final-build.log`.
- `indigo_test/build/unit/test_profile_selection`: exit 0, 4/4; `/private/tmp/agent-profile-final-unit.log`.
- `/private/tmp/agent-profile-selection-sanitize`: exit 0, 4/4, no ASan/UBSan diagnostics; `/private/tmp/agent-profile-final-unit-sanitize.log`.
- `indigo_test/build/integration/test_agent_mcp`: exit 0, 30/30; `/private/tmp/agent-profile-final-mcp.log`.
- `indigo_test/build/integration/test_agent_mcp_sanitize`: exit 0, 30/30, no ASan/UBSan diagnostics; `/private/tmp/agent-profile-final-mcp-sanitize.log`.
- `git diff --check -- indigo_test/unit/test_profile_selection.c indigo_test/integration/test_agent_mcp.c`: exit 0.

The temporary unit sanitizer binary used the existing native unit command with universal architecture flags replaced by arm64, O3 replaced by O1, and ASan/UBSan/frame-pointer flags added. Exact command, run from `/Users/polakovic/Development/indigo/indigo_test` (build log `/private/tmp/agent-profile-sanitize-build.log`):

```sh
/usr/bin/clang -g -mmacosx-version-min=10.10 -fPIC -O1 -isystem/Users/polakovic/Development/indigo/indigo_libs -I/Users/polakovic/Development/indigo/indigo_drivers -I/Users/polakovic/Development/indigo/indigo_mac_drivers -I/Users/polakovic/Development/indigo/build/include -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -D_GNU_SOURCE -I.. -I../indigo_libs -I../build/include unit/test_profile_selection.c -o /private/tmp/agent-profile-selection-sanitize -g -headerpad_max_install_names -framework Cocoa -mmacosx-version-min=10.10 -framework CoreFoundation -framework IOKit -framework ImageCaptureCore -framework IOBluetooth -lobjc -L/Users/polakovic/Development/indigo/build/lib -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -lindigo -lm -lpthread -lusb-1.0 -arch arm64 -fsanitize=address,undefined -fno-omit-frame-pointer
```

The unit directly includes `indigo_driver.c`, so the changed central attachment code is instrumented in that sanitizer run. Other shared-library code linked from libindigo is the coordinator's native build; this is not a claim of a fully instrumented framework build. Native targets build universal x86_64/arm64 and execute on macOS arm64; sanitizer binaries execute on arm64. Linux, Windows and physical hardware were not exercised. Hardware run count: 0. Unique synthetic cases: 34, all pass natively and under the scoped sanitizer configurations (68 case executions).

MCP integration diff is exactly the expected-version assertion 0x03000000 to 0x03000001. Its 30 original cases and default-disabled/connection behavior remain unchanged. The unrelated stashed guide/autostart work remains untouched. No new persistent test file or Makefile change was introduced.

Final SHA-256 content identities:

- Unit source: `6dc97c22288d6c958622fe07346ea7159a5e5eda0c5180d364d30a6ad9745165`.
- MCP test source: `5e34abec62ca5c436afc76fabe35f0cf214fdbe035ebafac94862025d0f5fcfd`.
- Changed shared driver source: `4182aaac54ec012f125c6153d5e16120086a976c86d2c6c3bfb3c836383d6269`.

Test source and reports released after these runs. Coordinator owns the eleven-module source audit/build evidence, final recorded MCP runner, independent review, retained logs and final cleanup. Binaries retained during concurrent review; no commit/staging/integration or product-source edit by this writer.

## Coordinator build, recorded run and review

`make -C indigo_libs all` exited0; `/private/tmp/agent-profile-libs-build.log`. Sequential scoped `make -C <agent-directory> [-f ../../Makefile.drv] all` for all eleven inventoried modules exited0; `/private/tmp/agent-profile-agents-build.log`. This builds only the core and agents, not off-limits drivers. macOS universalx86_64/arm64 builds; execution onarm64, Linux/Windows execution unavailable. All11 driver versions increased; exactbefore/after values in versions.json.

`python3 tools/run_driver_test.py agent_mcp` with authorized isolatedloopback access exited0,30/30; `/private/tmp/agent-profile-recorded.log`. It updates only the MCP simulator result in README/TEST_SUMMARY for this commit; historical other-agent/hardware records remain unchanged. Broad unrelated agent acquisition/motion/solver suites were not rerun solely for the shared visibility default; the core pure/mixed-interface contract plus non-agent persistence controls and actual eleven-module attachment inventory bound this change. No hardware/SDK/transport operations change.

Fresh independent review passed in quality-review.md/security.md, including independent nativeunit4/4 execution. Canonical root defect FND-20261105074950-dd62-hide-agent-profile-names-001 is fixed and reverified. No other finding or risk acceptance. Earlier MCP feature stash remains excluded.

Final synthetic acceptance:4 distinct profile cases and30 original MCP cases,34/34 native and34/34 under scopedASan/UBSan (the unit compiles changedcommoncode under instrumentation; no fully instrumentedframework claim). Independentunit4/4 and recordedMCP30/30 also pass. Physical hardware0run/0passed. Pre-fix2/4 reproducer is retained as failureevidence, not passingacceptance.

Cleanup: `make -C indigo_test test-clean` exited0 after all review executions. Final committed-range process check and WIP restoration are local delivery steps after archival/commit; no such result is claimed in this precommit report.
