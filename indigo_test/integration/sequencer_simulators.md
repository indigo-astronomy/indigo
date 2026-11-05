# Sequencer simulator script acceptance

Run from the repository root after building the native server, agents and simulator modules:

```sh
python3 indigo_test/integration/run_sequencer_simulators.py --output /tmp/sequencer-results
```

To include successful native solving, copy already installed indexes into the private test home:

```sh
python3 indigo_test/integration/run_sequencer_simulators.py --output /tmp/sequencer-results --index-dir "$HOME/.indigo/solver"
```

The output directory must not exist. Indexes are never downloaded. Without `--index-dir`, solver failure/recovery and filter selection still execute; successful center/GOTO tests are not scheduled. The equivalent opt-in Makefile target is `make -C indigo_test test-sequencer-simulators SEQUENCER_ARGS='--output /tmp/sequencer-results'`. This target is separate from the in-process `test-agent-scripting-sequencer` regression suite and is not part of the default no-socket integration target.

The required recorded driver run includes both native suites and this opt-in script suite:

```sh
INDIGO_SEQUENCER_OUTPUT=/tmp/sequencer-recorded INDIGO_SEQUENCER_INDEX_DIR="$HOME/.indigo/solver" python3 tools/run_driver_test.py agent_scripting
```

The Python runner appends plan/pass/fail records to `INDIGO_TEST_RESULTS` when supplied by that tool. Captured-image checks have their own plan. Known unfixed failures remain failures in the driver's Testing record and generated summary; do not convert them into expected successes.

The launcher starts its own server on an automatically allocated port with a new HOME, cache, image directory and temporary device and master ACLs. It loads real agents and simulator modules through `Server.DRIVERS`, then submits the complete unchanged `Sequencer.js` and prepared `sequencer_simulators.js` through `AGENT_SCRIPTING_RUN_SCRIPT`. Sequence execution and observations happen inside the actual Scripting Agent. No existing server, stored scripts, physical devices or saved user configuration is used. The owned server is stopped on success or failure. The temporary ACL is removed after shutdown.

The current native scripting change bindings omit access tokens. To test complete sequences on an authenticated server without changing production code, this suite replaces only those three primitive change bindings with a test transport relay. Each requested text/number/switch change is published by the script and forwarded unchanged by Python with the test token. All devices, processes, property responses, images and the `Sequence` implementation remain real; no successful response is fabricated. Reports explicitly record `authenticated_relay` and the request count. This mode tests Sequencer behavior and agent cooperation, not native scripting token propagation. Use `--native-bindings` to reproduce that separate defect; it is expected to fail on the current production code. The native `test_agent_scripting` token regression and `test_protocol_security` unset-master regression also expose the defects directly. The latter must not be interpreted as fixed by setting a test master token.

Near-transit fixtures derive sidereal time from the coherent Mount Agent display snapshot, then ask the launcher to convert the chosen JNow target to J2000 using the existing native `indigo_jnow_to_j2k` function through Python's standard-library `ctypes`. This requires the built shared INDIGO library. It avoids carrying a near-pole precession offset into a different target declination. The converted scalars are sent back to the script; `Sequence.slew()` and real mount updates still establish arrival and transit. No transit/flip events are fabricated.

The script assigns every public builder method runtime cases with readbacks and expected failure paths. It also exercises nested/empty loops, recovery inside repeated blocks, both conditional branches, explicit failure-policy changes, wait/capture pause/resume, wait/capture/altitude/transit-pause aborts, restart, preview, all acquisition overloads, FITS header addition/removal and value, real autofocus with distinct saved batch values, guider calibration, guiding/dithering, natural-transit flips with and without solver correction, mount/dome/GPS/rotator operations, and private configuration/driver operations. Solver-assisted flip requires `--index-dir`. Consult actual results before claiming a scenario passed. Feature switches establish option behavior; they do not establish physical joystick response, actual observatory protection or hardware performance. Guiding resume across a natural flip is not certified by these scenarios. The existing native regression suite provides complementary injected failure/race scenarios.

Each case has a deadline and fresh assertions. After completion or failure, the suite aborts any remaining owned processes and observes them stop before advancing; a cleanup timeout ends the run. Default runs fail when any public method lacks a passing runtime case. A passing negative case demonstrates failure handling, not successful acquisition/solving. Consult individual case results as well as the method inventory.

`results.json` includes per-case verdicts, expected/actual sequence states, timings, method-to-case mapping, missing methods, source hashes, index names and independent artifact checks. `messages.jsonl` and `server.log` retain diagnostic evidence. Images remain under `images/`; the private configuration and copied indexes remain under `home/`. Test output may be large when copying solver indexes. Delete the output directory when its evidence is no longer needed.

For diagnosis, `--case TEXT` runs matching case names (use `|` between alternative text fragments) and `--exclude TEXT` records a deliberately reduced selection. Cases can depend on earlier simulator preparation, so a filtered device case may fail without that setup. These options never establish complete coverage. An empty selection fails. The default overall deadline is 1,200 seconds; override with `--timeout SECONDS` for slower hosts.
