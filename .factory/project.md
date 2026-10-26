# Current project context

## Purpose and compatibility
INDIGO is an astronomy device framework and software bus, with native/legacy protocol adapters, server, tools and modular device drivers. Core code is C, with C++, Objective-C, Python, shell and browser assets where already established. Linux, macOS and Windows support must be preserved. Workflow adoption does not change application behavior or public properties.

## Repository map
| Component | Sources | Tests | Build / documentation evidence |
| --- | --- | --- | --- |
| Core bus, protocol, base drivers and portable I/O | indigo_libs/; public headers indigo_libs/indigo/ | indigo_test/unit/, indigo_test/integration/ | indigo_libs/Makefile; indigo_docs/DEVELOPMENT.md |
| Portable drivers and agents | indigo_drivers/; .driver inputs own generated C/headers/main files | indigo_test/integration/, simulator_common/, fake SDK/USB backends, driver-local simulators | Makefile.drv, Makefile.drvs; driver REFACTOR.md and README.md |
| Platform and optional drivers | indigo_linux_drivers/, indigo_mac_drivers/, indigo_optional_drivers/ | applicable indigo_test/integration/ and explicit hardware targets | platform make/project files; SDK prerequisites |
| Server and browser UI | indigo_server/ and indigo_server/resource/ | indigo_test/unit/, indigo_test/integration/ | indigo_server/Makefile; existing UI resources |
| Tools, examples and developer utilities | indigo_tools/, indigo_examples/, tools/ | indigo_test/; driver editor's existing tests | component Makefiles; indigo_tools/driver_editor/AGENTS.md |
| Windows and system integration | indigo_windows/, indigo_windows.sln, Directory.Build.props, systemd/ | platform/hardware checks when authorized and available | Visual Studio solution/projects; service units |
| Automated tests | indigo_test/unit/, integration/, fixtures/, simulator_common/, mountsim/, benchmark/ | indigo_test/Makefile; test_runner.h | indigo_test/AGENTS.md; DRIVER_TESTING_RULES.md |
| Factory helpers | .factory/scripts/ | indigo_test/factory/ | .factory/checks.json; disposable local fixture tests |
| Documentation and project configuration | indigo_docs/, root Markdown, indigo.xcodeproj/ | static checks and review | root AGENTS.md; indigo_docs/MAKEFILES.md |
| Vendor/generated/build/legacy exclusions | externals/ and bin_externals/ at any depth, build/, generated driver outputs, obsoleted_drivers/, indigo_test/shell_compliance/ | no automatic execution | owner exclusions; generated output inspected only with its source when assigned |

## Commands
All commands start at the Git root unless an explicit -C changes directory. Python helpers require Python 3.9+ and local Git with offline object flags.
- Workflow validation: `./.factory/scripts/check`, `./.factory/scripts/test`, `./.factory/scripts/security`; configured coverage is Factory integration, not application or hardware certification.
- Native build: `make all`; Windows: `msbuild indigo_windows.sln /t:Build /p:Configuration=Release`. Review scope first: broad native builds can include off-limits drivers and vendor SDKs.
- Native unit suite: `make -C indigo_test test-unit`, requiring a built INDIGO library. Full native suite: `make -C indigo_test test`; the opt-in Bash tools in indigo_test/shell_compliance/ are separate from this suite.
- Driver runs require an explicit interactive/non-interactive and simulator/hardware mode, then the final recorded run through `python3 tools/run_driver_test.py <driver>`; preserve per-driver commit units.
- Dependencies remain managed by existing makefiles, vendored SDKs and platform projects. No new package manager, dependency installation or hosted CI is introduced. Tests run through the local Makefile targets; no hosted CI is configured.

## Authority and scoped rules
Owner: Peter Polakovic. Root AGENTS.md starts with shared Factory rules followed by INDIGO requirements. Driver rules: indigo_drivers/AGENTS.md; automated test rules: indigo_test/AGENTS.md; source rules: indigo_libs/AGENTS.md; documentation rules: indigo_docs/AGENTS.md; editor rules remain scoped to indigo_tools/driver_editor/AGENTS.md. Vendor instructions remain unchanged.
Never modify ccd_apogee, ccd_sbig, mount_asi, mount_mxhd or system_ascol under indigo_drivers/. Do not change the generator without approval of a concrete diff. Existing README modification approvals and property/version/testing rules remain effective.
Legacy REVIEW.md tracking is retired. Current findings live in individual .factory/findings/active/ records (registry.json v2); historical aliases/checkpoints are preserved by the adoption task. Do not interpret imported findings as verified fixes or accepted security risks.

## Git, permission and data boundaries
- Integration branch: local master, origin git@github.com:indigo-astronomy/indigo.git. Adoption base: `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. Remote protections are not inspected; publication and integration require separate owner authorization.
- Task branches: task/<task-id>, using UTC YYYYMMDDHHMMSS-hhhh-slug IDs (seconds and four random hex digits), matching the reviewed helper guard; this is the explicit Factory branch convention for adopted tasks. Existing branches and history are preserved. No merge or push is authorized by adoption.
- The owner authorizes this pinned Factory upgrade, task-ID/reference migration and independent findings/completion storage. The owner authorizes the subagent/cadence policy transfer to AI Factory and pinned update back into INDIGO. Routine subagents within the approved host/provider are authorized and preferred for separable subtasks and independent verification/review when useful. Further upgrades, provider changes, additional charges, publication and deployment require their own authorization.
- Assign each subagent an explicit scope and output; keep one writer per task/worktree and use separate worktrees for concurrent implementation. Private configuration, credentials and inventory stay untracked. Cross-provider review is disabled in review-policy.json; enabling it requires an owner-approved policy change. No provider is invoked by model configuration.
- No budget ceiling was supplied; actual per-task cost and exact host model/effort are unavailable. Retain the current host/model.

## Hosts and maintenance
Owner timezone: Europe/Bratislava. Entry points: AGENTS.md, CLAUDE.md, GEMINI.md, .cursor/rules/ai-factory.mdc and .github/copilot-instructions.md. Shared bootstrap: .factory/bootstrap.md. Codex explicit loading/context export can be checked here; fresh native sessions for other clients remain unverified. See task verification evidence for precise limits.
Execution state is per task (.factory/tasks/<id>/state.json); .factory/state.json is a v2 repository marker. Findings use FND-<origin-task-id>-<sequence> for new records, preserving legacy IDs. Maintenance policy stays in .factory/maintenance.json; `./.factory/scripts/maintenance status` derives cadence from .factory/completions/. Twenty uncovered work tasks or seven calendar days makes the full audit due; task-diff review does not reset it. The latest full maintained repository source-contract audit is task 20261024121920-3bd2-repository-audit, reviewed source 9d45bc8ccadaf0c3aab6c454c38429fec21d1699 on 2026-10-24, including server security. It records 44 new open findings (6 high / 32 medium / 6 low) and explicit platform/hardware/runtime limits; the six high findings 001/002/003/010/011/042 are repaired and reverified in [the high-priority repair](tasks/archive/20261024135437-8e0c-fix-property-copy/security.md); fifteen selected core/image/agent findings (004–009, 012–014, 033, 037–039 and earlier audit 031–032) are repaired in [the selected core repair](tasks/archive/20261024155149-8381-fix-core-audit-findings/findings.md); thirty-one further audit findings (bc8d, 3bd2, 60d4 and FND-000048) are repaired in [the audit findings batch](tasks/archive/20261025211159-e308-fix-audit-findings-batch/verification.md); other remaining findings still require product changes, with no risk acceptance. The 13 high-priority server-audit findings and a coupled upstream XML message-format defect are repaired and reverified in the [server security repair](tasks/archive/20261023121227-a609-fix-server-security-high/security.md). Four lower-priority server-audit findings and the separately registered XML cached-RO-buffer leak are repaired and reverified in the [remaining server security repair](tasks/archive/20261024115039-61cc-fix-server-security-remaining/security.md). The blank-ACL-line finding (audit-010) remains open. The [server security audit](tasks/archive/20261023112118-8e5e-server-security-audit/security.md) retains original evidence; no whole-server security/hardware/release certification or risk acceptance is implied. Scope and follow-ups: [repository review](tasks/archive/20261024121920-3bd2-repository-audit/repository-review.md). Cadence/checkpoint comes from qualified completion evidence; earlier partial review did not reset it. Historical approvals remain in maintenance.json.

## Provenance and updates
- Approved template: https://github.com/polakovic/ai_factory, baseline `a94e122fbe358d877863cbc1b38ae8284edc91c4` (verified local candidate; remote publication pending); reviewed source-to-target map: .factory/template-lock.json.
- Persistent local registration alias: indigo. Inventory is kept in the owner-designated persistent AI Factory checkout, outside this project; do not copy its contents or other project paths into task evidence.
- Future updates require a new owner-authorized project task and the pinned three-way template-update routine. Registration alone does not authorize an upgrade or a background automation.
- Adoption evidence: [.factory/tasks/20261021193138-70b4-adopt-ai-factory/migration.md](tasks/archive/20261021193138-70b4-adopt-ai-factory/migration.md), [verification](tasks/archive/20261021193138-70b4-adopt-ai-factory/verification.md), [legacy findings](tasks/archive/20261021193138-70b4-adopt-ai-factory/legacy-findings.md).

## Xcode metadata boundary
Xcode projects must not reference .factory or any descendant through groups, folder/file references or build membership. Factory lifecycle changes must not modify project.pbxproj. Application build/source objects and the actual Factory records stay independent.

## Documentation
New project documents are English. Keep current facts here, task execution evidence under tasks/, canonical findings under findings/, and durable Factory decisions under docs/decisions/. Product/developer documentation remains under indigo_docs/. No template project task history or findings archive is imported.
