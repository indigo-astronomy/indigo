# Plan

1. Capture the current test-unit failure (AC-001/003).
2. Replace /dev/null with mkstemp's isolated regular file. Close the creation descriptor before serial open, then close any returned handle and unlink before checking the exact existing contract (AC-001/002).
3. Run test-unit and configured Factory check/test/security groups. Review the final diff and temporary resource lifecycle in separate sequential role passes. No dependencies or ADRs are needed.
4. Reconcile FND-000047, retain evidence, archive the completed task, commit once and run the committed-range process gate. Integration/publication require separate authorization.
