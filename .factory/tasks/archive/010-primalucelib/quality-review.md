# Task quality review

Verdict: pass
Reviewer and independence: quality-reviewer role, same agent that implemented the change (no separate reviewer context is authorized in this project); performed as a separate pass over the final diff after documentation.
Reviewed revision and working-tree state: base `80d073bcb`, final uncommitted task diff (library, driver `.driver` and regenerated `.c`, project files, tests, `indigo_uni_io.c`, records) as it stands before the task commits.

## Scope and method
Read the whole diff of:
- `indigo_drivers/shared/primalucelib/`
- `indigo_drivers/focuser_primaluce/` (`.driver`, generated `.c`, `.vcxproj*`, `REFACTOR.md`, `README.md` record)
- `indigo_libs/indigo_uni_io.c`
- `indigo_test/unit/test_primalucelib.c`, `test_uni_io.c`, `integration/test_focuser_primaluce_simulator.c`, `Makefile`
- `indigo.xcodeproj/project.pbxproj`, `MIGRATION_STATUS.md`, `TEST_SUMMARY.md`

Checked against the root, driver, test and libs AGENTS.md rules: formatting, single-line calls, no blank lines in function bodies, generated-driver rules, version and record rules. Also checked the repository-review inspection checklist (duplication, complexity, misleading docs, weak tests, scope drift).

Excluded: the regenerated `.c` was compared mechanically (generator-owned code identical, verification.md C8) rather than read line by line.

## Quality findings
No blocking finding. Observations:

- **Q1, nonblocking, low.** `indigo_focuser_primaluce.driver` `primaluce_open`: the firmware warning still spells "at least 3.05" while the comparison uses `PRIMALUCE_MINIMUM_FIRMWARE_VERSION`. A future change of the constant could leave the message stale. Kept to leave the published message text untouched in a pure extraction. Remediation when the driver is next changed: format the constant into the message.
- **Q2, resolved.** The first design made `test_primalucelib.c` include `primalucelib.c`. With the static library (owner decision) the test links `libprimalucelib.a` like the drivers do, as the unit-test rules prefer.
- **Q3, nonblocking, informational.** The library ships GIOTTO, ALTO and SESTO SENSO 3 functions that no driver calls yet. This is in scope by owner decision. They are marked as INDI-derived and unverified in the header and `PROTOCOL.md`, and unit-tested, so they are not dead code without contract.
- **Q4, nonblocking, informational.** `primalucelib_focuser_settings_done()` casts `const char *` keys to `char *` for the path arrays. The path API stays `char *[]` to accept the existing tables and avoid a C `char **` to `const char **` conversion problem. The keys are never written.

Checks that found nothing:
- **Duplication:** the old driver's ~100 path arrays and request literals are gone, not duplicated (verification.md C10). The two run-config helpers replace 56 repeated assignments.
- **Test strength:**
  - The golden case is proven sensitive by mutation (C6).
  - The two test changes keep their assertions. `status_polling` widens a timing window derived from the documented poll cadence. `handshake_timeout` waits for the attempt to end instead of racing it. Neither hides a driver defect: both fail on Linux with the original driver for test reasons only.
- **Scope:** the libindigo fix and the two test fixes go beyond the extraction, but the owner explicitly chose to include them (handoff.md). They are kept in separate commits.
- **Documentation:**
  - `PROTOCOL.md` marks every command HW, SIM or INDI. Claims that the evidence did not support were downgraded during the pass (access point and credential writes are SIM, not HW).
  - The `REFACTOR.md` record matches verification.md.

## Fix reinspection
- The PROTOCOL.md corrections above were re-read after editing.
- The FND-000049/50/51 fixes are minimal and documented in place. Retests: verification.md C14–C17.

## Quality verdict
Pass, with the nonblocking observations Q1 to Q4 above. The diff does what the specification asks with less code in the driver, and every request is
pinned by a test. The observations are nonblocking and need no registry entry, except FND-000052,
which is a functional finding owned by the coordinator. Limitation: same-agent review.
