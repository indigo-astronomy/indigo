# Plan

1. Capture baseline library/driver builds and simulator results, then add credential and firmware regressions.
2. Bound firmware append and encode WiFi JSON string values before request formatting; increase focuser version and regenerate using the unchanged generator.
3. Run library, simulator, sanitizer and configured Factory checks; update protocol documentation and REFACTOR.
4. Independently inspect final diff, reconcile both findings, archive evidence and commit the verified candidate. Leave integration/publication pending.
