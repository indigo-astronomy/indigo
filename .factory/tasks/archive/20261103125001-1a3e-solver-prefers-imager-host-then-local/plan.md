# Plan (as executed)

1. Read `sequencer_solver_agent()` and its callers; tell the owner the imager host was already preferred.
2. Tests first: fail on the old code. Strengthen them after mutation runs showed two checks that could not fail (the local solver listed first, no solver on the stale imager's host).
3. Sequencer.js and Sequencer.md: host only from a loaded imager agent, local preferred over other hosts.
4. Independent read-only review; add the correction at use test and the documentation sentence.
5. Final suite, Factory checks, process evidence, archive, owner approval of the product and `factory: ` commits.
