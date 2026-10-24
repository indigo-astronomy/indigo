# CGUSB-ST4 / NYX-101 hardware verification

Verdict: pass

This verdict covers the requested experimental investigation, not a production fix or complete guiding acceptance. FND-000006 is confirmed and remains open for repair. No production source changed.

## Environment and authorization

Date: 2026-10-24, Europe/Bratislava. Local and clean remote source baseline: 42ff049e199348b083f36a0746a7518214ba2196. Host: Codex desktop, single writer acting sequentially as analyst/verifier/reviewer; no independent agent review. RPi: indigosky, Debian aarch64, kernel 6.12.87+rpt-rpi-2712. CGUSB-ST4 USB 04d8:ffe5, ttyACM0, 9600 8N1; NYX-101 ttyUSB0, 115200 8N1, firmware 1.32.1. Device identity came from USB by-id links and NYX GVP/GVN; adapter ACK 0x06 returned A. Both serial ports had no fuser owners; no INDIGO server ran. Python serial module is absent; only standard-library serial access was used.

Owner explicitly approved unpark, tracking, at most two-second ST4 and direct NYX guide pulses, tracking off and repark, and confirmed free space. Automatic approval initially rejected physical motion before this explicit approval; no motion was performed until approval arrived. Owner separately deferred the overdue repository audit until after this task; the audit checkpoint is unchanged.

## Commands and retained evidence

All local commands ran from /Users/polakovic/Development/indigo. Each script was supplied on stdin with `ssh -A -o BatchMode=yes indigo@indigosky.local 'python3 - [full]' < TASK/SCRIPT > TASK/LOG`, where TASK is .factory/tasks/20261024064954-2f11-verify-cgusbst4-protocol and optional `full` is a positional argument to `python3 -`, not shell syntax.

| Exact script | Argument | Output | Purpose |
| --- | --- | --- | --- |
| probe.py | none | probe-initial.jsonl | Idle, letter south, digit south and direct north, 2000 ms |
| probe-small.py | full | probe-small.jsonl | Every direction in both dialects, four direct controls, 500 ms with space padding |
| probe-dec-repeat.py | none | probe-dec-repeat.jsonl | DEC repeated with zero/space padding, 500 ms |
| probe-guide-rate.py | full | probe-guide-rate.jsonl | Every direction, lower R1 manual/ST4 preset, zero padding, 500 ms |
| probe-dec-stable.py | none | probe-dec-stable.jsonl | Repeated opposite DEC directions on stable reported pier side, R1, 500 ms |

All five SSH experiment commands exited 0. Logs record every transmitted command, raw reply, monotonic timestamp, before/during/after snapshots and cleanup status. GRH/GDH are controller-reported coordinates; GX94 supplies independent controller motor-step counts. These demonstrate controller movement, not an optical/encoder measurement or direct ST4 electrical measurement. Serial settings are restored on normal script exit. Scripts are retained execution evidence, not a hardened reusable hardware test harness.

## Results

Letters n/s/e/w never activated NYX ST4 status `g` and never changed DEC motor position or RA position beyond idle tracking. With 500 ms commands and 0.9 s observation windows, idle RA advanced 142–144 steps; letter trials advanced 143–145 steps and changed DEC by zero steps. INDIGO currently emits these ineffective letters.

Digits 0/1/2/3 all activated ST4 status `g` and changed motor position. At the initial R6 (64x) manual/ST4 preset the movement was much larger than a direct serial pulse at GX90=0.50. The numeric dialect was accepted with both a four-digit duration and the driver's space-padded %4d form. Near-pole coordinates and changing reported pier side confound celestial-direction interpretation. R1 (0.5x) reduced ST4 movement; original R6 was restored after each reduced-rate run.

Selected motor-step evidence from the R1 run (all 500 ms):

| Command target | Command | RA delta | DEC delta |
| --- | --- | ---: | ---: |
| Idle | none | +144 | 0 |
| Adapter east letter | :Mge0500# | +144 | 0 |
| Adapter digit 2 | :Mg20500# | +113 | 0 |
| Adapter west letter | :Mgw0500# | +144 | 0 |
| Adapter digit 3 | :Mg30500# | +175 | 0 |
| Direct NYX east | :Mge500# | +105 | 0 |
| Direct NYX west | :Mgw500# | +181 | 0 |

Thus digits 2 and 3 produce opposite RA corrections with the same signs as direct NYX east/west controls. No equality of pulse calibration/timing is claimed.

The last experiment holds the reported side W across all four DEC comparison trials:

| Adapter command | DEC motor delta | Reported DEC delta |
| --- | ---: | ---: |
| :Mg00500# | +19 | -0.249 arcsec |
| :Mg10500# | -18 | +0.241 arcsec |
| :Mg00500# | +18 | -0.240 arcsec |
| :Mg10500# | -18 | +0.240 arcsec |
| Direct NYX :Mgs500# | +22 | -0.315 arcsec |
| Direct NYX :Mgn500# | -22 | +0.316 arcsec |

Digits 0 and 1 therefore do drive opposite DEC directions. On this setup and side, their effects are reversed relative to PHD2's nominal NORTH=0, SOUTH=1 labels: 0 matches direct south and 1 direct north. Earlier apparent same-direction results occurred across reported-side changes; those raw observations remain retained. This experiment does not identify whether the inversion is in the adapter, ST4 cable, NYX configuration/firmware, or near-pole coordinate handling. It does not justify silently swapping N/S globally in the driver.

## Recommendation and limits

Replace letter encoding with the numeric dialect supported by the adapter, retaining the four-character duration field. Upstream PHD2 maps N/S/E/W to 0/1/2/3 (see upstream-reference.md). Before claiming full compass-direction acceptance or choosing a global DEC swap, verify ST4 wiring/inversion and repeat away from the pole with stable pier side. No slew away from the pole was authorized or performed in this experiment. Direct controls distinguish the silent-letter defect from a disconnected/unresponsive mount. Guide timing, overlap/replacement, zero cancellation, firmware variants and other adapters are outside this verification.

AC-001 pass: hardware identified, ports unused, handshake and mount replies obtained. AC-002 pass: letter/digit comparison in all four directions, idle and direct controls, repeated DEC and padding comparisons. AC-003 pass: exact scripts/logs retained, recommendation qualified, no production modification. Full physical-direction certification remains explicitly unclaimed.

## Cleanup

Every run requested park successfully and ended with `GU=nNPzET160`: parked, no goto, not tracking, original R6 restored where changed. Final logged motor state comes from controller queries; no service started or stopped, no package installed, no remote repository files changed. SSH agent forwarding was owner requested; no agent credentials or private configuration were logged.

## Local checks

- Python compile() syntax checks passed for all retained scripts. Initial py_compile failed to create a cache outside writable roots; this was replaced with compile(), not counted as a passing py_compile run.
- ./.factory/scripts/check: pass (diff whitespace, findings integrity, ADR index and project boundaries).
- ./.factory/scripts/security: pass (configured Factory boundary validation; not hardware or general security certification).
- ./.factory/scripts/test: pass, 188 tests in 105.290 seconds. These check Factory support, not guide behavior.
- Final registry/diff/process checks are recorded in delivery output after the documentation and registry updates.
