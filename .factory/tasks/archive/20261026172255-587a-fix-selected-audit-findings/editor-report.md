# Editor mapping repair report

Scope: FND-20261023065605-60d4-repository-audit-006 / AC-008.
Developer: in-host `editor_mapping` subagent, inherited model/settings (not exposed). Review independence: implemented and ran developer checks, not independent sign-off.
Working directory for all commands: `/private/tmp/indigo-selected-audit-fixes`.
Base revision: `a35823d61`; candidate working tree on task branch, source changes uncommitted.

## Changes

- `indigo_tools/driver_editor/source_map.py`: resolve device `id` independently of its class; apply identity to existing generated markers, lifecycle functions, templates, handle tokens and property-handler defaults. Match property/item contexts against exact generated function names so an identity that prefixes another (`ccd` / `ccd_guide`) cannot claim the latter's property dispatch.
- `indigo_tools/driver_editor/test_editor.py`: two new regressions, one synthetic same-class pair with overlapping identity prefixes and the same inherited property, one covering every one of the five explicitly identified CCD simulator logical devices.
- No generator, DSL definition, generated output, frontend, README, dependency or runtime security changes. No new source/test file needs Xcode registration. Editor `REFACTOR.md` does not exist; results recorded here as assigned.

## Baseline and regression evidence

Existing suite before implementation: `python3 -B -m unittest discover -s indigo_tools/driver_editor -v` ran 35 tests; 3 failures, 5 errors. Three errors were sandbox denial of isolated loopback HTTP binds.

The original source and tests were then executed directly from immutable `git show a35823d61:...` snapshots, without changing candidate files, with loopback permission: 35 tests, 3 failures and 2 errors; `editor-baseline.log`. This proves the two parser failures and two polaralign generation errors predate this repair, as well as the target CCD mapping failure.

New regressions against the unmodified source: the same discover command ran 37 tests, 10 failing subtest reports and 5 errors; `editor-regression-baseline.log`. Each of the five CCD simulator devices and both synthetic logical devices had no correct generated function/property mapping. Later the overlapping-id test also demonstrated the prefix leak before exact function scoping.

## Candidate checks

- `python3 -B -m unittest discover -s indigo_tools/driver_editor -v`, with sandbox escalation only to permit the suite's isolated loopback HTTP server: 37 tests, 2 pre-existing failures and 2 pre-existing errors; `editor-final.log`. The target corpus mapping failure is fixed and all HTTP tests pass.
- `PYTHONPATH=indigo_tools/driver_editor python3 -B -m unittest test_editor.MappingTests.test_serial_patterns_parent_union_and_no_output test_editor.MappingTests.test_repeated_code_blocks_report_shared_mapping test_editor.MappingTests.test_preview_matches_normal_generator_working_directory test_editor.MappingTests.test_property_lifecycle_and_detach_marker test_editor.MappingTests.test_duplicate_property_names_are_scoped_and_references_excluded test_editor.MappingTests.test_explicit_ccd_identities_scope_lifecycle_properties_and_markers test_editor.MappingTests.test_all_ccd_simulator_logical_devices_map_exposure test_editor.EditorTests -v`: 20 tests, all pass; `editor-targeted.log`.
- `git diff --check -- indigo_tools/driver_editor/source_map.py indigo_tools/driver_editor/test_editor.py`: pass.

## Existing full-suite defects outside assigned scope

- `test_template_code_blocks_match_generator_contexts`: template lacks SDK `discover` code-block catalog present in generator.
- `test_template_matches_generator_attribute_contexts`: template device catalog lacks `id` identifier attribute present in generator.
- Corpus generator tests for `indigo_polaralign_mlastro.driver` and `indigo_polaralign_simulator.driver`: editor supported device set lacks `polaralign`, so output_names rejects these before generation.

These remain visible failures, not skips or passes. No test expectation was weakened or removed. Full editor suite is not clean; targeted requested mapping repair passes. Parent should reconcile these baseline observations with existing findings and avoid a full-suite-pass claim. No physical hardware, external service, installed dependency, ABI change or publication involved.

Next: independent final-diff review of these changes; coordinator owns overall acceptance and finding reconciliation. Writer released for assigned source/tests.
