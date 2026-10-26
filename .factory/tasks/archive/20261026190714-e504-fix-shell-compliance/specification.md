# Specification

- AC-001/FND-000031: executable paths, device names and remote arguments remain exact argv entries; nonzero tool exits never become a successful check through filtering, including valid-looking stdout/stderr.
- AC-002/FND-000032: all six compliance entry points restore the original connection state on normal and trapped exits; originally connected devices are not left disconnected. Preserve connection checks and report cleanup failures.
- AC-003/FND-000033: focuser/rotator abort scenarios use bounded observed BUSY synchronization, send abort immediately and reap any started helper. Fast completed motion must not cause a false required BUSY abort failure; missing BUSY/transport failures must be diagnosed honestly.
- AC-004/FND-000034: exact literal property/item matching and anchored numeric range parsing reject unrelated substring matches and stray diagnostic ranges. Respect real indigo_prop_tool list/get/state formats.
- AC-005/FND-000035: documentation describes passed/failed/skipped totals and corrected invocation, cleanup and abort behavior.

Preserve optional-property skips, command semantics, class coverage and existing GPS regression. Shell compliance remains opt-in; no live server/hardware is selected automatically. No new dependencies. Complexity high (exit status, async process lifetime and cleanup); risk medium (manual scripts can move/disconnect equipment). No model/provider switch or budget supplied.
