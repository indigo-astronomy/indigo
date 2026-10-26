# Motion deadline regression coverage

Finding: `FND-20261025211159-e308-fix-audit-findings-batch-004`.

## Assignment and limits

Developer/test-designer subagent; same-provider context separate from coordinator. Scoped files: Temma/NexStar `.driver` and regenerated C, their existing simulator test C files, Temma host simulator and the two `REFACTOR.md` files. Coordinator owns build registration, migration counts, final recorded runs and finding disposition. No generator change, public property change, default runtime behavior change, physical hardware run, shell compliance change, or commit.

## Behavior and implementation

Existing generated-driver deadlines are now enclosed in `#ifndef` guards: production remains Temma 600 seconds (GOTO and park), NexStar 300 seconds (park). Separate deadline test binaries compile the same driver with one-second limits, real queues, transport and timers. They reuse the existing suite source under `INDIGO_TEST_MOTION_DEADLINE` and select only the new cases; ordinary suites retain their original cases and defaults. No driver version increase is needed for an unchanged production default and test-only build seam.

Temma simulator gains deterministic `stall`/`resume` control which freezes actual slew completion and coordinates while leaving normal status exchange alive. Default simulator behavior is preserved. This simulator has no version constant. NexStar uses its existing command-selective `stall` injection; its normal stop clears the stall.

New tests: `temma_goto_deadline_stops_and_recovers`, `temma_park_deadline_stops_and_recovers`, `nexstar_park_deadline_stops_and_recovers`. Assertions require BUSY then fresh ALERT, the specific timeout message, elapsed deadline, exact GOTO cancellation (Temma PS / NexStar M plus axis stop commands), unchanged stalled position, unparked state, and a fresh successful operation without another cancellation.

## Commands and evidence

Base: `bab591d40`; macOS arm64 execution; universal x86_64/arm64 builds. Source checks run against the task working tree. Default test binary commands below run from `indigo_test/` because simulator paths are relative to that directory.

- `make -C indigo_test build/integration/test_mount_temma_simulator build/integration/test_mount_nexstar_simulator`: exit 0, baseline build (`/tmp/indigo-deadline-baseline-build.log`).
- `INDIGO_TEST_CASE_FILTER=temma_sync_goto_overlap_abort_and_recovery build/integration/test_mount_temma_simulator`: exit 0, 1/1 baseline (`/tmp/indigo-temma-baseline.log`).
- `INDIGO_TEST_CASE_FILTER=nexstar_goto_completion_follows_slew_status build/integration/test_mount_nexstar_simulator`: exit 0, 1/1 baseline (`/tmp/indigo-nexstar-baseline.log`).
- Initial invocations from the repository root failed because simulator-relative paths did not resolve; rerunning from the suite's documented working directory produced the passing baselines above. These setup errors are not product failures.
- `make -C indigo_test build/integration/test_mount_temma_deadline build/integration/test_mount_nexstar_deadline`: exit 0 (`/tmp/indigo-deadline-build.log`).
- `build/integration/test_mount_temma_deadline`: exit 0, 2/2 after timeout-message assertions (`/tmp/indigo-temma-deadline.log`).
- `build/integration/test_mount_nexstar_deadline`: exit 0, 1/1 after timeout-message assertions (`/tmp/indigo-nexstar-deadline.log`).
- Negative control: take each deadline compile command from `make -n -B`, replace only `-DTEMMA_SLEW_TIMEOUT=1` with `=600` / `-DNEXSTAR_PARK_TIMEOUT=1` with `=300`, write binary under `/tmp/indigo-{temma,nexstar}-unshortened-deadline`, and run from `indigo_test/`. Both builds exit 0; Temma exits 1 with both cases failing at the expected missing ALERT wait, NexStar exits 1 at the same wait. Logs: `/tmp/indigo-{temma,nexstar}-unshortened-{build,deadline}.log`. This control confirms the tests actually depend on expiry rather than an injected communication error. No tracked source mutation was used.
- Regeneration check: SHA-256 hashes of each generated `.c`, `.h`, `_main.c` unchanged after rerunning `build/bin/indigo_generator indigo_drivers/mount_<driver>/indigo_mount_<driver>.driver`: pass for both. Generated C diffs contain only the matching two guard lines; headers/main outputs unchanged.
- Scoped `git diff --check`: exit 0.

Ordinary full-suite diagnostic runs are in progress; coordinator will perform final recorded runs through `tools/run_driver_test.py` and retain their outcomes. Expected combined counts: Temma 30 simulator cases on macOS (25 on Linux), NexStar 39; hardware unchanged. New persistent files: none. Xcode registration is not required for the reuse of existing C source.

## Dependency qualification and ordinary diagnostics

Coordinator subsequently identified the pre-existing `libindigo` artifact as older than current source. All subagent diagnostic runs above used that artifact; they are comparative test-development evidence, not final acceptance of the current library source. Coordinator has rebuilt with `make -C indigo_libs all` and will run final recorded suites against it. Temma ordinary full suite `build/integration/test_mount_temma_simulator` passed 28/28, exit 0 (`/tmp/indigo-temma-ordinary.log`). NexStar ordinary full suite `build/integration/test_mount_nexstar_simulator` passed 38/38, exit 0 (`/tmp/indigo-nexstar-ordinary.log`). All source writes in this assignment are complete; final recorded acceptance is assigned to the coordinator.
