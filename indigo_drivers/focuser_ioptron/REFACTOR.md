# iOptron focuser generated migration

Status: steps 1–6 completed, 2026-09-09. Final generated version 0x03000005; 40/40 simulator scenarios and 20/20 targeted ASan scenarios passed. Baseline driver version 0x03000004. Preserve the initial Xcode project ordering edits. No generator changes are authorized or needed; no new hardware capabilities are planned.

## Sources and contract

Read root/test AGENTS.md, driver migration/lifecycle and serial simulator documentation, common/focuser testing standard, Lacerta REFACTOR.md and folder REVIEW.md. No dedicated open iOptron focuser finding is recorded before this work. The existing mount findings are separate. Do not advance the folder review baseline for this scoped migration.

No bundled manufacturer protocol document exists in this driver folder. Official iEAF manual https://www.ioptron.com/v/Manuals/8453_iEAF_Manual.pdf documents zeroing, reversal, relative control and temperature but no wire grammar. Product pages https://www.ioptron.com/product-p/8453.htm and https://www.ioptron.com/product-p/fa20.htm identify iEAF/iAFS hardware. Exact wire grammar comes from existing driver and simulator, not an independently published protocol. Preserve their six-digit identity position + two-digit model + four-digit firmware and seven-digit status position + moving + five-digit Kelvin hundredths + direction. README's seven-digit identity and direction description disagree with code; preserve code's model 2/3 and direction 0=reversed, 1=normal. Preserve space-padded seven-column FM and 0..99999 advertised travel. These hardware assumptions need real-device validation, not fabricated certainty.

Custom ZERO_SYNC violates the X_ property rule: migrate to X_FOCUSER_ZERO_SYNC (SYNC item retained) and document the client-visible rename. No arbitrary coordinate SYNC is supported. Speed, backlash, limits configuration, automatic mode/compensation remain hidden. Relative travel follows coordinate sign irrespective of physical reversal, matching legacy behavior; reversal changes hardware direction mapping.

## Atomic plan

1. Baseline build/test; audit and convert simulator to shared elapsed-time motion. Add model variants, bounded fault fixtures, command journal, direct protocol checks and public-bus regression cases. Record failures before fixing production.
2. Bounded portable full serial transactions, exact fixed-field parsing, transactional open and required status initialization with rollback. Keep hardware startup delay; run malformed/identity/status tests and ASan.
3. Correct measured/target state, no-op/relative clipping, zero/reverse/abort readback, failure recovery, external position and concurrency. Validate every supported control against the class matrix.
4. Migrate directly to authoritative .driver and generated lifecycle/handler queues with named motion finalizers; combine queue and generator transition to avoid duplicate temporary ownership. Preserve generator defaults, no MAX_DEVICES override, increase version.
5. Validate independent instances, pending operation disconnect/reconnect, parser and operation faults, no stale completions, firmware/model metadata. Run complete applicable simulator suite and targeted ASan. Strict checks must include -Wconditional-uninitialized, -Wconversion, -Wshorten-64-to-32 and -Werror at O0/O2 on both macOS architectures.
6. Synchronize generated outputs, Xcode and existing Windows projects, README, PROPERTIES, CHANGES, reviewed findings and MIGRATION_STATUS. Verify reproducible generation, diff hygiene and cleanup. Record each result here. Hardware, Linux/Windows build/runtime and x86_64 runtime are separate unverified acceptance.

## Coverage requirement

Map common/focuser standard to named tests in CHANGES.md: interface/visibility; both model identities; initial/status malformed, partial, missing and overlong frames; absolute/relative/direction/reversal, limits/no-op; zero without FM; abort idle/moving then fresh movement; stop/start/read failures; external position/temperature; overlapping requests; disconnect pending read/motion; reconnect and independent instances. Unsupported controls and generic framework validation are non-applicable. Do not claim exhaustive C branch coverage or physical timing from PTYs.

## Step 1 progress

Baseline universal production/test build and the existing smoke scenario passed. Simulator now uses serial_motion at 5000 coordinate steps/s (minimum 0.5 s nonzero move), with both model profiles, split responses, a private journal and per-command one-shot faults. Motion progresses independently of status queries. Added an isolated test runner with child watchdog and simulator reaping, fresh property revisions, archive dependency and driver-instrumented ASan target. The new custom property name is intentionally asserted only against migrated production; baseline regressions use unchanged existing properties.

Step 1 baseline regressions: both independent simulator protocol cases pass against model 2/3. `init_status` fails on the handwritten driver because connection succeeds without validating FI. `poll_badflag` fails because moving=2 is accepted by sscanf instead of rejected. These are production regressions; no expected-failure exemptions were added. The simulator/test fixtures are independently validated before migration.

Steps 2–4 implementation: exact 12/14-byte terminated responses and decimal field widths, full writes, bounded input drain/deadline, status initialization rollback and generated queue ownership are implemented in authoritative DSL. Local outputs are explicitly initialized for Xcode aggressive warning checks. Kept 115200 baud and two-second startup settling on the queue. Generated single-device init failure requires an explicit close in on_connect; low-level open rolls back its own failure. Version increases from 0x03000004 to 0x03000005. Pending motion is completed by motion_finalizer, idle polling cannot overwrite a queued target, and stop/zero/reversal check FI readback. Testing of the transition follows.

## Step 5 intermediate validation

First generated transition: all 34 registered scenarios passed. Targeted ASan filters init_ (5), poll_ (9), instances (1), disconnect (2) passed 17/17. Strict universal O0/O2 compiler checks including Xcode conditional-initialization warnings passed.

A subsequent ownership audit added six regressions: zero/reverse/abort lost status readback, moving status failure, stalled motion and pending zero versus new movement. Fresh status before reversal prevents an unnecessary second toggle after a lost reply. Failed zero readback marks coordinate knowledge uncertain until confirmed status. Position/steps dispatch also guards pending zero/abort, preventing their completion from overwriting a newly copied movement target. These are identified cross-property operations, not duplicated same-property framework guards. Final expanded suite and ASan are running.


## Final acceptance and Step 6 result (2026-09-09)

- Final production universal macOS arm64/x86_64 compilation and archive/dylib/executable linking passed. Simulator and ordinary/ASan test builds passed. All **40/40 ordinary scenarios** passed on native macOS arm64 with zero failures, including all six added ownership/recovery regressions.
- Final targeted ASan passed **20/20 scenarios**: readback_failure (3), init_ (5), poll_ (9), instances (1), disconnect (2). Production driver C and test are instrumented; the framework remains the normal library. No ASan finding occurred. Baseline init_status/poll_badflag regressions are fixed, with descriptor-count rollback/retry assertions.
- Strict generated C checks passed for arm64/x86_64 at O0 and O2 with `-Wall -Wextra -Wconversion -Wshorten-64-to-32 -Wconditional-uninitialized -Wunreachable-code -Wcomma -Werror`. Standalone simulator `-std=c11 -Wall -Wextra -Werror` check passed. No warning suppression was added.
- Authoritative DSL and C/header/main reproduced byte-for-byte across two additional generator invocations. Generated change handlers/finalizers and connection failure/disconnect ownership were inspected; no generator change or MAX_DEVICES override was introduced. Version is higher than baseline.
- Added DSL/refactoring notes to the existing Xcode group and Windows project/filter. Xcode `plutil -lint` and Windows XML/reference validation passed. Existing user Xcode ordering edits were preserved. No new solution entry was needed because the Windows project was already present.
- README documents supported models, corrected wire-reference assumptions, software failure/stall policy, generated port-selection defaults and the client-visible ZERO_SYNC → X_FOCUSER_ZERO_SYNC rename. PROPERTIES, simulator inventory, CHANGES coverage matrix and root MIGRATION_STATUS agree. DRV-101/102 record scoped fixed findings; folder review baseline remains unchanged.
- Full applicable common/focuser behavior coverage is mapped to named scenarios in `indigo_test/CHANGES.md`. Hardware wire grammar/polarity and physical motor behavior remain unverified; official manuals do not provide an independent wire specification. Linux/Windows compilation/runtime and x86_64 runtime were not performed. These are explicit acceptance gaps, not passing simulator claims.

Reproduction from repository root:

```sh
build/bin/indigo_generator indigo_drivers/focuser_ioptron/indigo_focuser_ioptron.driver
make -C indigo_drivers/focuser_ioptron -f ../../Makefile.drv
make -C indigo_test build/integration/test_focuser_ioptron_simulator build/integration/test_focuser_ioptron_simulator_asan
```

Run from `indigo_test`:

```sh
./build/integration/test_focuser_ioptron_simulator
for filter in readback_failure init_ poll_ instances disconnect; do
  IOPTRON_TEST_FILTER="$filter" ./build/integration/test_focuser_ioptron_simulator_asan || exit 1
done
```

Final hygiene: `git diff --check` passed; `make -C indigo_test test-clean` removed the test build tree. Test parents reaped their PTYs/simulators; task-owned temporary logs were removed. No commit was created.


## Follow-up: preferred uniform I/O (2026-09-09)

User requested replacing custom wait_for_data/read_available drain/read loops. Version raised from 0x03000005 to 0x03000006. The DSL now uses indigo_uni_discard, indigo_uni_printf and indigo_uni_read_section2 with explicit first/inter-byte timeouts. The 32-byte response buffer is shared in private data; helpers decode before subsequent commands reuse it. Terminator is retained for completeness checks, embedded NUL and truncated replies are rejected, and capacity minus one reserves the API's trailing NUL. No platform or generator changes.

iOptron retains exact fixed-width identity/status validation after stripping the # terminator; write-only commands remain write-only.

Validation pending: regenerate, universal build, full simulator suite, targeted ASan and strict warnings; results will be appended here.

Shared-I/O regression found during iOptron verification: indigo_uni_discard is documented to discard input, but its serial implementation used TCIOFLUSH / PURGE_RXCLEAR|PURGE_TXCLEAR, dropping pending output too. A readback query could therefore erase the preceding write-only Z/R/Q command. Corrected the shared implementation to TCIFLUSH / PURGE_RXCLEAR, preserving transmitted commands without adding platform code or timing sleeps to drivers. Existing zero/reverse/abort readback tests reproduce the defect; full suites are rerun with the corrected library.

User-directed variadic command pattern applied: command helpers accept format strings/arguments and use va_start/va_end with uni_vprintf (or uni_vtprintf for Prodigy newline termination). Numeric motion/settings arguments are formatted at send time. Repository rule added to AGENTS.md. Full tests are rerun on this final transport form plus the input-only discard fix.

Final variadic transport build checkpoint: all three drivers compile under strict O0/O2 arm64/x86_64 flags. Lacerta required explicit zero initialization of decoded position locals after separating command and response parsing; failed queries still return before publication. This warning-only refinement is separately smoke-tested after the full-suite run. The corrected input-only discard restores iOptron zero/reverse/abort readback regression cases.

Targeted ASan on the final variadic/input-only-discard implementation: 20/20 scenarios pass (readback_failure, init_, poll_, instances, disconnect), including all previously failing write-only-command/readback cases. Full ordinary suite remains in progress; strict compilation and reproducible generation pass.

Preferred-I/O follow-up complete: full 40/40 simulator scenarios and targeted 20/20 ASan pass on the final variadic transport and shared input-only discard implementation. Strict O0/O2 arm64/x86_64 checks and byte-identical regeneration pass. Version 0x03000006 is higher than pre-follow-up 0x03000005. Existing MIGRATION_STATUS status columns remain correct and Comment is preserved. Hardware/Windows/Linux runtime limits remain unchanged.
