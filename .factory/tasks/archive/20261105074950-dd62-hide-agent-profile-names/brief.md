# Hide profile names on every agent

Owner: Peter Polakovic. Requested on2026-11-05: fix the common layer and ALL agents, with a separate commit. Agents expose PROFILE_NAME although their PROFILE/CONFIG controls are hidden. All eleven current agent modules are in scope; ordinary devices with functional profiles must retain them. No hardware, server restart, push or main integration authorized.

Prior MCP manuals/autostart work is preserved in stash c376e9507f27b7c216adbf5dd3392bd201ffa106 from task/20261104203810-488c-mcp-agent-guides and must be restored after this commit. Do not include it in this task's commit. Current branch task/20261105074950-dd62-hide-agent-profile-names based on323a560ff37d036c34bd7984a5e38abed9036c6a. Dependent-base exception: master67a7e06 is included; committed MCP is needed to cover every agent. Work remains original checkout, no worktree. No remote update/publish/integration requested.
