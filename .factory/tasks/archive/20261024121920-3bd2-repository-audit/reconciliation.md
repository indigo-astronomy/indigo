# Existing finding reconciliation

Reviewed source: 9d45bc8ccadaf0c3aab6c454c38429fec21d1699. Canonical open records retain ownership and unresolved status. No source repair or risk acceptance occurs here. Component reports identify further revalidated families without duplicating them.

- FND-000040: current indigo_driver_metadata.c uses PATH_MAX and checked snprintf; historical fixed128/strcpy subclaim is no longer current. indigo_metadata_extractor shell invocation and unchecked pipe remain. See root-closure-report.md. Narrow record, do not close it.
- FND-20261023101753-bc8d-complete-repository-audit-022: retain imager and Alpaca consumer length findings; add platesolver preview indigo_platesolver.c:63-100, where valid header/type alone does not prove pixel payload extent. Guider capture now checks header/shape/payload and is not asserted vulnerable by this audit.
- FND-20261023101753-bc8d-complete-repository-audit-024: add nfocus138-152, optec208-211 and nexstar1030-1033 finalizers lacking overall operation deadlines despite bounded polls. Preserve abort/disconnect serviceability distinction. See driver-fm-report.md.
- FND-20261023101753-bc8d-complete-repository-audit-035: add indigo_bus.c:2470-2483 slave query: writes slaves[count] before count++ >= max; full/zero capacity therefore is not guarded before indexing. Keep one capacity-boundary follow-up with native registries; include zero/full/max-1/removal tests. No capacity stress executed.

Historical Linux findings whose original patterns are absent remain open pending complete repair/retest reconciliation; this audit does not infer closure. Existing editor generation failures, example compile failure and approximate assertion NaN weakness remain known. No archived findings were mined routinely; targeted FND-000047 was read to resolve an inaccurate recollection and is unrelated to text formatting.
