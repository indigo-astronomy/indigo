# Plan and ownership

1. Coordinator prepares task, specification and regression contracts from retained audit evidence. Base 989ec8db122f65ea3813af9db13d510d18bee90e; origin/master equals master after successful fetch.
2. Transport developer owns indigo_server_tcp.c, indigo_bus.c and indigo/indigo_bus.h plus uniquely named transport tests in isolated /private/tmp/indigo-security-transport. Address 001/002/004/005. Propose/cache snapshot API to parser developer; never edit XML/JSON concurrently.
3. Parser developer owns indigo_xml.c and indigo_json.c plus uniquely named parser tests in /private/tmp/indigo-security-parsers. Address 011–017; coordinate XML cache consumer with transport API.
4. Management developer owns indigo_server/indigo_server.c plus uniquely named management tests in /private/tmp/indigo-security-management. Address 008/009. No actual helper effects.
5. Each developer reads applicable source/test instructions, designs/reproduces bounded regressions before repair where practical, runs relevant checks and returns exact results in a separate temporary report. Coordinator alone owns main task reports, registry and shared test Makefile. Isolated worktrees are explicitly assigned to this one task, not separate outcomes; no duplicate task records. No subagent commits or integration.
6. Coordinator integrates patches, builds source-current library/targeted tests, independently verifies combined behavior, documents any new practical limits, then commissions independent final-diff/security review. Synchronize configured target before final candidate checks. Follow normal process/archival/commit gates; merge/publication remain unauthorized.

No dependencies planned. Follow existing portable I/O and locking conventions; inspect the public API compatibility impact before changing cache contracts. Rollback is the unmerged task diff. Review ownership and parser/transport boundary before integration.


Coupled scope refinement: coordinator owns the additional retained-cache API consumer in `indigo_drivers/agent_alpaca/indigo_alpaca_ccd.c`, its version in `indigo_agent_alpaca.c`, REFACTOR.md and `indigo_test/integration/test_agent_alpaca_cache.c`. Baseline bridge build and original synthetic image-array characterization passed before production edits. This is necessary to cover the maintained reader discovered by transport inspection; no broad driver migration or hardware work is assigned.

Coupled review repair: FND-20261023121227-a609-fix-server-security-high-001 requires XML upstream message text to publish literally, preserving NULL for absent messages; percent-containing define/update/delete/message regressions verify the same boundary class as audit 009. See preliminary-review.md.
