# Repair RAW selection safety

Fix FND-20261023101753-bc8d-complete-repository-audit-006: reject incomplete/nonpositive selection ROIs before reads and eliminate PSF border variance overflow. Preserve public ABI and valid selection behavior.

Owner explicitly authorized this separate session and audit deferral extension on 2026-10-23. Local integration belongs to the coordinator after 039 then 026; no push.

Allowed paths: indigo_libs/indigo_raw_utils.c and its public header, indigo_test/unit/test_raw_image.c and indigo_test/Makefile, task evidence, finding 006, maintenance.json, and essential Xcode evidence references. No drivers, generator, hardware or README changes.
