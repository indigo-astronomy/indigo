# Test and regression plan

Status: final

The scenario-to-test mapping is in `indigo_drivers/aux_alto/REFACTOR.md` (Scenario mapping). Layers: `test_primalucelib` (unit),
`test_aux_alto_simulator` (28 cases against `alto_simulator`), `test_aux_alto_hw` (9 opt-in cases on the requester's ALTO1), the
GIOTTO and focuser suites as regression checks of the shared library. Ten driver mutations proved the failure cases.
