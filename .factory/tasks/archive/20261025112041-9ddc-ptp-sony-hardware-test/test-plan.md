# Test and regression plan

Status: final

Hardware: 15 cases of `test_ccd_ptp_hw` (see the driver REFACTOR.md section "Sony ILCE-7RM4 hardware run"). Fake USB: `device_identity_published`, `sony_busy_after_connect`, `sony_dropped_setting_write`, each confirmed to fail without its fix. Full fake USB/ICA suite normally and under ASan/UBSan. No hot-plug (owner decision).
