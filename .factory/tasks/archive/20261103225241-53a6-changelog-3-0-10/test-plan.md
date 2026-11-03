# Test plan

| Criterion | Check |
| --- | --- |
| AC-001 | header against `Makefile.inc` and the weekday of the date; block position, section order, tabs, blank lines (`grep`, `cat -A`); `git diff` adds lines only |
| AC-002 | inventory scan; independent review sampled about 40 lines against commits and source and looked for omissions and in-range fixes |
| AC-003 | `git ls-tree` driver directory comparison of `e5787a7e2` and HEAD |
| AC-004 | review of line style; traceability through `commit-inventory.txt` and the subagent research summarized in verification.md |

No code changes; no build or test suite applies.
