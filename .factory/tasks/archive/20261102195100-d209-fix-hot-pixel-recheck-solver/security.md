# Security review

Verdict: pass

Root read security-reviewer role; independent reviewer inspected actual simulator/test boundary and solver evidence, root synthesized this scoped report. Baseline067293e2f, final source identities in process.json. Technical AI source/artifact review, not human approval or risk acceptance; runtime ownership in verification.md.

Simulator defect is an off-by-one write boundary: >= rejects bottom-edge row before any hot-pixel/row write; adversarial synthetic coordinates exercise negative, equal/beyond dimensions with preserved valid border points and guard memory. ASan/UBSan focused test clean; original guard corruption reproduced before fix. Common libraries remain uninstrumented and physical/platform validation is not claimed. Production allocation/lifecycle/property surface unchanged. Test wrapper is linked only into simulator tests/benchmark, forwards real attachment, accesses actual private state without copying layout and adds no production hook. Fixtures synthetic; no credentials or external calls.

Solver evidence uses maintained static image and read-only existing local indexes, freshly compiled unchanged core, no downloads or threshold weakening. Historical byte replay limitation retained. Gzip preserves verbose diagnostic output; input/source hashes and construction retained for reproducibility. No new dependency/authentication/path/network boundary. Configured local security check passes; not a repository-wide security audit.

Conservative data-model risk signal from MIGRATION_STATUS.md is reviewed: only ccd_simulator automated availability28/0→29/0; manual Comment and hardware0 unchanged. README/TEST_SUMMARY changes record actual software run, no schema/data migration. No risk accepted or unresolved blocking security finding introduced.
