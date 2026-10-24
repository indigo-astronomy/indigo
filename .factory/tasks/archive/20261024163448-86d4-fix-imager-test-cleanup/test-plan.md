# Test plan

AC-001/004: existing instance and full imager integrations exercise connected cameras and active guider work; require successful shutdown/stop with no attached-device warning. AC-002/003: inject observed failure after real CCD driver shutdown, observer detach and bus stop; require nonzero child exit, exactly one fail result and no pass. Verify subsequent stages still execute. AC-005: existing six instance scenarios and all other imager cases remain unchanged, and machine plan agrees with selected cases.

Baseline: source ignores all five teardown return values and records pass before cleanup. Prior task retained a real attached-device warning despite successful result. A focused negative test can also run against the existing binary before applying the harness patch: ignored fault injection means unexpected success and a failed regression check. Tests use private HOME and per-process synthetic resources only.
