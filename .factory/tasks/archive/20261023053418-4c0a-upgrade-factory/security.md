# Security review

Verdict: pass

Reviewed revision/state: final working upgrade diff from b0c5cf377f0c68de9e0abd850b0b15e925cb8091; source identities recorded in process.json. Same-agent separate security pass, not independent certification. Scope: process-policy/data-model migration, helpers/instructions, local state/evidence, template provenance and navigation integration. Existing application security behavior and findings are outside repair scope.

Trust boundaries: credential-free pinned upstream Git objects; local reviewed map and upgrade inputs; project-owned findings/completion/configuration; retained commit proof and local lifecycle locks. Reviewed code validates bounded JSON/schema/paths, rejects symlink escape and malformed state, fingerprints migration inputs/HEAD and writes atomically with ordinary-error rollback. Proof commits remain ancestors; records cannot silently reset missing history. Explicit audit coverage prevents concurrently unseen work disappearing from cadence; exact report verdict preserves the existing accepted audit without treating this upgrade as an audit.

Protected driver/vendor/legacy paths are filtered before task inventory reads, existing archive-time exclusions remain, and no protected path changes. All new helpers match pinned source; existing local archive safeguards survive. INDIGO instructions, disabled external review, private ignore boundaries and configured commands are retained. Xcode changes affect navigation only, with existing build/settings object equality verified. No new dependencies, external provider invocation, credentials, deployment or data upload.

`./.factory/scripts/security` passes the configured local boundary validator; `check`, record-preservation and synthetic negative tests are detailed in verification.md. This is not a network/dependency scanner or native application audit. docs/security.md is absent in this adopter; no security document or scan result was fabricated. No newly confirmed security finding or accepted-risk waiver; existing nonblocking findings remain open.

Limitations: local locks coordinate cooperating writers, not independent clones; 16-bit task suffixes provide probabilistic uniqueness only. Same-finding/shared-configuration edits still need deliberate Git integration. Reports/completion declarations are trusted-writer assertions, not authenticated human approvals. Multi-file rollback handles ordinary exceptions, not a guaranteed crash/power-loss transaction. Recovery retains source/manifest/proof history. No Linux/Windows/hardware or independent review performed.
