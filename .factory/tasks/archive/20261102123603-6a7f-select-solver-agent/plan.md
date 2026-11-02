# Plan (as executed)

1. Done: baseline Sequencer suite 29/29 at `a5c7b08dd` (`baseline-sequencer.log`).
2. Done: earlier scratch probe showed the flip reaches Platesolver Agent when loaded before `start()` and misses a solver appearing after it (result in verification.md).
3. Done: removed the `start()` argument, added the `select_solver_agent` constructor and sequencer step, updated Sequencer.md.
4. Done: tests: step routing, automatic selection after a selected run, two flip cases; public method inventory counts updated for the new method. First run failed only on the second inventory count; corrected, 32/32 (`test-agent-scripting-sequencer.log`).
5. Done: mutation checks (`mutation-checks.txt`).
6. Independent review, reports, archive; owner approval of the code commit and `factory:` commit.
