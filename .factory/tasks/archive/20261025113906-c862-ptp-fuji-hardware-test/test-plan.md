# Test and regression plan

Status: final

Hardware: 15 cases of `test_ccd_ptp_hw` on the X-T1 (see the driver REFACTOR.md section "Fujifilm X-T1 hardware run"). Fake USB: `fuji_bulb_not_offered`, `rejected_setting_readback`, `sony_charger_not_attached`, each confirmed to fail without its fix. Full fake USB/ICA suite normally and under ASan/UBSan. No hot-plug (owner decision).
