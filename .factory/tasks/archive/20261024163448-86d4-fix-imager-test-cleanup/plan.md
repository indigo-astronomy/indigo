# Plan

1. Inspect the finding and actual shutdown contracts; use the prior recorded warning and current source as baseline.
2. Stop running agents, wait bounded completion, disconnect all simulator logical devices and check teardown results while retaining cleanup progress.
3. Wrap setup/body/cleanup in the recorded test callback; add deterministic negative regression checks using completed real teardown.
4. Run focused normal/negative cases and the final complete run through tools/run_driver_test.py agent_imager, with recording enabled from the start.
5. Update REFACTOR/test records, independently review final changes, close the finding, archive task evidence and commit one candidate. Preserve dependency branch and leave integration/publication pending.
