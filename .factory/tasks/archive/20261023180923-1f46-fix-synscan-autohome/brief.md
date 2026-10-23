# Fix interruptible SynScan AUTOHOME

Repair FND-20261023101753-bc8d-complete-repository-audit-026 with a staged queued handler/finalizer operation. Preserve the original protocol sequence and public property contract. Cover success, stage-specific abort/disconnect, actual stop commands, malformed/failed replies and timeouts with simulator tests and retained characterization traces.

Scope: mount_synscan driver source and regenerated outputs, its simulator and driver-specific tests, REFACTOR.md, test/build/Xcode registration and migration/test summary metadata, this task evidence and finding resolution. No generator changes, subagents, physical hardware tests or push are authorized.

Isolation: task/20261023180923-1f46-fix-synscan-autohome in /private/tmp/indigo-20261023180923-1f46-fix-synscan-autohome. Base master: 3b48146ca79b4eb314460d4f9c3728b11df9990c. Primary checkout belongs to fix 039 and must remain untouched.

Integration order: 039 first, 026 second. Wait for coordinating chat 01a169ed-63c1-7443-a96e-007a7872884e to supply tested 039 SHA and integration authorization. Incorporate that master target and reverify before readiness.

Implementation entry is blocked pending owner audit deferral for 026 or completion of the due audit. Existing deferral is explicitly for 039 only.
