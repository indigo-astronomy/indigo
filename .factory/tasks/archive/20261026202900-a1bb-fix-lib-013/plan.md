# Plan

1. Add synthetic public-bus lifecycle regressions for concurrent enumeration, recursive property publication and unlock-before-CONFIG.
2. Run the regression against the unchanged library, preserving the expected failure.
3. Move only the device_mutex unlock from before attach to immediately after attach; add a concise rationale comment.
4. Rebuild the shared library. Run lifecycle and native unit regressions, then complete rotator_lunatico and focuser_lunatico simulator suites through tools/run_driver_test.py. These exercise the historical dynamic port-device case and shared implementation. Record each driver's testing update separately per owner policy if separate commit units are required.
5. Run configured Factory check/test/security groups, independently review final diff, reconcile FND-000013, archive and commit tested scoped changes. Leave integration/publication pending.

One source writer (coordinator/developer), one test writer (in-host subagent, lifecycle file only), separate read-only quality and security reviewers. No source generator or new dependencies. No property/version changes in drivers. Existing test source is already in Xcode; Factory metadata must not enter the project.
