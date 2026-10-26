# Specification

Owner requests repair of these ten active findings. Shell compliance, physical hardware and unrelated functional findings are excluded. No merge or push is authorized.

- AC-001: `FND-000028` — Default integration registration has current passing Lunatico serial and UDP evidence.
- AC-002: `FND-000029` — Cooler injection and guide/abort/exposure scenarios use causal synchronization and retain completion/image checks.
- AC-003: `FND-20261023065605-60d4-repository-audit-003` — Retired hosted CI configuration, badge, Xcode reference and maintained documentation references are removed. Generic manual build helpers and historical evidence are preserved.
- AC-004: `FND-20261023101753-bc8d-complete-repository-audit-030` — A failed test process prevents per-device OK even if all tagged cases passed; factual case counts remain unchanged.
- AC-005: `FND-20261024121920-3bd2-repository-audit-040` — Dry-run help describes execution/build/temporary-file/hardware side effects and previews final record writes only.
- AC-006: `FND-20261024121920-3bd2-repository-audit-043` — Factory roles point to adopted test/docs instructions and archive guidance distinguishes v2 retained completion state from legacy reset.
- AC-007: `FND-20261025211159-e308-fix-audit-findings-batch-001` — Optec descriptor accounting covers repeated driver lifecycles after process-global USB setup.
- AC-008: `FND-20261025211159-e308-fix-audit-findings-batch-002` — Mount simulator abort test observes fresh completed publications and real movement before abort.
- AC-009: `FND-20261025211159-e308-fix-audit-findings-batch-003` — iOptron validates signed angular RA motion across midnight in every protocol dialect.
- AC-010: `FND-20261025211159-e308-fix-audit-findings-batch-004` — Temma GOTO/park and NexStar park have deterministic real deadline expiry, stop, ALERT and recovery regressions, with unchanged production defaults.

Complexity high: timing-sensitive queue/timer/SDK interactions and state publication require causal synchronization and controlled fault scenarios. Risk medium: test false passes could misstate driver readiness; production defaults and public properties must remain unchanged. No new dependency, provider or spending. Material requirements are resolved; no hardware availability is assumed.
