# Plan (as executed)

1. Inspect the owner's diff and every caller of `abort()`: the client abort handler (only with a running sequence) and `flip_failed()`.
2. Run the existing suite with the change: 43/44, "meridian flip without solver fails" still expected Alert.
3. Correct that expectation, add a helper that checks the state, revisions and messages across a failed flip, add four scenarios, strengthen the running and idle abort assertions.
4. Sequencer.md: `enable_meridian_flip` row.
5. Mutation runs, independent read-only review, final suite and Factory checks, process evidence, archive, owner approval of the product and `factory: ` commits.
