# LIB-013 attachment publication repair

Owner request: hold the recursive bus mutex through device->attach(), release it immediately afterwards before automatic CONFIG.LOAD, and verify in non-interactive simulator mode. Repair FND-000013 (legacy LIB-013). No driver behavior or public API changes beyond attachment synchronization.

Owner explicitly deferred the overdue repository audit for this repair in the current chat on 2026-10-26. Audit remains due; this task does not reset its checkpoint. No merge, push or hardware run is authorized.
