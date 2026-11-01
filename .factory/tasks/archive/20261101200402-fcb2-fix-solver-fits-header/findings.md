# Separate pre-existing catalog follow-up

FND-20261101200402-fcb2-fix-solver-fits-header-001: catalog builder subtracts a foreign star pointer before membership validation.

Location indigo_drivers/agent_solver/indigo_solver.c:1052–1058 (current candidate), function indigo_solver_catalog_build. A quad star pointer outside the supplied stars array is subtracted from stars before index bounds are checked. C pointer subtraction is defined only within the same array; the subsequent range check cannot establish the precondition retroactively. Current test_agent_solver_core.c:index_file_roundtrip deliberately supplies an independent outside object and expects rejection. Under identical ASan/UBSan/float-cast flags, both original base127036f source and finding006-fixed source accept it and fail the same assertion at test188. Normal optimized suite passes, exposing stack-layout dependence rather than a demonstrated006 regression.

Evidence: verification.md full-sanitizer section, sanitize-original-command.txt, sanitize-original-index.log, sanitize-fixed.log (where retained) and final normal core output. No security exploitability or production arbitrary-pointer source is established. Consequence: catalog construction can accept invalid membership and write incorrect star indexes; no broad release certification.

Severity medium, category functional, nonblocking follow-up owned by coordinator. Next focused repair establishes membership/alignment safely before pointer arithmetic and verifies original rejection assertion across optimization/sanitizer layouts. Do not weaken the assertion. This is not part of owner-selected006 and is not accepted risk; preserve it as an active record.
