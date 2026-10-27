# Security review

Verdict: pass
Scope of verdict: PASS for the scoped test portability and connection-baseline changes. This is not full camera certification or evidence that the macOS -99 failure is resolved.

## Scope and independence

Independent security-reviewer pass at base aabaed8ec963631cca9ba37826b1777339cc43e7 plus this task's uncommitted diff in /private/tmp/indigo-pentax-rpi. The reviewer did not implement these changes or execute SSH, sudo, USB, camera, or remote-server commands. Reviewed the current source diff, adjacent fixture/hardware control flow, brief, specification, plan, test-plan, handoff and available documentation. Only this report is written by the reviewer.

Trust boundaries are the local temporary test fixture, explicitly selected physical test profile and owner-authorized remote execution. The owner supplied SSH agent-forwarding access and non-interactive physical-test authorization; forwarding is existing explicit authority, not an added feature or credential grant in this diff. Remote execution details and actual physical results remain the coordinator's evidence, not independently observed by this reviewer.

## Findings and controls

No new blocking security finding was identified.

- The fixture changes from /private/tmp to /tmp while retaining mkdtemp with a randomized template. The resulting private directory isolates synthetic output and the deliberate blocked-path fixture. Fixed child paths remain inside that directory; cleanup removes only the recorded fixture outputs. This does not introduce a predictable shared output file or production filesystem change.
- PENTAX_HW_ARGS defaults to --run, preserving the existing full physical group. The existing --baseline mode requires explicit selection; the executable accepts exactly one recognized argument. The Make variable is trusted developer/test configuration, with the same command-execution trust as other make inputs, not a new untrusted runtime input.
- Baseline mode skips control changes, exposures and restore writes, while retaining explicit disconnect. Its distinct case name makes a limited run visible. The coordinator must retain the distinct recorded type `PENTAX K-m (connection baseline)` so its pass cannot overwrite or imply a full exposure-suite pass.
- No production driver, privilege policy, USB permission, entitlement, public API/ABI, dependency package or persistent remote installation is changed. The isolated matching framework/server build is a compatibility measure; it does not justify changing the installed checkout. Existing unrelated Mac solver changes remain outside scope.
- The 60-second existing hardware watchdog is retained. A watchdog termination or disconnect failure must remain failure and cannot prove restoration. No physical hotplug is covered by this review.

## Checks and evidence limits

Working directory /private/tmp/indigo-pentax-rpi:

- `git diff --check`: exit 0.
- `./.factory/scripts/security`: exit 0, configured Factory local boundary checks passed. These checks do not scan native USB behavior or certify a remote host.

Inspected actual Makefile, fixture and hardware baseline code. Coordinator reports the old2.0.0.0 baseline and isolated current3.0.0.4 actual-server connection/disconnect succeeded, and canonical baseline profile passed1/1 with no exposures or physical hotplug. At this review's snapshot verification.md was still being populated, so final exact commands, logs, framework compatibility details, test completion and remote cleanup confirmation must be retained there by the coordinator. No SSH secrets, agent socket contents or private image data should be stored in task evidence.

## Next action

Retain version-labelled connection-only evidence and the distinct profile label; complete Linux fake-suite and final documentation/quality checks. Confirm owned servers/test processes are stopped and leave installed software untouched. Real native image transfer, short/bulb exposure, abort/recovery, physical timing, unplug and macOS connection remain unverified. Notify the owner and obtain readiness before any later battery-sensitive hardware group. No publication or integration authority is inferred from this review.

## Final recorder and evidence recheck

Reinspected final verification.md and the additional tools/run_driver_test.py/unit-fixture diff. The final report now retains the exact isolated sudo/LD_LIBRARY_PATH canonical command, matching-framework compatibility evidence, current driver version, distinct connection-baseline type and owned-server termination. The prior note about an unfinished verification template describes the earlier review snapshot and is superseded by this completed evidence review.

FND-20261027181603-4710-pentax-rpi-validation-002 is addressed for the documented PENTAX_HW_ARGS selection from the environment and explicit `PENTAX_HW_ARGS=...` make arguments. Explicit arguments take precedence. Recorded Pentax baseline mode requires a type containing `(connection baseline)` before build or physical execution; --no-record remains available. The guard uses shlex only to parse trusted test configuration and does not evaluate shell commands. This improves record integrity; it is not a security boundary against a contributor who controls arbitrary Make targets or modifies result files. Existing default full-mode selection remains unchanged.

This reviewer independently ran `python3 -B indigo_test/unit/test_driver_test_recording.py`:11/11 passed, using isolated synthetic fixtures and no SSH/hardware. Repeated `git diff --check` and `./.factory/scripts/security`: both exit0. Cases reject omitted/plain baseline labels before target execution, check environment selection, retain explicitly labelled records, permit no-record and preserve command-line full-mode override. Coordinator owns final finding registry dispositions and Linux test evidence.

Scoped verdict remains PASS. No new blocking security finding. A connection-only pass remains separate from full capture/exposure acceptance, and no additional physical rerun is required for this recording-only guard. The proposed task-scoped local commit does not authorize merge/push, alter installed remote packages, or change the production driver. No SSH, credential access or camera use occurred during either security review pass.

## Final literal-profile hardening and synchronized-base review

Final reviewed base is now32d7396d7257e33a3afc3520990151022c6fafcc after task synchronization. Hardware evidence remains explicitly tied to the isolated aabaed8ec build; the synchronization does not retroactively certify that different server revision on hardware. The supplied integration changes concern solver registration, not the unchanged Pentax driver/CCD control path. No additional hardware use is claimed or needed to verify this recorder-only guard.

Reinspected the strengthened recorder guard. For recorded Pentax hardware, it rejects alternate GNU Make assignment forms and requires the effective profile to parse as one literal --run or --baseline argument. This prevents accidental make-expanded/multi-command profiles from bypassing the distinct baseline-label check. Invalid quoting is rejected before build/execution. Standard explicit assignments retain precedence over the environment; full mode stays the default. Arbitrary Make target/file control remains trusted developer authority rather than a hostile-input boundary.

Independently reran `python3 -B indigo_test/unit/test_driver_test_recording.py`:13/13 passed. `git diff --check` and `./.factory/scripts/security` passed again. These were local synthetic checks only. This13-case final result supersedes the earlier11-case recorder result for the final guard. Coordinator retains the corresponding final Linux recorder/fake-suite evidence and registry dispositions.

Final scoped security verdict remains PASS, with no blocking issue identified in the literal-profile hardening. Existing connection-only hardware scope, physical-restoration uncertainty, macOS limitation and prohibition on unrequested merge/push remain unchanged.
