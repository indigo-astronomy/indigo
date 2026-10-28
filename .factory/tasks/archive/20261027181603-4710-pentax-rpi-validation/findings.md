# Findings

## FND-20261027181603-4710-pentax-rpi-validation-001

Linux fake output-recovery fixture uses absent macOS/private/tmp. mkdtemp fails, producing false recovery failure. Reproduced test-baseline.log; portable/tmp template preserves isolation/assertions and canonical Linux36/36passes. Test defect, not camera driver defect.

## FND-20261027181603-4710-pentax-rpi-validation-002

Review found PENTAX_HW_ARGS=--baseline could record connection-only success under a plain full-camera type when --type omitted. Add pre-execution recorder guard requiring explicit connection-baseline type (or --no-record), with actual-recorder regressions. The observed hardware run already used the safe distinct type; old full-class failure was preserved.
