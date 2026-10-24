# Specification

AC-001: Emit N/S/E/W as 0/1/2/3 in unchanged :Mg%c%4d# framing. Do not swap N/S or change lifecycle/timing/property contracts. Increment driver version 9 to 10 and regenerate with unchanged generator.
AC-002: Turn numeric protocol reproducer into ordinary passing regression coverage and retain equivalent direction/lifecycle/error coverage.
AC-003: Verify actual motor response through the repaired driver on owner hardware with idle controls; retain known DEC orientation qualification and restore park/tracking/R6.
AC-004: Record final tests through run_driver_test.py, update test records and narrowly relevant docs.

Complexity medium for bounded protocol fix, high uncertainty only in existing physical orientation; risk medium bounded hardware motion. No generator modifications or full driver migration.
