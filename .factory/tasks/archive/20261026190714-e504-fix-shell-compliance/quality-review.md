# Quality review

Verdict: pass

## Scope, revision and independence

Independent same-provider quality-review context, not an implementation author. Reviewed task branch `task/20261026190714-e504-fix-shell-compliance`, base `66d6815ddc2882895ebc351807977ad008b8274c`, and the actual working-tree diff including the new Python regression source. This is a task-diff review; the owner authorized the still-due repository audit deferral for this repair only. Off-limits drivers, unrelated code and physical hardware were excluded; archived finding records were accessed only for the concrete task reconciliation.

Scope covers the shared Bash framework, six compliance entrypoints, synthetic regression fixtures, corrected existing GPS fixture, documentation, Makefile/Xcode registration and task/maintenance metadata. The final required source headers preserve the existing Rumen copyright and script descriptions. No production driver, generator, dependency, live server, integration or publication change is introduced.

## Contract inspection

The real `indigo_prop_tool.c` list, verbose-number and tracked-state output contracts were compared to the fixed literal selectors. `run_prop_tool` preserves quoted executable/remote/device arguments; checks capture command failure before filtering or normalization. Scalar capture occurs in the parent shell, with approved getters and plain variable destinations compatible with Bash 3.2. Numeric transport replies and range bounds reject malformed/nonfinite values rather than silently becoming zero. Exact full property/item matching avoids regex and suffix collisions.

Connection snapshots are taken before the battery can alter an originally connected device. Normal cleanup occurs before summaries; the owned EXIT/INT/TERM cleanup retains an existing failure/signal status and reports restoration failure. No trap is installed merely by sourcing the framework. The abort helper uses synchronous bounded BUSY observation, no background helper, immediate abort and honest fast-completion skips. Wheel values are held in indexed arrays without eval; abort endpoints use valid finite readbacks and range-aware selection.

Existing GPS acceptance assertions remain intact; the fixture changes replace its invented legacy state lines with the actual CLI format and add the scalar state operation needed by the public helper. Synthetic regressions remain isolated from hardware while live compliance entrypoints remain opt-in. Build/Xcode registration adds only the new Python regression, not Factory metadata. Documentation accurately describes capture, typed numeric helpers, command failure, battery return 3 on query error, counters and the hardware limitation.

## Findings, corrections and reinspection

`FND-20261026190714-e504-fix-shell-compliance-001` (Q1, medium) is fixed. The initial nominal AO/guider fixture omitted mandatory pulse properties/ranges and the normal-flow test accepted a summary after early failure. An independent nominal AO probe reproduced exit 1 with an AO_GUIDE_RA read failure (8 passed / 3 failed). The corrected fixture supplies mandatory data and models pulse completion. All six entrypoints, each initially ON and OFF, now require the normal restoration footer, exit 0, no FAIL and zero failed count, as well as restored state. The separate `never_busy` control now exits 0 with only OK; failed-process mode retains exit 7. This distinguishes missing BUSY from command failure without weakening assertions.

`FND-20261026190714-e504-fix-shell-compliance-002` (S1) is fixed and independently reinspected for quality. Three numeric comparators now validate both operands as finite numbers and compare data through constant AWK code using environment values. Raw replies such as `1+1` cannot become executable arithmetic. The added negative control covers all three comparators. Security analysis and revalidation are recorded in [security.md](security.md).

No unresolved quality finding remains. The five original findings and these two review findings are archived fixed; the registry check reports 38 active / 177 archived. Final task findings and verification records agree with the implementation. Detailed records remain in [findings.md](findings.md).

## Independent checks and acceptance evidence

Independent `python3 -B indigo_test/unit/test_shell_compliance.py FrameworkContractTests` passed 20/20 before the numeric correction (5.568 seconds) and 21/21 after it (6.916 seconds), on macOS arm64 with `/bin/bash` 3.2.57. Independent affected-control run of `EntryPointRestorationTests.test_six_entrypoints_restore_after_normal_control_flow` and `AbortMotionTests.test_missing_busy_or_tool_failure_cannot_skip` passed 2/2 (25.375 seconds). Independent `git diff --check`, Xcode project lint and finding-registry validation passed.

Inspected retained final acceptance logs: `final-shell-regressions.txt` records 33/33 methods in 57.000 seconds; `final-gps-regressions.txt` records 9/9 in 34.789 seconds. Coordinator evidence records Factory 190/190 in 134.416 seconds, syntax checks of all seven scripts, Xcode lint and diff checks. Exact full-suite commands, working-tree identity, baseline and limitations are in [verification.md](verification.md); this review does not claim to have independently rerun every coordinator check.

No physical hardware, Linux/Windows execution, ShellCheck run or full native/driver regression is claimed. This review establishes the synthetic shell contracts and final diff quality, not live device-class compliance. The coordinator must inspect archival link rewrites and completion metadata, bind reviewed content identities, and run retained/committed-candidate delivery gates. Integration and publication remain pending and are not approved by this report.

## Post-archive metadata review

Narrow reinspection of this task archive and its completion record passed. Archived state remains phase/status `complete`, assigned role `coordinator`, blocker `null`; the completion is kind `work` with audit `null`. Both baseline `.txt` files and the final shell/GPS `.txt` evidence remain retained, with final logs still recording 33/33 and 9/9 successes. Archive reported zero Markdown link rewrites; inspected report links remain task-relative and resolve inside this retained task directory.

The handoff and verification preserve the repair-only owner deferral and explicitly leave the repository audit due; this ordinary work completion does not reset its checkpoint. Runtime sources are unchanged according to the coordinator; this addendum reviews archival metadata and retention only. Content binding, staging, commit and immutable delivery gates remain pending coordinator steps. The quality verdict remains pass.

## Evidence-format correction review

Narrow review of the follow-up delta passed: baseline line 68 loses only its trailing diagnostic separator space, and findings.md loses only an extra blank line at EOF. No diagnostic failure content, runtime source, test or assertion changes. The handoff and verification candidly record that orchestration committed despite the failed staged whitespace gate; they require corrected committed-range whitespace, process and configured checks before delivery. Original raw evidence bytes remain available in ancestor commit `0c3436a3e`; history is preserved. Independent working-tree `git diff --check` passes. This addendum approves the formatting correction and accurate explanation only; it does not substitute for the coordinator's pending immutable candidate gates. The quality verdict remains pass.
