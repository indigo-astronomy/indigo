# Confirmed findings

## FND-20261026223438-4f82-pentax-terminal-validation-001

Darwin automatic claim leaks a successful capture when raw interface claim fails. The backend increments its capture count in successful detach, but returns claim failure without undoing that capture. Since the core never marks the interface claimed, close does not release it. Impact: system Mass Storage binding can remain detached. Reproduced by compiling the actual backend wrapper with deterministic raw-claim failure; see auto-baseline.log. Fixed by tracking successful capture within that call and restoring it exactly once on failed claim, under the device lock. Failed capture does not trigger restoration. Verified by36/36 normal and sanitized fake tests; actual OS/kernel restoration remains unverified hardware behavior.

The explicit-versus-auto acquisition difference is confirmed, but it is not a proven root cause of physical-99 and is not recorded as a proven physical fix.

## FND-20261026223438-4f82-pentax-terminal-validation-002

Quality review found indigo_libs/Makefile's local Darwin libusb target had no source prerequisites. Existing builds could leave the required failed-claim rollback unbuilt. Fixed by declaring the touched backend source prerequisite. Independently reviewed dry-run with that prerequisite forced newer selects rebuild/install; ordinary supported libusb build exits0. See auto-dependency-dry-run.log and auto-supported-libusb-build.log.
