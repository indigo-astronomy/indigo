# NYX-101 with the iEXOS-tested ST4 cable

Verdict: pass

Orientation measurement passed. Exact RA park-count restoration did not: 63232 instead of 63312. Owner explicitly accepted this concrete residual and leaving the mount parked; that part of AC-003 is waived, not relabelled an exact restoration pass. Mapping/code unchanged.

## Identity, authorization and revision

Same agent designed and verified this physical comparison, not independent review. Local root /Users/polakovic/Development/indigo, task branch parent 9c3e0e05d81f0c767af5f43851426b3f9c2ed1a6; fetched/synchronized origin/master 7a586b925d05a6b6ce5ed3fe7bbc5260745cc29b (already included). Working-tree changes confined to evidence and qualified finding/doc updates. Existing owner approvals explicitly cover NYX unpark/tracking, ~20-degree off-pole movement, short pulses/return and full-range clearance. Latest owner states NYX replaced iEXOS with the same ST4 cable. No new model/provider/delegation or publication authorization.

CGUSB 04d8:ffe5 serial 000000043B ttyACM0, 9600 baud. NYX467edc0c ttyUSB1, 115200 baud, firmware 1.32.1. Stable by-id paths, no fuser/server/PHD2 owners. Initial state nNPezET160, DEC +89*59:59.995, side E, motors 63312:-39168. Stored target 00:00:00 / +00*00:00. The extra initial e flag is retained as observed; no unsupported meaning is inferred.

## Acceptance coverage

- AC-001 pass: actual identities, parked/not tracking/R6 near pole, exclusive links, owner cable continuity statement.
- AC-002 pass: one idle plus two repeats each direct n/s and adapter 0/1 at DEC +69*59:59.899 and stable reported side E. Ordinary goto kept current RA and required no forced side/meridian-setting change. Two-second frames and independent coordinate/motor readbacks retained below.
- AC-003 partial: park/tracking-off/R6, DEC, side E and target restored. Exact RA motor-count equality failed by -80. Script correctly exits 1 and retains the error; no counter sync or park rewrite to hide it. Four subsequent read-only samples confirmed stable final 63232:-39168, nNPzET160, DEC +89*59:59.995, side E. Owner explicitly replied “Áno, akceptujem zaparkovanie s týmto rozdielom” to the concrete 63232-versus-63312 residual. This is the AC-003 exact-RA-return waiver; no persistent park rewrite authorized or performed.
- AC-004 pass: no production code or mapping change; same-cable comparison qualifies the existing open NYX orientation finding without claiming an isolated firmware defect.

## Measurement

| Two-second operation | Reported DEC change (arcsec), repeats 1 / 2 | DEC motor steps |
| --- | --- | --- |
| Direct NYX :Mgn2000# | +15.037 / +15.030 | +90 / +90 |
| CGUSB :Mg02000# | -14.552 / -14.545 | -87 / -87 |
| CGUSB :Mg12000# | +14.529 / +14.529 | +87 / +87 |
| Direct NYX :Mgs2000# | -15.054 / -15.053 | -90 / -90 |

Matched idle interval: -0.012 arcsecond, zero DEC motor steps. Every before/during/after pier readback was E. RA tracked during trials. Current cable gave opposite signs on iEXOS: 0 north +12.8125 arcseconds, 1 south -12.8125, twice. The same adapter/cable now shows inverted DEC response on NYX relative to direct NYX n/s and reported celestial DEC. The original cable alone cannot explain the reversal. NYX ST4 input interpretation/configuration/firmware remains to be isolated; no global adapter N/S swap is justified. No optical astrometry/electrical contact measurement or second-side repeat on the replacement cable is claimed.

## Exact execution and cleanup

From repository root: `ssh -A -o BatchMode=yes indigo@indigosky.local python3 -u - < .factory/tasks/20261024105327-f79c-verify-nyx-same-st4-cable/same-cable-probe.py`, stdout same-cable-probe.jsonl and stderr same-cable-errors.txt. Exit 1 solely because added exact park motor/side equality check found RA residual. Raw trace includes all nine measurement intervals, cleanup_final and restored_target; measurement-summary.json retains extracted values. No failure was relabelled a transport pass.

Cleanup issued :Q#, :Td#, :hP#, polled parked/not tracking, restored R6 and saved target/readback. Final state nNPzET160, motors 63232:-39168 versus 63312:-39168 initially, DEC unchanged +89*59:59.995, side E, target 00:00:00/+00*00:00. No additional recovery movement or persistent park/configuration rewrite was attempted. Manufacturer NYX101.pdf pp.8,12–13 read with remote pdftotext confirms status, raw motor position and pulse/stop commands; it did not establish a cause or tolerance for this park residual.

Independent local stdlib JSON review confirmed nine intervals, two expected signs for each source, matching motor signs, stable E and the -80/0 final residual; output measurement-summary.json. This review passes orientation only, not the failed exact-return assertion. No native rebuild or driver acceptance rerun because production and acceptance sources unchanged; raw trials do not alter existing numeric-repair test totals.

## Repository validation

`./.factory/scripts/test` from root: exit 0, 188 tests in 103.643 seconds, factory-test.txt. Configured check and security groups also exited 0; factory-check.txt and factory-security.txt retain exact output. Final diff whitespace, findings integrity, decision index and project boundaries passed. Same-agent quality/security reviews are in quality-review.md/security.md. The overdue full audit remains deferred only for this owner-prioritized related hardware investigation; no audit reset.

Final read-only confirmation command: ssh -A -o BatchMode=yes indigo@indigosky.local python3 -u - < .factory/tasks/20261024105327-f79c-verify-nyx-same-st4-cable/park-confirmation.py, exit 0; park-confirmation.jsonl retains four stable parked/not tracking/R6/E snapshots at motors 63232:-39168 and DEC +89*59:59.995. No additional motion. Final committed content/process gate and clean-worktree confirmation are recorded externally after commit; structural validation does not independently authenticate owner approvals or hardware tests.
