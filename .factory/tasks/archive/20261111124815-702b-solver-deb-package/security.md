# Security review

Verdict: pass
The change only adds packaging metadata and copies a built binary. The solver package has only a read-only `preinst` status check, no
services, users or setuid files. No secrets or credentials involved. Reviewer: implementer, not independent.
