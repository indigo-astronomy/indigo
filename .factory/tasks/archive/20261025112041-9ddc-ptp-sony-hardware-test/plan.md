# Development plan

Status: final

1. Write the hardware suite and its make target; sync the branch to the Pi checkout (its own repository, not origin), build there.
2. Run, diagnose from traces (temporary container dump on the Pi only), fix D-HW1 (INFO identity), D-HW2 (DeviceBusy after connect), D-HW3 (dropped SetControlDeviceA writes) with fake regressions, each confirmed failing without its fix.
3. Rerun fake suites (normal, ASan/UBSan) and the full hardware suite; recorded runs; REFACTOR.md, MIGRATION_STATUS.md; review; commit.
