# Combined MCP/profile restoration verification

Verification assignment only. The coordinator completed the separate shared agent-profile fix in commit `5a48ccdd2a25c431811062595361cdf1424eb1e7`, returned to `task/20261104203810-488c-mcp-agent-guides`, fast-forwarded to that commit and restored the unfinished MCP guide/help/autostart work. This report verifies the current combined working-tree candidate; it does not claim that the uncommitted MCP work belongs to the profile-fix commit.

The coordinator reconciled the MCP version to 3.0.0.3 (`DRIVER_VERSION` 0x03000003 and `MCP_AGENT_VERSION` "3.0.0.3") to account for the additional profile behavior. The integration test expects that same version. The preserved unit source is the four-case committed profile suite. No source, tests, build definitions, older task records, runtime server or client were edited by this verification assignment.

## Commands and results

Working directory: `/Users/polakovic/Development/indigo`.
Branch: `task/20261104203810-488c-mcp-agent-guides`.
HEAD: `5a48ccdd2a25c431811062595361cdf1424eb1e7`, with restored uncommitted MCP work.

- `make -C indigo_test build/unit/test_profile_selection build/integration/test_agent_mcp build/integration/test_agent_mcp_sanitize`: exit 0; `/private/tmp/mcp-profile-restoration-build.log`.
- `indigo_test/build/unit/test_profile_selection`: exit 0, 4/4 passed; `/private/tmp/mcp-profile-restoration-unit.log`.
- `indigo_test/build/integration/test_agent_mcp`: exit 0, 34/34 passed; `/private/tmp/mcp-profile-restoration-native.log`.
- `indigo_test/build/integration/test_agent_mcp_sanitize`: exit 0, 34/34 passed, no ASan/UBSan diagnostics; `/private/tmp/mcp-profile-restoration-sanitize.log`.
- `git diff --check -- indigo_test/integration/test_agent_mcp.c`: exit 0.

The unit suite verifies pure/mixed AGENT profile-name hiding and guessed-write boundary, plus unchanged physical rename/persistence/selection. The MCP suite retains automatic service startup, unavailable CONNECTION, idempotent load/unload, overload/admission and draining, same-stack shutdown BUSY, read/write/token/Unicode and all 18 embedded help-resource cases including empty-cwd reload. All pass together after restoration.

Native targets are universal x86_64/arm64 builds executed on macOS arm64; the sanitizer binary executes on arm64. Tests use their existing private-HOME fixtures and synthetic loopback sockets. Local-listener execution used authorized sandbox escalation. No physical hardware, live server restart, real-client test, Linux or Windows execution is claimed. No new commit was created.

## Tested content identities

SHA-256, unchanged between pre-build capture and final verification:

- MCP agent source: `bba659b4e7d8f3740c7f0bd0f4afa764da2a90bd1c998a07527d0ce66014ef9d`.
- Regenerated embedded help header: `dd52730bc1feed2ee545279a46541e2f5cb6a92344c28f5a455b570af3fd53f0`.
- MCP integration source: `36df255c1e17c90ae66f1260fb3cd34c9a2126305b884f8e9b12c25dd226040f`.
- Profile unit source: `6dc97c22288d6c958622fe07346ea7159a5e5eda0c5180d364d30a6ad9745165`.

Only this new report was written. The original profile-fix task and its archived records remain untouched. MCP loopback use is released to the coordinator for the mandatory recorded driver runner, review and cleanup; test binaries remain available until that coordinated cleanup.
