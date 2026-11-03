# Plan (as executed)

1. Reproduce: tests holding the agent mutex until the timer is firing fail on the unfixed code.
2. Sequencer.js: `cancel_wait_timer()` returns whether the timer was cancelled; `flip_failed()` otherwise stores `flip_failure`; the altitude poll handler applies it.
3. Correct the test step index and add a second advance check; independent read-only review; apply its findings: check that the race was reached, the same check in the poll test, polls through the generation bound timer, abort and pause tests.
4. Final suite twice, mutation runs, Factory checks, process evidence, archive, owner approval of the product and `factory: ` commits.
