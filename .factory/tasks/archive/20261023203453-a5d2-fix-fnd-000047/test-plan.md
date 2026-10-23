# Test plan

| Criterion | Scenario | Evidence |
| --- | --- | --- |
| AC-001 | Existing test reproduces /dev/null mismatch; repaired test still checks NULL handle, non-NULL error and exact ENOTTY message through the public API | test-unit baseline and final logs |
| AC-002 | Inspect unique fixture creation, descriptor/handle close and unlink before assertions; check no fixture remains | final diff and filesystem inspection |
| AC-003 | Run make -C indigo_test test-unit without a filter | complete suite output including RAW suites |

Keep the existing assertion strict; do not accept multiple messages or skip the case. Run Factory checks for evidence/registry changes. No hardware, sockets or external data are needed for the repair itself. Linux runtime is unavailable.
