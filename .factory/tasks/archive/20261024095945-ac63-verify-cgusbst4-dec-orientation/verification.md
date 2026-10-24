# CGUSB-ST4 / NYX-101 N/S orientation away from pole

Verdict: pass

The requested experiment is complete. On this setup at DEC about +70 degrees and unchanged east pier side, adapter numeric code 0 decreases declination (south), while code 1 increases it (north). Direct NYX n/s commands produce the expected positive/negative declination changes. The inversion is therefore reproducible away from the pole. This result identifies the composite adapter/cable/mount ST4 path; it does not isolate an electrical component or justify a global driver swap.

## Source, environment and authorization

Local source base: 7a586b925d05a6b6ce5ed3fe7bbc5260745cc29b, already published numeric-protocol repair. Remote original ~/indigo remains clean at 42ff049e199348b083f36a0746a7518214ba2196; no remote source/build/install was needed for this direct orientation experiment. Both serial ports were unowned and no INDIGO/PHD2 process ran. Same CGUSB-ST4 USB 04d8:ffe5 /dev/ttyACM0 at 9600 8N1 and NYX-101 /dev/ttyUSB0 at 115200 8N1, firmware 1.32.1, linked over the same ST4 cable. Test date 2026-10-24, owner zone Europe/Bratislava.

Owner requested off-pole N/S verification while keeping mapping unchanged, then explicitly approved the ~20-degree excursion to DEC +70, short pulses and return to original park, confirming free space. Current Codex agent is sole writer and sequential analyst/verifier/reviewer; no independent reviewer or delegation. Existing scoped related-work priority continues; full repository audit remains due before unrelated work/release.

## Exact procedure and evidence

Local cwd /Users/polakovic/Development/indigo. `ssh -A -o BatchMode=yes indigo@indigosky.local 'python3 -' < .factory/tasks/20261024095945-ac63-verify-cgusbst4-dec-orientation/orientation-probe.py > .factory/tasks/20261024095945-ac63-verify-cgusbst4-dec-orientation/orientation-probe.jsonl 2> .factory/tasks/20261024095945-ac63-verify-cgusbst4-dec-orientation/orientation-probe-errors.txt` exited 0. Script and raw command/reply JSONL are retained; stderr is empty. Python standard-library termios/select only; no added package or service.

1. Confirmed identities and initial parked/tracking-off/R6 status. Initial reported DEC +89*59:59.995, motors 46848:-39168, side E. Saved target 00:00:00 / +00*00:00.
2. Unparked, enabled tracking, selected low R1; set target RA 07:21:33 (current reported RA rounded to a second) and DEC +70*00:00 with Sr/Sd, then MS accepted with 0. Bounded progress polling guarded against excessive RA-axis excursion, DEC below +69 and slew timeout. No park-position redefinition, SYNC or home command.
3. Settled at RA 07:21:33.0132, DEC +69*59:59.968, side E. Waited two seconds, then idle interval and two repeated cycles: direct :Mgn2000#, adapter :Mg02000#, adapter :Mg12000#, direct :Mgs2000#. Each pulse 2000 ms, observed through 2.5-second windows with before/during/after GRH/GDH/GX94/GU/Gm queries. All trials stayed on side E and within the approved target neighborhood.
4. Allowed self-timed adapter pulses to expire; independently attempted Q, Td and hP, polled for parked status, restored R6. Final DEC +89*59:59.995, motors exactly 46848:-39168, side E, GU nNPzET160. Saved RA/DEC target restored with acknowledgements 1/1. No cleanup error.

## Measurements

Every observation below is signed reported DEC change in arcseconds. Direct NYX n means north and is independently checked to increase DEC; s decreases DEC. Idle drift is over a matching window and is negligible relative to the guide response.

| Request | Repeats | DEC changes (arcsec) | Mean (arcsec) |
| --- | ---: | --- | ---: |
| idle | 1 | -0.011 | -0.011 |
| direct_n | 2 | +14.864, +15.030 | +14.947 |
| adapter_0 | 2 | -14.552, -14.552 | -14.552 |
| adapter_1 | 2 | +14.530, +14.529 | +14.530 |
| direct_s | 2 | -14.886, -15.053 | -14.970 |

Motor-step deltas agree with signed DEC away from the pole: direct n +89/+90, adapter 0 -87/-87, adapter 1 +87/+87, direct s -89/-90; idle DEC motor delta 0. All before/during/after pier-side queries report E. Thus adapter 0 matches direct s, and adapter 1 matches direct n. Reversal is consistent across repeats and exceeds idle drift by more than three orders of magnitude; it is not merely the earlier pole coordinate ambiguity.

## Interpretation and limits

The numeric frame bytes are exactly those emitted by repaired driver 3.0.0.10 for NORTH=0 and SOUTH=1, whose public-property-to-wire mapping was already verified through the real driver in the preceding repair task. This experiment deliberately uses direct serial comparison to separate mount guide semantics from adapter ST4 response. It is not another driver lifecycle acceptance run, so no run_driver_test.py/README case-count record is invented: that runner cannot perform this two-link goto/serial/ST4 cross-calibration. Existing recorded 7/7 regression and 11/11 hardware acceptance remain separate evidence. No production code, generated output, version or mapping changed.

The result is scoped to this CGUSB-ST4/cable/NYX-101 firmware 1.32.1 setup, DEC +70, east pier side and low R1 rate. It does not establish west-side behavior, other firmware, pin voltage/pinout, whether an inversion setting/cable/adapter is responsible, or optical guiding performance. No global driver N/S swap follows from it. A separate integration finding records the observed inversion and assigns isolation as the next step; the repaired numeric-encoding FND-000006 remains correctly closed.

## Acceptance and review

AC-001 pass: actual devices/firmware/exclusive access and baseline established. AC-002 pass: off-pole arrival, idle control, two repeats per numeric direction and direct counterpart, stable E side. AC-003 pass: commands, replies, coordinates and motors retained; exact park/motors, tracking off, R6 and saved target restored. AC-004 pass: mapping and all production source bytes unchanged; source diff is checked before delivery.

Script syntax passes compile(). Independent controller coordinate/motor readback is stronger evidence than a host completion timer, but remains a controller report, not optical metrology. Configured Factory support check/test/security outcomes are recorded separately; no C/driver code test rerun is claimed from this raw experiment.

Configured support checks passed: ./.factory/scripts/check (whitespace, findings integrity, ADR index, project boundaries), ./.factory/scripts/security (local security boundaries), ./.factory/scripts/test (188 tests in 102.046 s). See factory-check.txt, factory-security.txt and factory-test.txt. These validate workflow support, not hardware orientation; the physical result comes from the retained serial experiment. Syntax compile and production-source git diff check pass.
