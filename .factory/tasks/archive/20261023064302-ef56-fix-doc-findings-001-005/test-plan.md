# Verification strategy

| Acceptance | Evidence |
| --- | --- |
| AC-001 | Compare help option sequence with main() help strings and check guards/option handlers. |
| AC-002 | Inspect introduction and retain one occurrence. |
| AC-003 | Resolve changed relative links on disk and inspect visible labels. |
| AC-004 | Compare Makefile.inc target and Makefile.drv prerequisite/recipe rules; inspect manual generator setup. |
| AC-005 | Compare SERVICE/device definitions, connection calls and wait loops; document failure limits without running network examples. |
| AC-006 | Findings check validates closure and retained task references. |

Run configured .factory/scripts/check, test and security. No new executable tests for reversible prose edits; existing Factory tests exercise metadata lifecycle. No native, driver, network or hardware tests required.
