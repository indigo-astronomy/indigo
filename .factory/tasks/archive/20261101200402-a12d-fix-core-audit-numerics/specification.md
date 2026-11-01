# Specification
AC-001: Reject malformed/nonfinite Gaia fields without hangs; bounded normalization preserves finite inputs and null proper motions. Serialize SER UTC/local timestamp arithmetic in explicit 64-bit types without LLP64 narrowing. Define safe narrow Bayer behavior for1x1/1xN/Nx1 in ordinary/stretched8/16-bit paths, preserving larger-image output. Accept complete valid JSON numeric grammar including positive exponents; reject malformed/truncated numeric tokens without silently dispatching prefixes.
AC-002: behavior-based synthetic regressions reproduce pre-fix defects and pass after repair; preserve valid/public behavior and existing non-defect test assertions.
AC-003: scoped implementation builds/checks and independent quality/security review pass, with real platform/runtime limits.
AC-004: canonical findings close only with verified evidence; preserve other findings and unfinished full-audit state.
Complexity high where parsers/lifecycles/concurrent observations interact; risk medium for data correctness/test false success, memory and transport inputs. Same host/provider/model inherited; exact model/effort unknown. No budget ceiling or extra charges configured.
