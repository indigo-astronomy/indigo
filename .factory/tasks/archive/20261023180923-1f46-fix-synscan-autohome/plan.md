# Plan

1. Baseline: build original driver/test, run existing suite with retained command traces, identify stage markers. Extend deterministic simulator controls and bus-level tests before production edits; preserve success behavior and record cancellation as expected baseline failure.
2. Implement private AUTOHOME state and monotonic per-stage deadlines. Split blocking motion preparation/stop/index/dwell into bounded callbacks; preserve semantic command order. Wire explicit abort/disconnect cancellation and failure stops. Increment version and regenerate unchanged generator outputs.
3. Run new stage cancellation/failure/timeout cases and full existing driver scope; compare original normalized command trace, documenting timing/poll-only differences. Run strict/sanitized checks where available.
4. Update driver REFACTOR, migration/test records, project registration and finding. Complete same-agent sequential verification, documentation, quality and security passes with explicit independence limits and Factory process evidence.
5. Wait for tested 039 master SHA, synchronize/reconcile shared metadata, rerun checks/reviews on final candidate. Archive/commit evidence, committed-range checks, report readiness; integrate only with coordinator authorization. No push.
