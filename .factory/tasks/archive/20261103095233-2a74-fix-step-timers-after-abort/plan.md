# Plan (as executed)

1. Reproduce: tests with a pending step timer fail on the unfixed code, including a stale timer advancing the next sequence.
2. Sequencer.js: route the next and next_ok timers through `indigo_sequencer_set_step_timer()`, which binds them to `timer_generation`; `abort()` and the abort branch of `failure()` increment it.
3. Independent read-only review; apply its findings: same cleanup in `failure()`, `cancel_stale_timer()`, sentinel instead of sleep, mount delay test.
4. Final suite, mutation runs, Factory checks, process evidence, archive, owner approval of the product and `factory: ` commits.
