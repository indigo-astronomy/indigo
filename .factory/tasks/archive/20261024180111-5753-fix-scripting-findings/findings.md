# Finding remediation

FND-20261024121920-3bd2-repository-audit-035: all four define/redefine and update bindings copy properties before queueing. A private list tracks snapshots so dispatch can remove and release each copy after bus consumers return; after the queue joins, detach reclaims undispatched entries. The dedicated list mutex is never held across bus callbacks. Existing long-text deep-copy behavior in indigo_copy_property supplies independent item storage. The original pointer invalidation path was source-reviewed; tests verify immutable event values without a memory-corruption reproducer. Existing 257-item coverage checks property growth separately.

FND-20261023101753-bc8d-complete-repository-audit-046: save_batch allocates a new object and copies every own item value, including settings beyond COUNT/EXPOSURE. The regression executes public focus and focus_ignore_failure wrappers with deterministic process completion, feeds actual boot.js cache updates, checks original COUNT/EXPOSURE/DELAY and repeats with a different batch. Ignored failure uses the engine's failure routing.

The native test helper previously passed arbitrary script text as a printf format. New dynamic coverage exposed malformed script input when %g appeared. It now uses the raw text API, matching the existing Sequencer harness. This is a scoped test-harness correction, not a product API change.

See verification.md for baseline, full-suite and sanitizer evidence, quality-review.md for independent diff review and security.md for lifetime analysis. Registry closure follows the completed evidence.
