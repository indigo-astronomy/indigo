# Repair imager instance-test teardown

Task: 20261024163448-86d4-fix-imager-test-cleanup. Owner: Peter Polakovic. Date: 2026-10-24.

Repair FND-20261024155149-8381-fix-core-audit-findings-001: teardown errors must fail the case and machine-readable result; normal teardown must stop active work and disconnect simulator resources before shutdown. Preserve all existing assertions and repeat the full hardware-free imager scope. No production driver changes, hardware, publication or master integration are assigned.

This task depends on the preceding unintegrated repair commit 79dcf6d81e633460acfbefcf1701ca214d5089bc. A new dependent task branch starts there so the requested finding and prior fixes remain present; master remains unchanged. This is a new finding repair, not reopening archived task evidence.
