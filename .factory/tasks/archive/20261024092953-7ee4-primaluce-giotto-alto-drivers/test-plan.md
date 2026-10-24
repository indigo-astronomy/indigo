# Test and regression plan

Status: final

The scenario-to-test mapping is in `indigo_drivers/aux_giotto/REFACTOR.md` (Scenario mapping). Layers: `test_primalucelib`
(unit), `test_aux_giotto_simulator` (25 cases against `giotto_simulator`), `test_aux_giotto_hw` (8 opt-in cases on the
requester's GIOTTO), and the focuser suite as a regression check of the shared library. Each new failure case was proven by a
driver mutation (eight, listed in REFACTOR.md).
