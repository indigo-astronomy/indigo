# Verification

Verdict: pass

Reviewed candidate: working-tree text changes on base 023f51b0758840c7c8138c0b8d2c1153c68ecc82, matching master/origin/master after authorized fetch and fast-forward synchronization. Working directory: /Users/polakovic/Development/indigo. Verifier is the implementing primary agent in a separate inspection pass; no independent verification is claimed.

AC1–AC2 pass by inspecting indigo_xml.c parse_state (105–114), set_one_light_vector_handler (728), and def_one_light_vector_handler (1102). Only received legacy property states map Idle to Ok; the light handlers use the current version. CCD_LENS (indigo_ccd_driver.c:662–668) and CCD_LENS_FOV (indigo_filter.c:747) demonstrate actual property-state use.

AC3 pass: the header now describes the property as passive without the false unused-by-INDIGO qualification. AC4 pass: exact two-file diff contains prose and one Doxygen comment only. No runtime expressions, enum values or ABI changed. Owner Xcode edits remain excluded.

Commands executed from the repository root on the scoped working tree:
- git diff --check -- indigo_docs/PROTOCOLS.md indigo_libs/indigo/indigo_bus.h: exit 0.
- git diff -- indigo_docs/PROTOCOLS.md indigo_libs/indigo/indigo_bus.h: inspected, only the specified changes.
- ./.factory/scripts/check: exit 0; whitespace, findings integrity (89 active, 109 archived), decision index and project boundaries passed.
- ./.factory/scripts/security: exit 0; local Factory security boundaries passed.
- Source inspection via sed: behavior matches the revised wording.

No tests, test suites, builds or hardware checks were run. Peter Polakovic explicitly instructed no tests for this documentation-only change; this is an owner-authorized exception to the configured test group, not a passing test result. Committed-range structural validation will run after the task commit and be reported externally.
