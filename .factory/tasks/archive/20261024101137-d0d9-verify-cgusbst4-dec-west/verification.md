# CGUSB-ST4 / NYX-101 west-side N/S verification

Verdict: pass

The requested W-side experiment is complete. At DEC about +70 degrees, actual and stable Gm=W, numeric adapter code 0 decreases DEC (south), code 1 increases DEC (north). Direct NYX n/s guide commands retain their expected positive/negative DEC effects. This matches the prior E-side result: reversal is not limited to the pole or to the east pier side. The responsible component of adapter/cable/NYX integration remains unisolated; no driver mapping change is implied.

## Source, setup and authorization

Task source base 344aba0b57889fd2dfa0dc3e94e19530252e0dcd includes published repair 7a586b925 and prior E-side orientation evidence. Master remains 7a586b925. Remote source remains the clean original 42ff049e1; this raw two-link experiment changes no source/install. Same CGUSB-ST4 (USB 04d8:ffe5, ttyACM0 9600 8N1) and NYX-101 (ttyUSB0 115200 8N1, firmware 1.32.1), unchanged ST4 cable. Serial ports unowned and no INDIGO/PHD2 process running. Date 2026-10-24, Europe/Bratislava.

Owner requested side W and unchanged mapping, then explicitly approved the possible meridian flip/larger both-axis travel, short pulses and return to original park, confirming full movement clearance. The current Codex agent is sole writer and sequential analyst/verifier/reviewer; no independent review. Actual model/effort/cost unknown, no delegation or extra charges. This related continuation retains the owner priority; full audit remains due before unrelated work/release.

## Exact procedure and evidence

Local cwd /Users/polakovic/Development/indigo. Command: `ssh -A -o BatchMode=yes indigo@indigosky.local 'python3 -' < .factory/tasks/20261024101137-d0d9-verify-cgusbst4-dec-west/west-orientation-probe.py > .factory/tasks/20261024101137-d0d9-verify-cgusbst4-dec-west/west-orientation-probe.jsonl 2> .factory/tasks/20261024101137-d0d9-verify-cgusbst4-dec-west/west-orientation-errors.txt`. Exit 0, stderr empty. Retained script uses Python standard-library termios/select only, no package or service changes. Every serial command/reply and before/during/after coordinate/motor/status/side read is in JSONL.

1. Read-only baseline: NYX-101 1.32.1, nNPzET160, DEC +89*59:59.995, side E, motors 46848:-39168. Both meridian-limit overlap queries GXE9/GXEA returned 0. The bundled NYX command protocol documents MNw only within overlap and MD as the target-side predictor; no limit setting was changed or invalid flip forced.
2. Saved original target, unparked, enabled tracking, selected R1. Read sidereal time GS and chose RA sidereal+1 h, DEC +70. For RA 14:33:11, MD returned 1 (destination W); only then was ordinary MS sent and accepted with 0. The alternative sidereal-1 h fallback was not needed. No MNw, SYNC, home, park redefinition or meridian-limit override command.
3. Bounded slew polling reached RA 14:33:10.9972, DEC +70*00:00.061, actual Gm=W and no-goto status NpzEW110, motors 2733301:391897. Waited two seconds for settling, then idle window and two cycles of direct :Mgn2000#, adapter :Mg02000#, adapter :Mg12000#, direct :Mgs2000#. Each 2000 ms pulse used a 2.5-second observation window; all trial before/during/after sides stayed W.
4. Let self-timed pulses expire, attempted halt/tracking-off/park independently, polled park, restored R6 and original target (both restore acknowledgements 1). Final motors exactly 46848:-39168, DEC +89*59:59.995, side E and nNPzET160. Cleanup reports no error. Physical position is restored despite the naturally changed sidereal RA of a stationary parked mount.

## West-side measurements

Signed changes are reported DEC arcseconds, independently corroborated by motor steps; no host timer interpretation is needed.

| Request | Repeats | DEC changes (arcsec) | Mean (arcsec) |
| --- | ---: | --- | ---: |
| idle | 1 | +0.002 | +0.002 |
| direct_n | 2 | +14.877, +15.043 | +14.960 |
| adapter_0 | 2 | -14.539, -14.539 | -14.539 |
| adapter_1 | 2 | +14.543, +14.542 | +14.542 |
| direct_s | 2 | -15.040, -15.040 | -15.040 |

Motor DEC deltas: idle 0, direct n -89/-90, adapter 0 +87/+87, adapter 1 -87/-87, direct s +90/+90. Direct n is celestial north because DEC increases; direct s decreases it. Numeric 0 therefore matches direct s and numeric 1 matches direct n. Stable W side and two repeats exclude the earlier moving-side/pole ambiguity from this comparison.

## Comparison with E

The retained E-side experiment at the same DEC/rate/durations measured adapter 0 -14.552/-14.552 arcsec and adapter 1 +14.530/+14.529 arcsec, with direct n positive, s negative and idle -0.011 arcsec. On W the adapter signs remain the same, while the corresponding motor-step signs reverse with pier side. This establishes reversed nominal N/S on both tested sides of this same physical ST4 stack; it does not by itself isolate CGUSB firmware pin mapping, cable continuity/pinout or NYX ST4 configuration/firmware.

The active orientation finding is updated with W evidence; no duplicate finding or reopening of repaired numeric-encoding FND-000006. No global N/S swap and no production code/generator/version change. The exact numeric frames match repaired driver 3.0.0.10 output already verified in its preceding real-driver acceptance task.

## Acceptance and limits

AC-001 pass: identities/exclusive access/initial state and targets retained. AC-002 pass: MD-approved ordinary goto, actual settled W at +70 without changing limits. AC-003 pass: matched idle plus two repeats per numeric and direct direction with stable W. AC-004 pass: original park/motors, side E, tracking off/R6 and target restored; source mapping unchanged.

This is a raw serial/ST4 cross-calibration that run_driver_test.py cannot perform, not another acceptance-suite run. Existing driver README test counts are unchanged; no manual suite pass record is fabricated. Controller readback is not optical/electrical metrology. No other adapter, cable, firmware or physical pinout is certified; root-cause isolation remains open. Support checks validate workflow metadata, not direction behavior. Script compile() passes and production source bytes are checked against the baseline before delivery.

Configured workflow support checks all passed: ./.factory/scripts/check (whitespace, findings/ADR/project integrity), ./.factory/scripts/security (local security boundaries), ./.factory/scripts/test (188 tests in 102.320 s). Outputs are retained as factory-check.txt, factory-security.txt and factory-test.txt. These are not additional hardware case counts. Script compile() and explicit production source-byte diff checks pass. Fresh git fetch confirms master/origin master still 7a586b925; no newer main changes to synchronize.
