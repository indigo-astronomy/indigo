# Independent lifecycle and editor quality review

Verdict: pass

## Scope and independence

Reviewed task 20261026172255-587a-fix-selected-audit-findings, base a35823d61, uncommitted working tree in /private/tmp/indigo-selected-audit-fixes. Reviewer is a separate subagent from the lifecycle and editor implementers; reviewer implemented numerical/RAW files elsewhere in this task and explicitly excluded those files from this pass. Loaded quality-reviewer role, quality-review routine, security-reviewer role and applicable scoped instructions. This is a task-diff review, not a recurring full-repository audit.

Inspected actual additional-instance production diff and complete new lifecycle tests, unchanged guider allocation/shutdown/release implementation, editor mapping diff and new tests, surrounding generator naming/marker contracts, task specification/test plan and developer evidence. Makefile/Xcode registration and overall evidence aggregation remain coordinator-owned and are excluded from this scoped verdict.

## Assessment

- Clone fix changes only the assignment recipient: the slave clone gets its original slave base, while the master clone retains the original master base. Existing allocation/attach/ownership/remove ordering is preserved. Lifecycle tests use the real bus and compile actual framework source; only free observation is substituted. One/two slaves and two clone instances exercise repeated creation, enumeration, connected master/slave rejection, detach order, per-instance data sharing, one free and cleared slots. No production code is mocked away.
- Guider MKGP reset release is already present in base, introduced by c7c82c514f83538e7624ae12dc286bfd40d78a17. It follows stop_process and sits beside RESET_PPEC release. There is no new guider production change to approve. New test counts the independent allocation and release across five actual INIT/SHUTDOWN cycles. Developer mutation evidence demonstrates it catches removal of the historical release; the coordinator must close this as verification of an existing fix, not claim a new source repair.
- Editor property_handler matches generator parse_property_block naming for explicit identity versus device class. Markers and lifecycle names use explicit identity. Exact generated function-name scoping prevents a shorter identity such as ccd from claiming ccd_guide dispatch or handlers. Existing explicit property handler overrides are retained. The mapping remains an association of existing output, without generator edits or new execution/path behavior. Tests cover every CCD simulator logical device and overlapping identities with the same inherited property; no test was weakened.

## Checks and baseline comparison

Independently reran the reported targeted editor command (20 tests): pass, recorded in quality-editor-targeted.log. Independently executed both sanitizer lifecycle binaries with `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1`: 2/2 additional-instance cases and 1/1 guider case pass; quality-additional-sanitize.log and quality-guider-sanitize.log retain output. Exact build/developer baseline commands and limitations are in lifecycle-report.md, editor-report.md and the aggregate verification report.

Inspected editor-baseline.log and editor-final.log directly. The same two template catalog failures and two unsupported-polaralign corpus errors occur in base and candidate; base additionally has the CCD mapping failure now fixed. The full editor suite remains non-green and must not be reported as passing. Its remaining defects are unrelated to these changed mapping functions, and the new targeted checks pass.

## Review disposition and limitations

An early test-hygiene observation (unchecked private-home setup and omitted cleanup) was sent to the coordinator and implementer. Before sign-off, reinspection confirmed both mains now fail when private-home creation fails, retain the test result, remove the isolated home and return that result. Both binaries were rebuilt by the implementer and independently rerun after correction. No unresolved blocking finding remains in this scope.

No hardware, live server, full guider capability/timing test, Linux/Windows run, whole-process leak certification, or whole-repository review is claimed. MacOS ASan leak detection is disabled; explicit ownership counters supply the scoped resource assertions. Build/project registration still requires coordinator review.

## Reviewed content SHA-256

- `indigo_libs/indigo_driver.c`: `53ab3adedbd93d356683e151fe5aaa0c0af87d15e411d4276fc4feeaad829061`
- `indigo_test/integration/test_additional_instances.c`: `36dbcae173792cce3af62849209d275e5ef49d6bf511f31af205473e950973ae`
- `indigo_test/integration/test_agent_guider_cleanup.c`: `a799b7a95e843728154538c483822aab1b7304ee0d11fbc6897b93f6e9a3f61a`
- `indigo_tools/driver_editor/source_map.py`: `0a3460e59c8c133cd78f4edd44bd23b87a8f3b7ada4b91b0273543e8875a4bed`
- `indigo_tools/driver_editor/test_editor.py`: `89e204a2b615a4216caa66c88026ffe7a69d22387e292d61a50364e423fb7a8a`
