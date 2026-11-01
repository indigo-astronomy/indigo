# Unchecked areas retained after partial closure

Task20261101193246-1248-periodic-repository-audit; source inspection at42011325b8d3f9dcca2cc768594d42b385d4d481. The owner accepts partial closure, not full coverage. Reviewed portions and inventories remain in repository-review.md, maintained-inventory.txt and changed-maintained-inventory.txt. Coordinator owns follow-up below; these are inspection gaps, not newly confirmed defects. Do not repeat already-reviewed portions without a changed contract or concrete risk.

| Area | Remaining inspection | Follow-up trigger / owner |
| --- | --- | --- |
| Framework/protocol/math | Remaining parser/adapter/base-driver reads, timer ownership beyond inspected portions, and math/astronomy/polynomial entrypoints beyond selected paths. Completed JSON/XML portions remain reviewed as described in the report. | Next incremental audit when affected, otherwise planned full review; coordinator assigns component reviewer. |
| Server and browser | Remaining UI templates and behavior beyond selected UI changes and editor app.js; no whole-browser/runtime certification. | Changed UI/server contracts or planned full review; coordinator. |
| Portable drivers | Most driver families and lifecycle/protocol/ownership paths outside the explicit bounded pass. The453-file inventory is not semantic coverage. | Changed driver/dependency paths, risk-specific review or planned full review; coordinator/driver reviewers. |
| Platform/optional drivers | Remaining Linux/macOS/optional paths; GPIO/ASIAIR/libcamera checks and BT/Atik2/Andor/RPi excerpts do not establish full-family coverage. Protected drivers remain read-only exclusions. | Changed platform contracts or planned full review; coordinator. |
| Tests and harnesses | Individual scenarios and hardware-harness contracts beyond shared cache/recording/wait/hotplug architecture. | Changed test/harness contracts or planned full review; coordinator/test reviewer. |
| Tools/scripts | Generator/property-tool internals, large RPi scripts, fxload, minifier and remaining tool/script paths outside enumerated inspected utilities. | Changed tool/dependency paths or planned full review; coordinator. |
| Documentation/build/config | Remaining documentation-to-source, platform build and configuration contracts beyond explicit checks. | Changed contracts or planned full review; coordinator. |
| Execution limits | No Linux/Windows runtime, physical hardware or Valgrind certification; current-source versus existing-library provenance limits remain in verification.md. | Relevant platform/hardware task with explicit resources/authorization; no speculative broad reruns. |

## Investigation questions, not confirmed findings
Retain separately: simulator-cache pointer lifetime, server driver-catalog capacity, current consumers of the standalone Windows SDK project, HOME.md consumers and a supported non-NULL format-forwarding caller. Establish current consumers/reproduction before allocating any finding. No defect or successful verification is inferred from these questions.

## Existing findings
Canonical finding records remain authoritative. Origin findings004/005/011 and reopened server-security-audit-007 stay open with coordinator ownership; selected11repairs retain their independent closure evidence. Other existing findings remain unchanged. Partial closure does not accept security risk, close findings, remove evidence or certify a release. Future audit tasks should reference this retained list and add only the gaps they actually inspect.
