# Plan (as executed)

1. Inspect the flipper failure branches (all call `indigo_sequencer.abort()`), the sequencer failure policy (`failure()`, `skip_to_recovery_point`), the capture step waiting for the imager and the imager abort/pause handling.
2. Sequencer.js: `indigo_flipper.fail()` shared by the four failure branches (HA limit restore, `FLIPPER_STATE` Alert); `indigo_sequencer.flip_failed()` choosing abort, imager abort with failure on its Alert, or cancelled waits with an immediate step failure; reset `resume_ha_limit` at transit.
3. Sequencer.md: `enable_meridian_flip` row.
4. Tests: recover (wait step, HA limit, timer), capture with the real Imager Agent and CCD simulator, continue, abort; run against the previous Sequencer.js.
5. Independent read-only review, fixes, final suite and Factory checks, process evidence, archive, owner approval of the product and `factory: ` commits.
