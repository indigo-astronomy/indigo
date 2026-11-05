# Draft documentation security review

Verdict: pass
Reviewer: fresh `guides_review` subagent, separate security pass after quality review; did not author manuals or change implementation. Same reviewer performed both passes, so this is not independent dual-review certification.
Date: 2026-11-04
Reviewed revision/state: base `323a560ff37d036c34bd7984a5e38abed9036c6a`; untracked proposed manuals under `indigo_docs/mcp/` and task evidence on the assigned task branch. Documentation identity is recorded in [quality-review.md](quality-review.md).

## Scope and trust boundaries

This is a documentation-only proposal. It introduces no listener, authentication rule, dependency, native property, file operation or new MCP tool. Reviewed the manuals' directions across the client/MCP/native-bus boundary, agent/hardware ownership, native scripting execution, BLOB URL access, host files/configuration and bridge listeners. Relevant assets are equipment state, images, configuration/scripts, readable bus values and access tokens. Source contracts were inspected locally; no third-party system or hardware was probed.

## Risk coverage

- Authentication/authorization: manuals correctly distinguish native token dispatch from accepted/completed operations, state that no additional MCP authentication or denylist exists, and do not describe Origin rejection as authentication. Omitted token is not falsely described as inherited authorization. No new access scope or filtering policy is introduced by prose.
- Input/injection/output: exact discovered IDs and typed partial writes are required. Script execution is explicitly source execution with persistent side effects, not a filename import, transaction or sandbox. External BLOB URLs remain separate resources; the four MCP tools neither fetch payloads implicitly nor accept BLOB uploads.
- Sensitive data/logging: WO schemas are distinguished from unavailable WO values; readable token fields are not falsely claimed to be masked. Examples use synthetic values and contain no credentials. Guidance avoids publishing tokens and notes native trace/script logging exposure.
- Filesystem/network: configuration/profile overwrite/delete, stored-image deletion, logs, index downloads, external solver prerequisites and Alpaca/LX200 listeners have explicit relevant side effects. Server paths are distinguished from client paths. Missing dependencies are not treated as authorization for package installation or arbitrary downloads.
- Failure/physical state: dispatch, property state, result freshness and physical completion are distinguished. Abort/unload/disconnect are not universal emergency stops or rollbacks. Sequencer ongoing guiding and persistent flipper behavior are disclosed. Missing semantic prerequisites are reported rather than bypassing agent ownership.
- Dependencies/cryptography: none changed; no new cryptographic claim. Native implementation security was not re-audited by this prose task.

## Checks and findings

From repository root, with Python 3.9.6, ran `python3 -B .factory/scripts/validate_project.py --security`: exit 0, `Factory local security boundaries valid.` The configured check validates local Factory boundaries, not application security. Additional documentation checks are linked from [verification.md](verification.md); no native/hardware/client tests were needed or claimed for prose.

The role's suggested canonical `indigo_docs/security.md` input is absent in this checkout. Relevant existing implementation and explicit draft limitations were used for this scoped review; no absent reference is claimed to have been read. No new security finding in the revised documentation scope. The two source-contract findings referenced in the quality report remain open, outside this implementation-free task. No owner risk acceptance was requested, inferred or recorded.

## Verdict limits

Pass is limited to the security accuracy of showing these clearly marked source-inspected drafts. It does not certify equipment protection, runtime correctness, authentication strength, bootstrap deployment or eventual embedded-help implementation. The explicit Alpaca/diagnostic/coordinate/time coverage gaps remain visible. The coordinator retains delivery and follow-up ownership; no commit, integration or publication is authorized by this review.
