# Specification
AC-001: Trutek reply payload must decode to moving0 or an in-range slot. Movement OK requires actual slot equal retained target, both acknowledgement/finalizer; wrong stopped slot or malformed payload yields ALERT. Preserve deadline, lifecycle, BUSY/overlap and correct completion. Increase driver and changed simulator versions; regenerate through unchanged generator.
AC-002: behavior-based synthetic regressions reproduce pre-fix defects and pass after repair; preserve valid/public behavior and existing non-defect test assertions.
AC-003: scoped implementation builds/checks and independent quality/security review pass, with real platform/runtime limits.
AC-004: canonical findings close only with verified evidence; preserve other findings and unfinished full-audit state.
Complexity high where parsers/lifecycles/concurrent observations interact; risk medium for data correctness/test false success, memory and transport inputs. Same host/provider/model inherited; exact model/effort unknown. No budget ceiling or extra charges configured.
