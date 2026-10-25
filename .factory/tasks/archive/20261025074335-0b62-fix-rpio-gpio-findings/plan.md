# Development plan

Status: final

1. Ownership bits in `rpio_sysfs`, set by export, honoured by unexport and rollback.
2. Pulse finalizer propagates a failed OFF write in both `.driver` files; versions bumped, regenerated.
3. Fake helpers and regression cases in both sysfs suites; one hardware case in `test_aux_rpio_hw.c`.
4. Recorded runs on the Raspberry Pi 5 and the Raspberry Pi 3B+, REFACTOR.md, MIGRATION_STATUS.md, findings.
