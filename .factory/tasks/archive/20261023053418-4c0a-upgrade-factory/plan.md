# Plan

1. Synchronize clean master and create dedicated legacy upgrade task using installed helper. Capture baseline Factory commands, lock and registry/config hashes.
2. Review additions/mappings and unmapped upstream changes. Commit only the recoverable map baseline required by updater; meaningful migration checkpoint exception, no history rewriting.
3. Apply pinned three-way update, preserving INDIGO adaptations; add new helper/tests mappings, adapt synthetic tests to real repository root and required headers. Exclude template-only layout fixture/history/ADRs/export rules; write an INDIGO upgrade decision instead.
4. Preview/review/apply task renaming with excluded driver and legacy-test paths filtered before any inventory reads. Preserve pinned references. Commit the renamed completed states as the explicit migration proof checkpoint required before metadata import.
5. Build actual completion manifest from retained Git completion evidence, including qualified initial audit and exact covered tasks; preview/review/apply metadata migration. Validate derived nine-task cadence and active findings equality.
6. Run configured checks/regressions, inspect final diff and separate quality/security passes. Archive this task, capture reviewed source identities, commit final outcome and run immutable process gate. Leave integration/publication pending; full audit remains due.
