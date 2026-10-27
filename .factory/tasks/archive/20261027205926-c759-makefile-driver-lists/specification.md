# Specification

- AC-001: Makefile lists equal the brief's requested membership; no other list entry changes.
- AC-002: the regenerated `Makefile.inc` reflects the lists, `ccd_pentax` is no longer excluded from the driver build, and no driver is excluded from install.
- AC-004: no driver remains in `UNTESTED_DRIVERS` with a passing real-device record in its README `## Testing` section, and every moved driver has one.
- AC-003: `ccd_pentax`, `system_alpaca` and `agent_solver` build on macOS through `Makefile.drvs`.
