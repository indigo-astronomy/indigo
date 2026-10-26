# Security review

Verdict: pass

## Scope, revision and independence

Independent same-provider security-review agent; no implementation authorship or source edits. Reviewed base `66d6815ddc2882895ebc351807977ad008b8274c` and the task working-tree diff after the coordinator declared the runtime sources frozen, then reinspected the numeric-helper correction below. Scope: shared shell framework, six compliance entry points, synthetic Python fixtures, documentation, Makefile/Xcode registration and the owner-scoped Factory audit deferral. This is a task security review, not a repository/server audit. Historical archives, off-limits drivers and physical hardware were excluded.

## Trust boundaries and inspection

- Tool executable, remote options and queries remain separate quoted argv entries; the shared helper preserves actual executable status before filtering. Device names and replies are data rather than shell programs.
- Wheel names/offsets use indexed arrays instead of `eval`; required-read failure paths restore connection and stop further mutation. Captured destinations are plain identifiers with the helper's local namespace excluded, getters are whitelisted, and `printf -v` assigns scalar values under Bash 3.2. Arrays are assigned explicitly in the caller.
- Numeric reads and both range bounds use anchored numeric syntax and finite conversion checks. Numeric-mode comparison helpers now use typed readbacks and a constant AWK expression with ENVIRON data, removing interpretation of readback text as a bc program. Expected numeric literals are checked before a setter can mutate a device.
- Literal property/item selection and current-CLI state-line parsing reject substring and regex metacharacter confusion. Diagnostics are not property data. Only the current tool's actual list/state formats are used by the repaired fixtures.
- Original connection snapshots require ON/OFF before mutation. Owned EXIT/INT/TERM traps restore and verify that snapshot, exclude command-substitution subshell restoration, preserve existing failure/signal exit codes and fail a successful exit if cleanup cannot be verified. They restore connection state only, not every changed setting or motion state.
- Abort uses a synchronous bounded BUSY request and immediate current-state/abort checks. No background helper is started. A completed move can be SKIP rather than false coverage; malformed states and nonzero start/state/abort results fail. Restoration is an attempted and verified operation; inaccessible hardware is not represented as successfully restored.
- Fixtures use private temporary trees, synthetic CLI records and controlled owned subprocesses/signals. They select no real server/hardware. Automated registration runs the synthetic regression source only. No dependency, credential, external service or publishing behavior is introduced.
- The audit deferral records explicit owner authorization for this repair only and leaves the checkpoint unchanged. No validator, approval authority or acceptance rule is weakened.

## Resolved S1 — FND-20261026190714-e504-fix-shell-compliance-002

Category security/input validation; severity medium; coordinator-owned, verified corrected in this working-tree candidate. The first source pass found `test_get_value`, `test_set_and_verify` and `test_set_transition_smart` still reading raw text in numeric mode and interpolating it into bc. A benign literal readback `1+1` was falsely accepted as expected numeric `2`; untrusted bc-language input could also prevent bounded cleanup.

The correction selects `get_item_number` for numeric-mode reads, checks expected finite numeric literals before mutation, and compares validated values using fixed AWK source with ENVIRON input. String-mode behavior remains unchanged. Source reinspection confirms all three paths use these guards. Independent local controls now reject literal `1+1` in all three helpers; valid `1e3`, `-.5` and `+2.5e-1` still pass the intended numeric comparisons. `nan`, `inf` and overflowing `1e999` reject. The control used only synthetic function replies and the actual framework, with no server, network or source mutation.

`python3 -B indigo_test/unit/test_shell_compliance.py FrameworkContractTests.test_numeric_comparisons_reject_expression_reply FrameworkContractTests.test_numeric_readback_rejects_empty_malformed_and_nonfinite_values FrameworkContractTests.test_numeric_readback_accepts_finite_signed_and_exponent_values` passed 3/3 independently in 2.600s. Coordinator owns the central record closure after final acceptance. No security risk acceptance is granted.

## Checks and limits

Working directory `/Users/polakovic/Development/indigo`; macOS arm64; Bash 3.2.57, Python 3.9.6, Git 2.54.0 (Apple Git 157). `./.factory/scripts/security` exited 0 and reported configured Factory security boundaries valid; `git diff --check` exited 0. These structural checks supplement source inspection; they do not certify semantics or authenticate approvals. Final full synthetic/GPS acceptance and independent quality findings belong in [verification.md](verification.md) and [quality-review.md](quality-review.md). This source review supplements those gates; final documentation and record reinspection after the quality verdict is recorded below.

The existing C property tool can return 0 on connection timeout/failure. Its source semantics were inspected; no C change is part of this task. Required value/state checks must therefore validate output as well as exit status. Manual compliance still changes device properties; this review does not establish physical motion safety or rollback of all settings. No physical hardware, Linux/Windows runtime, unrelated driver suite, third-party probing or global absence-of-vulnerabilities claim is made. ShellCheck is unavailable and no pass is claimed.

Reviewed runtime/test-source SHA-256 inventory digest: `e6df999d0fc11276aee2c26648817195cccf13ace522ffb972bd94eee3bbdb3f` (ordered path/content hashes of seven Bash sources and the two Python regression sources).

## Final sign-off after quality and finding reconciliation

The independent final quality verdict is pass. Reinspected the final verification/findings/quality records, maintained documentation and the seven specifically selected archived finding records. All five owner-selected findings and both in-task findings have fixed resolutions, retained task references and no competing active record. The configured findings validator confirms 38 active / 177 archived. No risk acceptance or audit-checkpoint reset is asserted.

Retained final logs explicitly report 33/33 synthetic shell methods in 57.000s and 9/9 GPS methods in 34.789s; the verification report records Factory 190/190 in 134.416s and exact syntax/Xcode/check commands. Their coverage and limits are represented accurately: synthetic script behavior and error handling, not physical device motion or Linux/Windows execution. The final numeric correction remains independently reverified by this reviewer as described above.

Repeated `./.factory/scripts/security`, `git diff --check` and `./.factory/scripts/findings check` all exit 0 after quality/finding reconciliation. No further security issue was identified in the final documentation/record delta. The scoped pass verdict is retained. Archival links/completion metadata, reviewed Git content binding and immutable committed-candidate gates remain coordinator delivery work; a future gate is not claimed as already executed. No merge, push or publication approval is granted by this report.
