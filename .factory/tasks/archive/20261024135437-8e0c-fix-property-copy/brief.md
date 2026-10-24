# Repair high-priority repository audit findings

Owner request: repair FND-20261024121920-3bd2-repository-audit-001 after explaining the defect.
Final scope: audit high findings 001/002/003/010/011/042, related core/server/UI regressions, Alpaca shutdown ordering/version and task/finding evidence. No generator, public property or ABI changes. Local task commit is authorized by repository workflow; main integration/publication are not authorized.

## Owner scope extension

Owner subsequently requested all remaining high findings from audit 20261024121920-3bd2-repository-audit: 002, 003, 010, 011, 042, alongside 001. Keep the existing task identity and branch for this explicit scope extension. Related core changes share files; one combined final commit/review avoids overlapping task branches. Three isolated detached worker worktrees are explicitly assigned to this same parent task; root owns task evidence, integration and canonical registries. No main merge/push authorization is inferred.
