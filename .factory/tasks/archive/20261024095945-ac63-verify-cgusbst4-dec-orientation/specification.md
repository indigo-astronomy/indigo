# Specification

AC-001: Establish exclusive device access, current mount identity/firmware, parked baseline and original settings.
AC-002: At a stable non-polar position, compare numeric adapter codes 0/1 against direct NYX n/s pulses using actual reported DEC changes, repeat both signs on stable pier side, include idle controls.
AC-003: Record commands, coordinates, motor steps, side, interpretation and limits. Restore original park, tracking off and R6, with original park coordinates preserved.
AC-004: Preserve N/S/E/W mapping and all production source bytes.

Complexity high: physical motion, near-pole coordinate transform and ST4-versus-serial conventions; risk medium physical mount movement, bounded/authorized excursion required. No inference of electrical wiring from software acknowledgements alone.
