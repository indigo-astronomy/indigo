# Specification
Status: accepted for implementation from owner request, 2026-10-22.
AC-001: `%t` expands measured chip temperature with zero decimal places; `%nt` uses single-digit precision n capped at five, matching existing target formatting.
AC-002: `%T`/`%nT` retain target values, precision and NA behavior when temperature/cooler is hidden or cooling is off.
AC-003: `%t`/`%nt` use measured temperature whenever temperature is visible, including cooling off or unavailable. Hidden temperature produces NA. This is a low-risk interpretation of measured versus target temperature.
AC-004: Mixed/repeated macros retain surrounding filename text, suffix and sequence behavior. Document new macros in the existing reference.
Complexity low: one established parser branch, no ABI/lifecycle change. Risk low: reversible filename substitution. Existing filename buffer handling is outside scope; no new trust boundary, permissions or dependencies. No unresolved questions.
