# Adoption security review

Verdict: pass

Reviewer: same Codex agent in an explicitly loaded security-reviewer pass. Reviewed revision/state: final workflow-only adoption working tree against `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`; exact source identity in process.json. This is local workflow boundary/security triage, not a substantive application security audit or independent sign-off.

## Trust and data boundaries
Template files came solely from the separately fetched clean detached revision `11392a11253bae4d1c5d5db97e8a532917728c09` of the approved repository, never from the persistent registry checkout or a moving branch. The reviewed updater reads pinned Git blobs with replacements/lazy fetch disabled; init writes provenance, fleet/plan write no adopters. Shared instructions preserve higher-priority platform/owner constraints; repository content does not grant external tool authority.

The persistent registry's origin and clean main checkout were verified. The inventory was already ignored and untracked, validated for duplicate keys/names/canonical paths and version-1 schema, and changed with an advisory exclusive file lock, immediate pre-replacement identity/content check, fsync, mode preservation and atomic replace. This detects observed concurrent edits; it is not a general filesystem transaction against uncooperative writers. No other writer was observed. Existing entry/configuration/tracked files are preserved. Private inventory content and other projects' paths are not copied into the repository; the task retains only aliases/statuses. No credentials were read or introduced.

Cross-provider review remains disabled with empty allowed_reviewers/share_paths/required_roles. Model policy files do not switch the current model, authorize spending or invoke a provider. Helper regression review tests invoke local synthetic CLI files, never a real provider. No subagents, plugin installation, dependency download, application server, hardware session, publish/deploy/merge/push or future upgrade occurs.

## Evidence and limits
Configured local security command and helper negative tests pass; see [verification.md](verification.md) for exact commands. Tests exercise traversal/symlink guards, invalid JSON, stale provenance, protected instance data, concurrent-change detection, forbidden provider boundaries and evidence binding using disposable synthetic repositories. Every new private metadata destination is ignored; mapped support paths are regular local files. Existing application README/configuration/SDK and off-limits trees are unchanged.

Five imported historical findings are conservatively labelled security (FND-000023, FND-000036, FND-000037, FND-000040, FND-000041; inspect the canonical register for current exact IDs). They remain open with original evidence and coordinator ownership; import does not verify exploitability or accept risk. No product repair/retest or product-security assurance is claimed. Revalidate and prioritize these in the next full audit; the owner-approved audit deferral is a scheduling exception, not risk acceptance for those defects.

No new adoption-specific security blocker was found in this scoped review. Other client native instruction loading and the application test suite remain unverified/unavailable as documented. Broader product-security review requires its own authorized scope and qualified evidence.
