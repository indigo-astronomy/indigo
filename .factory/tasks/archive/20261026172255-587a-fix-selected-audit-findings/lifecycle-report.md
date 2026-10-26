# Lifecycle repair evidence

## Scope and reviewed state

Developer subagent; source/test checks are not independent approval. Worktree `/private/tmp/indigo-selected-audit-fixes`, base `a35823d61`, branch `task/20261026172255-587a-fix-selected-audit-findings`. macOS arm64, Apple clang. Root/scoped implementation, test and driver instructions and assigned developer role loaded. No hardware used; this is a bounded allocation/lifecycle regression, not a full guider driver run or timing certification.

## AC-001 / FND-20261023101753-bc8d-complete-repository-audit-043

`indigo_libs/indigo_driver.c` now sets `additional_slave_device->base_device = slave_device` and leaves the cloned master's original base unchanged.

New `indigo_test/integration/test_additional_instances.c` compiles the actual framework implementation, uses a synthetic master and one/two slaves on the real in-process bus, and observes only framework free calls. Its two cases each perform three create/enumerate/remove/recreate cycles, with two cloned instances per cycle. Assertions cover master/slave base identities, cloned slave-to-master linkage, one private-data allocation shared within each clone pair but distinct between instances and original, property enumeration, connected master/slave removal rejection, slave-before-master detach ordering, one shared-private-data free per instance, cleared instance slots and original identity preservation. Configuration is isolated with private HOME and removed after the test.

Before the production edit the binary exited 1: both cases failed, with 30 base-identity assertions. Other lifecycle checks still completed and cleaned up. After the one-line edit both cases pass normally and under ASan/UBSan.

## AC-003 / FND-20261024121920-3bd2-repository-audit-034

The requested release was already present at task base: `indigo_release_property(AGENT_GUIDER_RESET_MKGP_PROPERTY)` at line 3288, introduced by retained commit `c7c82c514f83538e7624ae12dc286bfd40d78a17` (`indigo_agent_guider: release the Multi Kernel GP reset property when the agent detaches`). Driver version is already `0x03000036`. No guider production change/version bump is needed; reconcile the stale finding after verification.

New `indigo_test/integration/test_agent_guider_cleanup.c` compiles the actual handwritten agent, runs five real driver INIT/SHUTDOWN cycles, and counts its MKGP reset switch allocation and release by pointer. It verifies one allocation and one release per cycle, no surviving tracked property between cycles, and clean bus stop. No synthetic stop/detach implementation replaces the behavior under test. This case passes against task base and with ASan/UBSan. A disposable mutation removing only the existing MKGP release exited 1 at `live_reset_property == NULL`, proving the regression detects the historical omission. The mutation did not modify repository source.

## Exact developer commands and results

Working directory for all commands: `/private/tmp/indigo-selected-audit-fixes`. The normal compile commands were:

```sh
/usr/bin/clang -g -O1 -arch arm64 -DINDIGO_MACOS -Duint=unsigned -D_GNU_SOURCE -Iindigo_libs -I. -Ibuild/include indigo_test/integration/test_additional_instances.c -o /private/tmp/test_additional_instances -Lbuild/lib -Wl,-rpath,/private/tmp/indigo-selected-audit-fixes/build/lib -lindigo -lusb-1.0 -framework IOKit -framework CoreFoundation
/private/tmp/test_additional_instances
/usr/bin/clang -g -O1 -arch arm64 -DINDIGO_MACOS -Duint=unsigned -D_GNU_SOURCE -Iindigo_libs -I. -Ibuild/include indigo_test/integration/test_agent_guider_cleanup.c -o /private/tmp/test_agent_guider_cleanup -Lbuild/lib -Wl,-rpath,/private/tmp/indigo-selected-audit-fixes/build/lib -lindigo -lindigocat -lm -lpthread
/private/tmp/test_agent_guider_cleanup
```

Both compile and execute commands exit 0; results 2/2 and 1/1. The pre-fix additional-instance binary used identical arguments with output `/private/tmp/test_additional_instances_baseline`, and execution exited 1 (2/2 cases failed as expected). The earlier initial fixture attempt had not enumerated the original properties and was corrected before recording the actual baseline; property definition is required for the production changeable-property contract.

Sanitizer builds repeated these exact compile commands adding `-fsanitize=address,undefined -fno-omit-frame-pointer` before the source, with output suffix `_sanitize`. Both were executed as:

```sh
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 /private/tmp/test_additional_instances_sanitize
UBSAN_OPTIONS=halt_on_error=1 ASAN_OPTIONS=detect_leaks=0 /private/tmp/test_agent_guider_cleanup_sanitize
```

Both exit 0; 2/2 and 1/1 pass with no sanitizer diagnostics. Sanitizer build warnings are existing deprecated `sprintf` calls in included production source (five framework, four agent); these calls were not changed. ASan leak detection disabled because this macOS runtime does not provide a whole-process leak gate; explicit resource counters establish the scoped ownership assertions. Linked prebuilt framework dependencies are not sanitizer-instrumented, while the changed framework implementation and actual agent source included in the test translation units are instrumented. Linux/Windows and physical hardware were not run.

## Parent integration handoff

Parent owns Makefile/Xcode/task finding reconciliation. Suggested rules:

```make
$(INTEGRATION_BUILD)/test_additional_instances: integration/test_additional_instances.c test_runner.h ../indigo_libs/indigo_driver.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(FRAMEWORK_TEST_LDFLAGS)

$(INTEGRATION_BUILD)/test_agent_guider_cleanup: integration/test_agent_guider_cleanup.c test_runner.h ../indigo_drivers/agent_guider/indigo_agent_guider.c | $(INTEGRATION_BUILD)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS) -lindigocat
```

Register both in the integration test list and Xcode integration group. Add a sanitizer target if desired using the project's existing architecture-filtering sanitizer conventions. The new guider test adds one hardware-free integration case; the current MIGRATION_STATUS row reads 88 / 0, so parent should reconcile it to 89 / 0 if that row's inventory is unchanged elsewhere. The driver itself remains untouched; no refactoring or transport/algorithm change was performed. No README or generated source edits. No commit/merge/push made by this subagent. Ready for independent review.
