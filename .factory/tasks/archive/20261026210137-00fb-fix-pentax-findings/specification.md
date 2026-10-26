# Specification

AC-1: Failed USB open returns failure without using or closing an invalid handle; retry remains possible.
AC-2: Both readiness polling modes stop after a monotonic overall deadline, retaining successful and device-error behavior.
AC-3: Camera response lengths cannot cause unbounded allocation, narrowing or out-of-bounds decode. Short data stages cannot be accepted as complete; identity/firmware need four bytes, full state needs every model-specific field extent before any update.
AC-4: Unplug releases each allocated private state exactly once after detach.
AC-5: Preserve valid protocol order and public property behavior, increment driver version, register meaningful fake USB regressions and record final run.

Complexity: high, because USB transport, decoded state and hotplug ownership interact. Risk: medium, including untrusted USB reply memory safety. No API/dependency change. Hardware and other platforms unavailable; no full driver migration or capability certification is intended.

AC-6: Missing, failed or non-terminated USB manufacturer/product strings during hotplug produce bounded, initialized device names. This additional pre-existing Pentax defect was found by independent security review and is repaired within the owner request to fix all Pentax findings.
