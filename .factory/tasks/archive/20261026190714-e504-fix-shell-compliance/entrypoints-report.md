# Shell compliance entrypoint repair

## Assignment and implementation

Developer subagent; base `66d6815ddc2882895ebc351807977ad008b8274c`, shared task branch. Assigned writes are the six existing compliance entrypoints and this report only. No framework, test fixture, driver, build-system, documentation, branch or commit mutation was performed by this writer. Hardware and live-server execution are not performed.

Each entrypoint snapshots its original connection state after its interface check and before its connection battery can disconnect an originally connected device. Shared `preserve_connection_state` owns EXIT/INT/TERM cleanup. Normal completion and early range/count failures call `restore_connection_state` before the summary. Existing final disconnect/no-op disconnect assertions are retained, then originally connected devices are reconnected. Snapshot failures are already counted by the shared helper and are not counted twice. The former `WAS_CONNECTED=$?` assignment no longer overwrites the pre-mutation snapshot.

Focuser and rotator abort scenarios use shared `test_abort_motion` instead of a two-second sleep. Neither backgrounds a property-tool process. The focuser selects the more distant valid numeric endpoint. The rotator selects a valid half-turn when its actual bounds permit one, otherwise the more distant endpoint of a shorter range. This avoids0/360 equivalent endpoints and invalid wrapping outside nonstandard ranges. Failed, malformed, nonfinite or out-of-range current readbacks fail before requesting movement. Existing post-abort position/steps not-BUSY assertions remain.

Filter-wheel direct writes use `test_set_and_verify`, so a failed set cannot pass merely because the expected old value remains present. Interpolated-device `eval` storage was replaced with Bash3 indexed arrays: device names and saved text stay data. All six executable defaults are quoted. Remote argv and delay handling remain owned by the framework.

## Baseline observations and validation

The original five non-GPS footers disconnect unconditionally and ignore the battery's initial-state return. GPS restores only on normal completion. Focuser uses a two-second delay; rotator also backgrounds its BUSY waiter without reaping it. The original rotator half-turn subtracts360 after exceeding the upper bound without checking the lower bound. These are source-confirmed baseline defects; no hardware reproduction is claimed.

Working directory `/Users/polakovic/Development/indigo`; macOS arm64, system Bash. `bash -n <path>` exits0 for each assigned script. `git diff --check -- indigo_test/shell_compliance` exits0.

A Python subprocess check extracted the actual awk target selectors and ran them directly. Focuser cases `(current,min,max)->target`: `(0,0,100)->100`, `(100,0,100)->0`, `(20,10,30)->30` passed3/3. Rotator: `(0,0,360)->180`, `(350,0,360)->170`, `(150,100,200)->200`, `(200,150,250)->250` passed4/4. Each selector rejected `garbage`, `nan`, `inf` and out-of-range9999 (8/8 negative checks). No property-tool invocation occurred.

## Handoff and limits

Agreed framework contracts: `preserve_connection_state DEVICE TIMEOUT` returns0 valid snapshot or1 already-reported failure; `restore_connection_state` returns0 success/noop or1 already-reported failure; `test_abort_motion NAME SET_QUERY MOTION_PROPERTY ABORT_QUERY TIMEOUT` returns0 exercised/pass,1 reported failure,2 honest reported skip. Full synthetic entrypoint integration remains with the parent/regression writer. No all-six runtime pass is claimed before those tests execute.

Source scopes are released to the parent. Parent will apply required standard license/AI headers consistently across seven Bash sources, retaining original copyright/description, then repeat syntax and synthetic suites. Further changes should follow concrete regression evidence.

## Additional failed-read audit and corrections

The shared scalar getters preserve subprocess status but cannot update parent-shell failure counters when called inside command substitution. All ordinary bare property/range assignments in the six entrypoints were therefore replaced with parent-shell `capture_property_value` calls. A failed required read records FAIL, restores the saved connection, prints a summary and exits before further mutation. Numeric saved values/readbacks use `get_item_number`; legitimate empty text values remain valid. Wheel arrays receive checked scalar temporaries because macOS Bash3 does not support `printf -v` assignment to an array element. Its private slot-count converter, which masked getter failure, was removed; the directly checked maximum must convert to a positive integer count.

AO/guider unusable pulse-limit branches now record FAIL instead of echo-only errors. Missing required focuser steps cannot fall back silently to1000. Optional-property SKIP branches execute only for confirmed absence(status1), not command errors(status3). An originally empty wheel label is restored after testing rather than skipped. GPS caches its optional OFFSET presence from the initial existence check so a failed read is caught while absence is counted only once.

An intermediate nine-method GPS rerun exposed four expected integration failures after fail-fast reads were introduced: three missing-mandatory-item subcases lacked the normal restoration header, and the optional-offset case counted its absence twice. The source now prints the restoration header on fail-fast cleanup and reuses the cached OFFSET presence. Those fixes preserve the existing test expectations; no assertion was weakened. A targeted correction rerun is recorded by the parent/final verification. The earlier32-method new synthetic run passed, but source changed during it, so it is diagnostic rather than frozen-candidate acceptance.

Six entrypoint source files are released again after syntax and scoped whitespace checks. Root owns the final header application and full frozen-source synthetic runs. No runtime source edits are planned by this writer unless verification identifies a new concrete failure.

Targeted correction verification: `python3 -B indigo_test/unit/test_gps_compliance_script.py GPSComplianceScriptTests.test_utc_offset_remains_optional GPSComplianceScriptTests.test_mandatory_property_failure_returns_nonzero_after_restoration` exits0,2/2 methods passed(11.901s), including its three missing-mandatory-item subcases. Actual shell is `/bin/bash`3.2.57 arm64; no new runtime assertion or fixture expectation change was needed. Scoped `git diff --check` remains clean.
