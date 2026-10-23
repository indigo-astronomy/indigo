# RAW-to-FITS boundary repair

Fix FND-20261023101753-bc8d-complete-repository-audit-005. Owner explicitly requested isolated repair; primary checkout belongs to coordinator. Base: local master 3b48146ca. Only this worktree is writable by this task.

Scope: indigo_libs/indigo_fits.c, its public API documentation, indigo_test/unit/test_raw_to_fits.c and Makefile registration, finding record and task/maintenance evidence. No driver, generator, hardware, publication or push changes. Integration belongs to coordinating session. Original order 039 → 026 → 006 was superseded by the owner’s readiness-based instruction; verified post-006 master a1beecfdb8ad4e71af9df6670ef7b047e72ab107 is the assigned target.

Owner approved audit deferral extension to 005 on 2026-10-23: coordinator asked whether to expand deferral with audit mandatory after 039/026/006/005 before unrelated work/release; owner answered “Áno, odložiť aj pre 005”. This task does not reset audit cadence.
