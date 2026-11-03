# Plan (as executed)

1. Range `e5787a7e2..HEAD` (3.0-9 release commit to the current master), non-merge commits of all authors; inventory without Factory, test, README, agent-instruction and migration-status-only commits (`commit-inventory.txt`).
2. New drivers from the driver directories at `e5787a7e2` and HEAD.
3. Read subjects; read the diffs of commits without a descriptive subject through two read-only research subagents (core/server and drivers/tools) and check their claims that conflict with the repository (ccd_libcamera is new, build list installation rule).
4. Draft the block following root AGENTS.md "Change Log".
5. Independent read-only review, corrections, Factory records, owner approval of the product and `factory: ` commits.
