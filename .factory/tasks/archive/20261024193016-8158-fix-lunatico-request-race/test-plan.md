# Test plan

Run the original focuser (57 cases) and rotator (29 serial +29 UDP cases) simulator suites. Add bounded held-poll request tests checking exact requested GOTO/SYNC/relative commands, final target/value, duplicate rejection and cancellation/reconnect. Retain existing lifecycle, external movement, error and compensation tests. Capture original failure and fixed outcomes. Run available ASan regressions and Factory checks. No physical hardware, Linux or Windows runtime certification.
