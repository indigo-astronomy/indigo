# iEXOS-100 N/S verification

Verdict: pass

Scoped result: CGUSB numeric 0 increases reported celestial DEC (north), and 1 decreases it (south) on this iEXOS-100 plus replacement ST4 cable. Mapping unchanged. This comparison does not identify whether the original NYX inversion comes from that mount, its configuration or the original cable.

## Review identity and environment

Same Codex agent planned, wrote and verified the raw experiment; no independent reviewer or subagent. Local macOS checkout /Users/polakovic/Development/indigo, task branch parent 30d172d377007308cd2045ecf6e999f4ca6d9009 includes prior E/W evidence. Refreshed origin/master and merged it before final review: already up to date at 7a586b925d05a6b6ce5ed3fe7bbc5260745cc29b. Initial synchronization needed sandbox escalation for ORIG_HEAD.lock; authorized retry succeeded without content changes. No production code changes. Final content identities are in process.json; final commit/range check is recorded externally after commit.

RPi indigosky, Python stdlib termios/select. CGUSB 04d8:ffe5, serial 000000043B, ttyACM0, 9600 baud. iEXOS FT232R AK06KTVZ, ttyUSB0, 115200 baud; stable by-id paths used, fuser no owners. Firmware ESGvES20A01.1 Release 2021.08.08: ES1A01CB20A01, ESP8266, DST4!; configuration ESGi1152000001040040000900060000001!. Owner confirmed physical northern polar home. No running INDIGO driver was involved in these raw trials.

## Acceptance and measurements

| Criterion | Result and evidence |
| --- | --- |
| AC-001 actual devices/reference/exclusivity | Pass: USB/by-id identity, firmware/config checks, idle counts/rates [0,0], explicit owner home confirmation. |
| AC-002 operational ST4/config boundaries | Pass: actual ST4 inputs moved DEC repeatedly. No flash, EEPROM configuration, sync/counter reset or persistent setting commands. Manufacturer reference pp.15–16 documents digital ST4 hardware on model 1A-01C; the suffix alone was not used as enable proof. |
| AC-003 repeated independent comparison | Pass: nine intervals below, DEC around +70 degrees, positive raw DEC (northern W-side coordinate convention), independent PMC8 position telemetry. Direct controls are finite +/-48-count target moves, not timed pulse-guide commands; they establish direction but do not certify pulse duration. |
| AC-004 restoration and unchanged mapping | Pass: restoration readback [1,0] counts versus initial [0,0], rates [0,0], directions [1,1], before DTR closure. Residual RA +1 microstep = 0.3125 arcsecond; within recorded 2-count (0.625 arcsecond) target tolerance. Tracking off, DEC at pole. Driver/source mapping untouched. |

Model scale 4,147,200 counts/revolution = 0.3125 arcsecond/count. On the observed positive-DEC branch, celestial DEC = 90 degrees minus raw DEC angle. The finite departure target was +230400 counts, settled exactly at DEC +70 degrees; RA tracking register 1203 produced about 48 counts/s. No meridian flip or opposite-side iEXOS comparison was performed.

| Operation | Repeats | Raw DEC change | Celestial DEC change |
| --- | --- | --- | --- |
| Idle 2.6 s | 1 | 0 | 0.0000 arcsec |
| Direct finite north target | 2 | -48 each | +15.0000 arcsec each |
| CGUSB :Mg02000# (2 s) | 2 | -41 each | +12.8125 arcsec each |
| CGUSB :Mg12000# (2 s) | 2 | +41 each | -12.8125 arcsec each |
| Direct finite south target | 2 | +48 each | -15.0000 arcsec each |

Raw command/readback: iexos-orientation-probe.py/jsonl; stderr empty, SSH exit 0. measurement-summary.json retains extracted numbers. Motor counts are controller step telemetry, not optical astrometry or electrical relay measurement. ESGr1 reports the base DEC rate zero even during ST4 correction; it was not used as evidence of pulse activity. Observed position deltas establish the response. Existing numeric-repair acceptance totals remain 7 simulated and 11 hardware cases; these raw intervals are not added to them.

## Stability and exact commands

From local root: `ssh -A -o BatchMode=yes indigo@indigosky.local python3 -u - < .factory/tasks/20261024102929-75ee-verify-cgusbst4-iexos/serial-stability-probe.py`, output serial-stability.jsonl/stderr. After owner USB cable replacement: 120 samples / 480 correct position/rate replies, 55.257 seconds between first/last reply, exit 0, same ttyUSB0, no stderr. Every position and rate remained zero. First startup-only attempt had boundary NUL plus valid reply; retained new-cable-startup artifacts. Probe now records raw_hex and strips only boundary NUL/CR/LF, matching the motion probe framing. No USB EIO occurred in the successful stability or motion run. post-run-usb.txt retains by-id and kernel tail; no new disconnect since replacement enumeration.

Motion command from local root: `ssh -A -o BatchMode=yes indigo@indigosky.local python3 -u - < .factory/tasks/20261024102929-75ee-verify-cgusbst4-iexos/iexos-orientation-probe.py`, output iexos-orientation-probe.jsonl/stderr, exit 0. One mount FD retained through departure, all trials and proved return. Bounded finite targets, position/rate observations and explicit stop/return commands executed. No late motion or lingering probe process after completion. A separate local stdlib JSON review checked 480 replies, all nine expected trial signs/positive branch, zero idle drift and final restoration/cleanup.

Syntax: initial local py_compile hit macOS cache-directory sandbox restriction; rerun with PYTHONPYCACHEPREFIX=/tmp/iexos-pycache passed. Final source syntax is checked without bytecode writes in the final check run. Matching official 20A01.1 archive download returned HTTP 403; no installation occurred. Protocol reply mapping was verified against the bundled manufacturer Programmer Reference Release 2, pp.28–30 using remote pdftotext; firmware observations remain the current-system evidence.

## Failed attempts retained and recovery

Attempt 1 incorrectly expected setters to echo their family. ESPt was accepted but replied ESGt, causing the script assertion; cleanup aborted/stopped the axes but its same assertion skipped physical return, then DTR closure reset counters. This script error lost reference; owner was informed and explicitly restored physical polar home. No claim that post-reset zeros proved restoration. attempt-1.py/jsonl/stderr preserve the failure. This is not a production-driver defect.

Developer corrected documented ESPt/ESSd/ESSr/ESTr replies to ESGt/ESGd/ESGr/ESGx. Cleanup now attempts every restoration step despite individual reply failures, verifies final readback and holds DTR session open for operator recovery if return cannot be proved. Attempt 2 stopped at preflight because a point to existing zero returned -1 microstep; cleanup proved exact [0,0], zero rates/directions [1,1]. Applied existing two-count arrival tolerance consistently rather than changing polarity expectations. attempt-2 artifacts retained.

Attempt 3 failed before movement with USB EIO/-71 and re-enumeration. An old-cable read-only soak also failed after 22 complete samples/88 replies. attempt-3 artifacts, old-cable-stability files and usb-disconnect*.txt preserve the evidence. Owner replaced cable. The successful stability/experiment above are the retest; failed runs are not counted as passes. No ST4 pulses occurred in failed attempts.

## Repository checks

Final task checks/results are appended below after execution. No native rebuild/driver acceptance rerun is required for evidence/docs-only changes; no C/.driver/test production sources were edited. Scope excludes off-limits drivers and indigo_tests. Full repository audit remains due under the owner-authorized related FND-000006 hardware continuation; this task review does not reset it.

Final configured runs from repository root: ./.factory/scripts/check exit 0 (diff whitespace, findings, decision index and project boundaries); ./.factory/scripts/test exit 0, 188 tests in 109.464 seconds; ./.factory/scripts/security exit 0. Exact captured stdout/stderr: check.log, test.log, security.log. Final retained Python sources parse using compile() without executing hardware or writing bytecode. Separate git diff --check and findings check passed.

An unnecessary findings check --prune-task invocation returned 2 because an active report would need pinning before deletion. No report is being deleted/pruned: completed evidence is retained by the archival routine, so ordinary findings check is the applicable gate and passed. No registry reset or pin bypass. Archive link checks and final immutable process gate follow before delivery.
