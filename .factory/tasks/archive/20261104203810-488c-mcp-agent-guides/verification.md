# MCP manuals and embedding verification

Verdict: pass

Current verdict covers the owner-authorized uncommitted offline-help implementation and its integration acceptance. Earlier draft-only checks below are historical phases of this same task; observing-recipe/model acceptance and committed-range delivery remain outside this verdict.

## Scope and working state

Repository root: `/Users/polakovic/Development/indigo`. Branch: `task/20261104203810-488c-mcp-agent-guides`; base MCP revision `323a560ff37d036c34bd7984a5e38abed9036c6a`. Candidate is untracked draft Markdown plus task evidence, not committed, integrated or embedded. No native source, build files, properties or Xcode project changed. Runtime, simulator and GUI tests were not performed for this prose-only proposal. The acceptance scenarios describe future checks, not results.

## Mount epoch source discrepancy

Independent source review found Mount Agent target forwarding and observed coordinate readback use the selected mount frame (agent_mount.c around406–415 and1125–1130), while derived coordinate calculation unconditionally invokes J2000-to-current conversion. Shared solver converts targets according to the mount's reported epoch (indigo_platesolver.c around646,742,752). The manuals were corrected to distinguish actual passthrough behavior and unresolved derived-calculation assumptions. A separate functional finding retains follow-up ownership; this task does not fix or validate non-J2000 operations.

## Coverage boundaries

Eleven native agent modules have source-inspected manuals. Optional Sequencer is separately inventoried with its interpreter/bootstrap prerequisites. Alpaca per-member semantics and Bahtinov spike-angle interpretation remain explicitly incomplete. Missing hardware-specific recipes are not inferred from labels. Draft acceptance is limited to a reviewable proposal, not complete production reference or model task success.

## Checks

Preliminary documentation check parsed12 JSON examples and checked379 local links/anchors; only the pending Sequencer file references were unresolved at that point. Final results follow after release. `git diff --check` exited0 for tracked content; the custom Markdown check also inspects untracked files. `./.factory/scripts/findings check` exited0:23 active records and retained task evidence valid. Independent reviewer ran `python3 -B .factory/scripts/validate_project.py --security`: exit0, local security boundaries valid. No network clients or hardware were contacted.

## Sequencer transit pause discrepancy

Canonical Sequencer.md describes `enable_meridian_flip(use_solver, time)` as seconds (line307); Sequencer.js forwards time unchanged via `set_pause_after_transit` (around313 and1695) to Imager `AGENT_IMAGER_BATCH.PAUSE_AFTER_TRANSIT`. The Imager source labels and uses that value in hours (around1312 and2627). This is a separate nonblocking documentation/contract finding. The new draft explicitly warns about the discrepancy rather than silently choosing a conversion. Existing source/reference files remain unchanged.

## Final draft structural check

`python3 /private/tmp/check_indigo_mcp_guides.py` from repository root checks all17 Markdown files, local paths/anchors, all12 JSON examples and whitespace, including untracked documents. The optional Sequencer group inventory is included in the final catalog. Writer source inventory found all108 public Sequence prototype methods represented; no script execution claimed. Final command output is recorded below.

Final structural output: `17 Markdown files, 410 local links checked, 12 JSON examples parsed`; exit0.

Final independent Sequencer reinspection corrected three draft mistakes: batch start item is `AGENT_START_PROCESS.EXPOSURE`; joystick preparation uses `FILTER_JOYSTICK_LIST`; numeric `wait_until` input is epoch seconds. These are documentation-only corrections checked against library/native names; structural check was rerun afterward.

Coordinate-frame notes and numeric `break_at` epoch seconds were also corrected in final Sequencer review. Final post-correction structural result:17 Markdown files,411 local links/anchors,12 JSON examples; exit0.

Independent reviewer confirmed the final411-link state and recorded final content identity in quality-review.md. Earlier399/410 counts above are intermediate checks, superseded by411 after catalog/frame additions. Security and findings checks reran successfully; Git tracked diff remains empty.

## Embedding review defects and fixes

The independent reviewer found two additive help interoperability defects before final delivery: generated catalog links include section fragments but initial lookup rejected those full links; and standard resource requests may carry `_meta` objects but initial strict validation rejected them. Production now strips a fragment then matches only a compiled base topic/URI, returns the full document/canonical base URI, and accepts optional object metadata for resources only. Get_help arguments stay strict. Focused regression/reverification results are owned by verification-embedding.md; these are not native bus, authentication or filesystem authorization changes. No arbitrary file/URL lookup was introduced.

## Embedding delivery checks

The detailed independent baseline/new-feature/final test evidence is in [verification-embedding.md](verification-embedding.md). Final frozen source was tested native34/34 andASan/UBSan34/34. The parent then ran `python3 tools/run_driver_test.py agent_mcp`: initial sandbox attempt could not bind its loopback listener (Operation not permitted), resulting in environment failure33 cases, retained in `/private/tmp/mcp-embedding-recorded.log`. The same recorded runner with authorized loopback access exited0,34/34, recording3.0.0.1 in MCPREADME/TEST_SUMMARY; log `/private/tmp/mcp-embedding-recorded-unrestricted.log`. No assertions/source changed for the rerun.

`make -C indigo_drivers/agent_mcp all` passed for universal macOS x86_64/arm64; `file build/drivers/indigo_agent_mcp.dylib` confirms both architectures. Execution was on arm64, not an Intel host. Xcode project `plutil -lint` and Windows project XML parsing passed; no Xcode full-server build, Windows or Linux execution is claimed. Only MCPfiles/header/docs metadata changed; no Factory Xcode references introduced.

`python3 indigo_drivers/agent_mcp/generate_help.py --check` passes18documents. An isolated temporary-output exercise confirms stale content returns exit1, generation is deterministic, and a fresh check passes; repositoryheader not mutated by that exercise. Independent review additionally compiled generateddata as strictC99 and decoded UTF8/terminalNUL +embeddedlinkwhitelist. The script uses standardPython only at generation/check time; runtime has no file/network documentation lookup.

`./.factory/scripts/check` and `./.factory/scripts/security` passed; decision index/current findings and configured boundaries valid. These checks are not a full application audit. Native client/model observing-task acceptance scenarios remain unperformed; only loopback/synthetic bus integration verified this increment. ExistingMount/Sequencer findings001/002 remain open. Embedding findings003/004 were fixed/reverified and archived; no risk acceptance implied.

No commit, staging, merge, push or task archival. A committed-range process check is not applicable to this intentionally uncommitted candidate; retained task/process evidence supports later owner-authorized commit preparation.

## Final test summary and cleanup

Final hardware-free acceptance:34 distinct integration cases,34 passed in the native suite and the same34 passed underASan/UBSan. Independent reviewer execution34/34 and the mandatory recorded runner34/34 also passed. Physical hardware tests:0 run,0 passed. Intermediate baseline/fixture/environment failures above are retained and are not passing acceptance results. Recipe/model task scenarios remain unperformed.

After all reviewers released their files/binaries, `make -C indigo_test test-clean` exited0. Removed only the generator-import bytecode created by the temporary-output exercise. Native driver build outputs remain ignored build products. No user processes/configuration were changed. Final configured checks/freshness are rerun after this evidence update.

## Automatic activation verification

Owner explicitly changes the prior disabled-by-default behavior: loading the MCP module now starts service, and inherited CONNECTION is hidden. Old enable/disable assertions are obsolete under that owner contract, not removed to conceal a regression. Detailed baseline and final34/34 native plus34/34 ASan/UBSan results are retained in verification-autostart.md. Existing saturation503, shutdown drain, self-unload BUSY, protocol/bus/help behavior remain covered.

Parent commands from repository root: `make -C indigo_drivers/agent_mcp all` exited0 (universal macOS x86_64/arm64); `python3 tools/run_driver_test.py agent_mcp` with isolated loopback permission exited0,34/34, recording3.0.0.2 (log `/private/tmp/mcp-autostart-recorded.log`). No running user server was changed or restarted. `python3 indigo_drivers/agent_mcp/generate_help.py --check` passed18documents; Xcode plutil lint passed; configured Factory check/security passed. A mistyped shell invocation of security did not run; the corrected exact security command passed. Native client/model task acceptance remains outside this change; hardware0 run/0 passed.

Property reference now records the MCP-specific hidden CONNECTION exception and load/unload lifecycle. No shared-framework or custom-property addition. No commit, staging, merge, push or archival. Prior frozen embedding hashes/reviews are historical; final autostart identities and review supersede them for this working candidate.

Final autostart independent review passed (quality-review-autostart.md/security-autostart.md); reviewer independently executed native34/34. Source/header/test frozen identities match verification-autostart.md. No new finding. Latest verdict supersedes historical embedding-only identities without changing their retained evidence. All34 distinct hardware-free cases passed native and underASan/UBSan; independent/recorded replays34/34; hardware0/0.

Final autostart cleanup: `make -C indigo_test test-clean` exited0 after independent review. Post-cleanup Factory check/security and18-document freshness passed. No staging or commit.

## Post-profile-commit restoration

Separateprofilefix5a48ccdd2 passed its immutable processgate witherrors[] on clean tree. Its implementation/evidence is committed; MCPfeaturework was restored afterward withoutcommit. verification-profile-restoration.md records combined4/4profile,34/34MCP native and34/34scopedASan/UBSan. Parentrecordedrunner`python3 tools/run_driver_test.py agent_mcp` exited0,34/34 at3.0.0.3; `/private/tmp/agent-profile-restored-recorded.log`. SavedMCP C/test content preserved exactly apart from version02→03; original untracked files restored. Masterunchanged/no push. This restoration does not rewrite priorreview identities or imply newGUI/hardware/modelacceptance.

## Final synchronized delivery

The latest combined candidate is verified in [verification-delivery.md](verification-delivery.md); this supersedes earlier phase state/version statements. Final MCP version3.0.0.3, native34/34, ASan/UBSan34/34 and profile4/4 pass after merging origin/master. Final independent reviews are canonical quality-review.md/security.md. Commit/integration/publication now authorized; immutable delivery gates remain pending.
