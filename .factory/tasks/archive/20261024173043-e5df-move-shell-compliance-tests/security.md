# Security and process policy review

Verdict: pass

Root performed a separate security pass on the dirty candidate based on afdf183985d8337d51b7f1d0d57ffd58859e9452; same-writer limitation applies. Only file placement and direct references change. No new executable content, dependencies, network calls, credentials or device actions are introduced. Original shell content and permissions are preserved.

Process-policy signal: AGENTS.md and indigo_test/AGENTS.md update the existing opt-in exclusion and describe the shell directory; quality, security, owner authorization and review gates remain. The archiver now excludes the relocated shell folder and still excludes historical indigo_tests paths, with the existing fixture extended to cover both. No path-based gate is weakened. Historical pinned findings remain open and unchanged; this is not remediation or risk acceptance of their shell behavior. `./.factory/scripts/security` passed its local boundary checks; this limited check is not a full shell audit. No hardware/server actions were run.
