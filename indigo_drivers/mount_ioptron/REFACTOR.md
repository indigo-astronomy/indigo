# iOptron mount simulator & test coverage

## Baseline and scope

2026-09-15, macOS (Darwin 25.6.0), Apple clang, universal `-arch x86_64 -arch arm64`.
Task: finish the host-side serial simulator so it covers **all six protocol
dialects** and simulates **real mechanics** (elapsed-time GOTO, manual motion,
tracking drift, park/home motion), and add **complete applicable test
coverage** per `indigo_test/DRIVER_TESTING_RULES.md` (Mount + Guider standards).
This is test-infrastructure work; the generated driver is preserved unless a
concrete defect is found and recorded below.

Driver: `indigo_mount_ioptron` (generated from `indigo_mount_ioptron.driver`,
`version = 48` at baseline, bumped to `49` = `DRIVER_VERSION 0x03000031` for the
defect fixes below). Exposes a **mount** logical device and a **guider** logical
device sharing the same serial connection. Protocol dialects (enum
`protocol_type`): `HC_8406`, `HC_8407`, `V1_0`, `V2_0`, `V2_5`, `V3_0`, chosen
either by the persistent `PROTOCOL_VERSION` switch or by autodetection
(`:MountInfo#` product code + `:FW1#` firmware matched against the `PRODUCTS`
table, with `:GLS#` length as a fallback discriminator). Serial transactions run
on the shared device queue; polling (`mount_timer_callback`) and guide-pulse
completion (`guider_*_finalizer`) use real INDIGO timers.

No hardware testing is planned or performed (decision recorded below). Linux and
Windows execution are unavailable in this environment; the simulator is a
portable POSIX pseudo-terminal program built for arm64/x86_64.

Baseline build/test (before changes):
- `make -C indigo_test build/integration/test_mount_ioptron_simulator` — OK (clean).
- `indigo_test/build/integration/test_mount_ioptron_simulator` — **3/3 passed**
  (`ioptron_mount_connects_all_protocol_dialects`,
  `ioptron_protocol_3_mount_passes_serial_compliance_checks`,
  `ioptron_guider_passes_serial_compliance_checks`).

## Current-state audit

### Driver protocol inventory (commands actually sent)

- Identity/detect: `:MountInfo#` (4-char product), `:FW1#` (12-char firmware),
  `:V#` (`V1.00#`), `:GLS#` length fallback.
- Coordinates readback: `:GR#`/`:GD#`/`:pS#` (8406/8407), `:GEC#` (V1.0/2.0/2.5),
  `:GEP#` (3.0). Units: GEC dec = mas/10 signed, ra = time-mas (÷15) ; V1.0 ra 9
  digits ÷ extra 10, V2.x ra 8 digits; GEP dec/ra = mas/10 with side-of-pier +
  pointing-state trailing chars.
- Status: `:SE?#` (8406 slewing), `:AP#`/`:AH#`/`:AT#`/`:SE?#` (8407),
  `:GAS#` (V1.0/2.0, status at `[1]`, track rate at `[2]`), `:GLS#`
  (V2.5 status `[14]`/rate `[15]`, V3.0 status `[18]`/rate `[19]`).
- Motion: `:MS#`/`:MS1#` GOTO, `:CM#`/`:CMR#` sync, `:SR%d#` slew rate,
  `:m%c#` (n/s/e/w) manual move, `:Q#`/`:q#`/`:qR#`/`:qD#` stop.
- Set target: `:Sr…#`/`:SRA…#` (RA), `:Sd…#` (DEC) with per-dialect encodings.
- Park/home: `:PK#` (8406), `:MP1#`/`:MP0#` park/unpark, `:MH#` home (go to zero),
  `:MSH#` search home, `:SPA#`/`:SPH#` park position set, `:GAC#` alt/az.
- Tracking/rates: `:ST%c#` on/off, `:RT%d#`/`:STR%d#` rate, `:RR…#` custom rate,
  `:GTR#` custom rate readback, `:QT#` (8407 rate), `:RG…#`/`:AG#` guide rate.
- Time/site: `:SL…#`/`:SC…#`/`:SG…#`/`:SDS…#`/`:SUT…#` set time,
  `:SLA…#`/`:SLO…#`/`:St…#`/`:Sg…#` set site. (The `:GC/:GL/:GG/:Gg/:Gt/:GLT/:GUT`
  getters are commented out in the driver; UTC is derived from host time.)
- Meridian (V2.5/3.0): `:SMT%c%02d#`, `:GMT#`. PEC (V3.0 non-encoder):
  `:SPP%c#`/`:SPR%c#`. Guide pulses: `:Mn/Ms%05d#`, `:Me/Mw%03d#` (no reply).

### Existing simulator gaps (driver-dir `mount_ioptron_simulator.c`)

1. **No real mechanics** — `:MS#`/`:MS1#` set `current = target` instantly and
   clear `slewing`; GOTO/park/home never report a BUSY/slewing phase.
2. **Manual motion ignored** — `:mn/ms/me/mw#` return nothing and do not move the
   axes; `:qR/:qD#` do nothing; abort cannot be observed on coordinates.
3. **No tracking drift** — RA does not advance when tracking is off.
4. **Wrong `:GLS#`/`:GAS#` layout for V2.5/V2.0** — the shared "else" branch puts
   the status digit at the wrong offset for V2.5 (`[14]` lands inside the numeric
   longitude field), so tracking/park/home state is never reported for V2.5.
5. **Guiding not modelled** — pulses accepted but no motion/telemetry.
6. **No fault injection / command-event log** — cannot assert exact commands or
   exercise malformed/dropped replies, timeouts, or recovery.
7. **Coverage** — the integration test connects all dialects but does not assert
   the detected model/firmware per dialect, GOTO BUSY→OK with coordinate arrival,
   SYNC readback, manual motion + abort on coordinates, park/home motion,
   tracking/rate readback, guide-rate ranges per dialect, guiding-pulse timing,
   or transport-failure recovery.

### Firmware ground truth

Per-dialect Arduino sketches under `mount_ioptron_simulator/mount_ioptron_*_simulator/`
(shared `common.h`) are the authoritative wire formats and mechanics model
(RA/DEC in milliarcseconds, rate-integrated `loop()`, park/home slew to the
zero position). The PDF protocol manuals (`Protocol_V1.0/1.01/1.4/2.0/2.5/3.0/3.01/3.10`,
`Protocol_Special_Mode`) corroborate command syntax and status codes.

## Hardware-test decision

**No hardware testing.** No iOptron mount is available in this environment. All
validation is simulator-backed. Hardware acceptance (real slew/park/home, guider
electrical timing) is documented as a gap, not claimed.

## Atomic plan

1. COMPLETE — Baseline, driver/protocol audit, firmware/PDF review (above).
2. COMPLETE — Create this `REFACTOR.md` before production/test changes.
3. COMPLETE — Rewrote `mount_ioptron_simulator.c`: exact per-dialect wire formats
   (identity, `GR`/`GD`/`pS`, `GEC`, `GEP`, `GAS`, `GLS` with correct status/rate
   offsets, guide-rate/`QT`/`GTR`, meridian, PEC), `serial_motion.h`-based
   elapsed-time GOTO/park/home with a pending-target model, sidereal drift,
   manual motion with rate selection and stop, guide-pulse acceptance, plus
   control-file fault injection and a command-events log (LX200 simulator model).
   Builds `-Wall -Wextra -Werror` for arm64/x86_64.
4. COMPLETE — Makefile: added `serial_motion.h` dependency, strict `IOPTRON_TEST_CFLAGS`,
   `-lm`, and a `test_mount_ioptron_simulator_sanitize` ASan/UBSan target. The
   simulator stays in the driver directory (the repository norm).
5. COMPLETE — Expanded `test_mount_ioptron_simulator.c` to the Mount + Guider
   acceptance checklist. Scenario→test mapping below.
6. COMPLETE — Strict + sanitizer builds, full suite (13/13) and sanitizer suite
   (13/13) pass; generator reproducibility confirmed (no unexplained diff);
   `MIGRATION_STATUS.md` updated to 13 / 0; `REFACTOR.md` registered in Xcode.

## Scenario → test mapping (DRIVER_TESTING_RULES Mount + Guider)

- Identity/model/firmware per dialect, capability visibility (tracking, guide
  rate + ranges, park/unpark, home/search, side-of-pier, slew rate),
  SYNC + readback, GOTO BUSY→arrival, tracking on/off + track-rate selection,
  manual motion (`:m..#`) + abort, park/unpark, home + home-search, reconnect:
  `ioptron_hc8406_profile`, `ioptron_hc8407_profile`, `ioptron_protocol_1_profile`,
  `ioptron_protocol_2_profile`, `ioptron_protocol_25_profile`, `ioptron_protocol_3_profile`.
- Manual `PROTOCOL_VERSION` selection of every dialect: `ioptron_forced_protocol_selection`.
- GOTO progress, abort mid-slew (not reaching target), fresh GOTO after abort:
  `ioptron_goto_progress_abort_and_restart`.
- Failed slew reply, dropped slew reply, failed SYNC, each recovering:
  `ioptron_coordinate_command_failures_recover`.
- Meridian handling/limit (`:SMT#`), PEC enable/disable (`:SPP#`), custom-rate
  value readback: `ioptron_v3_options_meridian_pec_custom_rate`.
- Queued PARK/HOME cancelled by abort: `ioptron_protocol_3_abort_queue_checks`.
- Guider directions, overlap rejection, and guide-pulse duration accuracy:
  `ioptron_guider_directions_overlap_and_timing`.
- Guider property/interface compliance: `ioptron_guider_passes_serial_compliance_checks`.

Non-applicable / hardware-only: physical slew/track/PEC accuracy, real relay
guide-pulse timing, and real serial timeouts (only software timing is measured;
see guider note below). No iOptron hardware was available.

## Found defects

1. **MOUNT_GUIDE_RATE hidden on HC 8407** (source audit, fixed). `ioptron_init_mount`
   set the RA guide-rate range and read `:AG#` for the `HC_8407` branch but never
   set `MOUNT_GUIDE_RATE_PROPERTY->hidden = false`, unlike every other dialect, so
   an 8407 hand controller silently omitted the mount guide-rate property.
   Fix: unhide it and set `count = 1` in the `.driver` `HC_8407` branch; regenerated.
   Regression: `ioptron_hc8407_profile` asserts the property, range and a write.
2. **HC 8406 could not connect while idle** (reproduced, fixed). `ioptron_get_state`
   for `HC_8406` returned `true` only when `:SE?#` reported slewing; an idle mount
   made it return `false`, and `ioptron_init_mount` treats that as a connection
   failure. Fix: return `true` on a successful `:SE?#` and derive `slewing` from the
   reply (matching the other dialects). Regression: `ioptron_hc8406_profile` and
   `ioptron_forced_protocol_selection` connect the idle 8406 dialect.
3. **PROTOCOL_VERSION only defined after connect** (fixed, user-directed). The
   persistent protocol switch was defined in the connection handler, so a dialect
   could not be selected before connecting (required for the non-autodetectable HC
   controllers). Fix: `always_defined = true` on the `MOUNT_PROTOCOL` switch;
   regenerated. Regression: `ioptron_forced_protocol_selection`.

### Stage 2 defects (reproduced by the v2 suite, fixed in version 50)

| ID | Impact | Root cause | Fix | Regression |
| --- | --- | --- | --- | --- |
| IO04 | HC 8406 not autodetected (fell back to 1.0 and failed); dropped `:Q#`/ack replies reported success | `ioptron_simple_reply_command()` returned true for a timed-out 0-byte read | require at least one byte (`result > 0`) | `ioptron_profile_8406`, `ioptron_dropped_stop_acknowledgement_alerts`, `ioptron_initial_readback_failures_alert` |
| IO05 | 2.0/2.5/3.0 SYNC below +/-27.8 deg sent sign+7 digits, rejected by firmware | `:Sd%+08.0f#` in `ioptron_sync()` | `:Sd%+09.0f#` | profiles 0200/0205/0300, `ioptron_coordinate_boundaries_0205/0300`, `ioptron_epoch_jnow_translation` |
| IO06 | RA guide pulses malformed on 8407/1.0/2.0/2.5/3.0 | `:Me/:Mw%03d#` | `%05d` | `ioptron_guider_commands_*`, `ioptron_guider_directions_overlap_and_timing` |
| IO07 | West longitudes rejected (sent as 180..360 deg) | `longitude += 360` | normalize to (-180, 180] | `ioptron_settings_translation_*` |
| IO08 | 3.0 `MOUNT_UTC_TIME` wrote host time | `:SUT#` computed from `JDNOW` | compute from requested seconds | `ioptron_settings_translation_0300` |
| IO09 | short/garbled status, `:AG#`, `:QT#`, `:GTR#` replies changed tracking/park state or rates (stale buffer indexes) | no length/digit validation | validated decoding (`ioptron_decode_status`, `ioptron_binary_reply`, `ioptron_get_guide_rate`, `ioptron_apply_track_rate`) committed only on success; ALERT on bad initial readback | `ioptron_status_poll_failures_*`, `ioptron_initial_readback_failures_alert` |
| IO10 | malformed coordinate replies set RA/DEC to garbage (8406/8407) or were silently ignored | unchecked `indigo_stod()`/partial `sscanf` | strict sexagesimal/fixed-width parsing; ALERT with preserved readback, OK after next valid read | `ioptron_malformed_readback_*` |
| IO11 | guide pulse write failure still completed OK | return value ignored | ALERT, pulse values cleared, no finalizer | `ioptron_guider_transport_failure_and_recovery` |
| IO12 | GOTO reported OK by a poll before the slew command was sent | poll converted the macro-set BUSY to OK | `goto_issued` flag set by accepted slew or observed slewing | `ioptron_goto_completion_waits_for_slew_command` |
| IO13 | 2.0 exposed unusable `MOUNT_PARK_SET`; 2.5 showed only CURRENT (count 1 keeps item 0) and ignored `can_park` | wrong visibility/count | hidden on 2.0; 2.5 follows `can_park` with both items | `ioptron_park_set_*`, `ioptron_detect_azmountpro_without_park` |
| IO14 | HC 8406 manual motion and guiding used commands absent from 8406 | shared 8407+ command set | `:RG#`/`:RCn#` speeds, `:Mx#`, `:Qn#/:Qe#`; timed pulses via finalizer stop; slew-rate property exposed | `ioptron_manual_motion_8406`, `ioptron_guider_commands_8406` |
| IO15 | arrow speed not re-sent after reconnect to a power-cycled mount | caches survived reconnect | reset caches in `ioptron_init_mount()` | `ioptron_slew_rate_resent_after_reconnect` |
| IO16 | King/custom tracking unreachable on 2.0/2.5/3.0 | `MOUNT_TRACK_RATE` count 3 | count 5; `:RR#` only for a valid custom rate, documented offset format on 1.4/1.0/2.0 | `ioptron_tracking_rates_*` |
| IO17 | CEM40 fw 210101 detected as 2.5, CEM60 fw 161101 as 1.0, unknown fw 140807/161101 misclassified | product table/fallback boundaries | documented dialects, strict `>` comparisons | `ioptron_detect_*` |
| IO18 | stopping one arrow axis stopped both | global `:Q#`/`:q#` | per-axis `:qR#/:qD#` (1.0+), `:Qn#/:Qe#` (8406), 8407 restarts the other axis after `:q#` | `ioptron_manual_motion_*` |
| IO19 | `MOUNT_UTC_TIME` request could be replaced by host time before being applied; UTC offset readback always 0 | poll rewrote the items while the change was queued; `utc_offset` never stored | skip rewrite while BUSY; store offset | `ioptron_settings_translation_*` |
| IO20 | hand-controller slews and parks left `MOUNT_EQUATORIAL_COORDINATES` BUSY (introduced by first IO12 repair, caught by suite) | completion required the driver flag | observed slewing also enables completion | `ioptron_tracking_state_follows_mount_status`, `ioptron_park_workflow_0205/0300` |
| IO21 | selecting CUSTOM with an unset custom rate sent `:RR00000#`/`:RR +0.0000#` and failed | missing value guard after cache reset | send `:RR#` only for a positive rate | `ioptron_tracking_rates_8407/0100/0200/0205` |

Stage 1 observations resolved in stage 2: the unreachable King/custom tracking
rate is IO16; the "ON_COORDINATES_SET TRACK→SYNC anomaly" was the GOTO polling
race IO12 (a queued GOTO handler sent `:MS1#` after a poll had already
published OK, so the test proceeded early).

Remaining observations (not changed): 8406 `:STRn#` orders solar/lunar
differently from INDIGO but `MOUNT_TRACK_RATE` stays hidden on 8406; 2.5
meridian handling is implemented but hidden because product support cannot be
distinguished; CEM60/iEQ45Pro fw 171001 → 3.0 rows are undocumented in the
bundled PDFs; `PROTOCOL_VERSION`, `MOUNT_MERIDIAN_HANDLING` and
`MOUNT_MERIDIAN_LIMIT` predate the `X_` naming rule and are kept for
compatibility; `indigo_uni_is_valid()` probes TCP with a zero-length `send()`
that does not report a peer close, so a lost `ieq://` session is not
auto-disconnected (commands fail with ALERT and a manual reconnect works;
framework change out of scope); guide-pulse completion is published ~50 ms
after the device command because `ioptron_no_reply_command()` sleeps 50 ms
before the finalizer is scheduled.

## Guider pulse-timing measurement

`ioptron_guider_directions_overlap_and_timing` issues N/S/E/W pulses of 100/200/500 ms
(6 samples/direction, n=24) over an idle shared serial connection and measures
from the simulator's receipt of the `:Mn/Ms/Me/Mw#####/###` command to the public
`GUIDER_GUIDE_*` OK callback. Typical result: mean ≈ 56–63 ms, max_abs ≈ 60–70 ms
of positive software latency (finalizer scheduling + bus round-trip), completion
always after entry and within 5 s. This is simulator/driver software timing only;
no physical relay output was measured (no hardware).

## Stage 1 test summary

Simulated tests: 13 run, 13 passed (strict `-Werror` build); the same 13 also run
and pass under AddressSanitizer + UBSan. Generator outputs reproduce with no
unexplained diff. Hardware tests: 0 run, 0 passed (no device available).
Platforms exercised: macOS universal (arm64 + x86_64). Linux/Windows not run in
this environment.

## Coverage expansion (2026-09-16)

User review: the 13-case suite is too shallow compared with
`indigo_test/integration/test_mount_lx200_simulator.c` (≈70 cases covering
metadata, initialization rollback, malformed readback, per-dialect time/site
translation, park/home workflows, shared-session ordering, transport loss,
pulse cancellation, TCP and polling races). This stage expands the simulator
contract and the suite to that depth. Hardware decision unchanged: no hardware.

Baseline of this stage: 13/13 simulated cases pass (strict and ASan/UBSan,
recorded above); driver `0x03000031`.

### Protocol re-audit (bundled PDFs extracted locally with macOS PDFKit)

Documents: `Protocol_V1.01 (8406)`, `Protocol_V1.4 (8407)`, `Protocol_V1.0`,
`Protocol_V2.0`, `Protocol_V2.5`, `Protocol_V3.0`, `Protocol_V3.01`,
`Protocol_V3.10`, `Protocol_Special_Mode` (special mode is not used by the
driver). Contracts relevant to the driver:

- HC 8406: no `:MountInfo#`/`:FW1#`; `:Sr HH:MM:SS.S#`, `:Sd sDD*MM:SS#`,
  `:Sg sDDD*MM:SS#` (east positive, west negative), `:SG sHH#`, `:MS#` → `0`
  accepted, `:CMR#` → 32-char `Coordinates     matched.        #`,
  `:SC#` → two 32-space blocks, `:pS#` → `East#`/`West#`, `:SE?#`, `:PK#`,
  `:STRn#` (0 sidereal, 1 solar, 2 lunar), manual motion is `:Mn/Ms/Me/Mw#`
  with `:Qn/Qs/Qe/Qw#`/`:Q#` (no reply) and speed `:RCn#`; no `:SR#`, `:m?#`,
  `:ST#`, `:RT#`, timed guide pulses or guide-rate commands.
- HC 8407 (1.4): `:pS#` → `0`/`1` without `#`; `:AP/:AT/:AH/:SE?#` one digit;
  `:QT#` one digit; `:AG#` → `n.nn#`; `:RGnnn#` (10..90, 100); `:q#` no reply;
  `:RR snn.nnnn#` is an offset in [-0.0100, +0.0100]; timed pulses
  `:Mn/Ms/Me/MwXXXXX#` (5 digits, all directions).
- 1.0: `:GEC#` sign+9 dec (0.01") + 9 RA (0.01 s); `:AG#` `n.nn#`; `:RGnnn#`
  10..80; `:SG sHH:MM#`; `:Sg sDDD*MM:SS#` range ±180; `:q/:qR/:qD#` no reply;
  pulses 5 digits; `:MP1#` parks at the last `:Sr/:Sd#` target; `:AH#`.
- 2.0: `:GAS#` 6 digits (7 = zero position); `:GEC#` sign+8 dec + 8 RA (ms);
  `:AG#` `nnn#`; `:RGnnn#` 10..90; `:SCYYMMDD#`, `:SLHHMMSS#`, `:SGsMMM#`,
  `:SgsSSSSSS#` range ±180°, `:StsSSSSSS#`; `:SdsTTTTTTTT#` sign+8 digits;
  `:SrXXXXXXXX#`; `:RRsnn.nnnn#` offset ±0.0100; `:MS#`; `:MP1#` parks at last
  target; `:MSH#` (CEM60); no `:SPA/:SPH/:GMT/:SMT#`; `:CM#` ignored while slewing.
- 2.5: `:GLS#` sign+6+6+6 (status [14], rate [15]); `:AG/:RGnnnn#`;
  `:SgsSSSSSS#`/`:StsSSSSSS#` ±180°/±90°; `:RRnnnnn#` multiplier 0.5..1.5;
  `:SPA#` 9 digits, `:SPH#` 8 digits, `:GAC#` sign+8 alt + 9 az;
  `:GMT/:SMT#` for CEM60/CEM40/iEQ45/iEQ30/CEM25; MS1-less `:MS#`.
- 3.0/3.01/3.10: `:GLS#` sign+8+8+6 (status [18], rate [19]); `:GEP#` sign+8 dec
  + 9 RA + side (0 east, 1 west, 2 indeterminate) + pointing (0 cw up, 1 normal);
  `:SUTXXXXXXXXXXXXX#` = (JD(UTC) − J2000) × 8.64e7; `:SLOsTTTTTTTT#` ±64,800,000
  (±180°); `:SRATTTTTTTTT#`; `:SdsTTTTTTTT#`; `:MS1#`; `:RRnnnnn#` 0.1..1.9;
  `:q#` and `:V#` removed; CEM40 fw 210101+ and GEM45/GEM28/CEM26/CEM70 are 3.10.

### Newly suspected defects (source/protocol audit, to be reproduced)

| ID | Area | Suspected defect |
| --- | --- | --- |
| IO04 | transport | `ioptron_simple_reply_command()` treats a timed-out empty read (0 bytes) as success; HC 8406 autodetection falls back to 1.0 and dropped acknowledgements (`:Q#`) are hidden. |
| IO05 | SYNC | 2.0/2.5/3.0 SYNC sends `:Sd%+08.0f#` (sign+7 digits below 27.8°) instead of sign+8. |
| IO06 | guiding | RA pulses use `:Me/:Mw%03d#`; every dialect with timed pulses requires 5 digits. |
| IO07 | site | West longitude is converted to 180..360°, outside every documented range. |
| IO08 | time | 3.0 `MOUNT_UTC_TIME` writes send host `JDNOW` instead of the requested time. |
| IO09 | status | `:GAS#`/`:GLS#` status/rate indexes are read without length/digit validation (stale buffer, spurious tracking/park changes). |
| IO10 | readback | 8406/8407 `:GR#`/`:GD#` are parsed with unchecked `indigo_stod()`; malformed replies overwrite coordinates. |
| IO11 | guiding | Guide-pulse write failures are ignored; the finalizer publishes OK. |
| IO12 | GOTO | Polling can publish OK for a queued GOTO (BUSY set by the change macro) before the slew command is sent. |
| IO13 | capabilities | 2.0 exposes `MOUNT_PARK_SET` although 2.0 has no `:SPA/:SPH#`. |
| IO14 | 8406 motion | 8406 manual motion sends `:SR#`/`:m?#` and pulses `:Mn#####`, none of which exist in 8406. |
| IO15 | lifecycle | `last_slew_rate`/`last_tracking_rate`/`last_custom_tracking_rate` caches survive reconnect, so a power-cycled mount does not receive `:SR#`/`:RR#`. |
| IO16 | rates | 2.0/2.5/3.0 keep `MOUNT_TRACK_RATE` count 3 (King/custom unreachable) although all document `:RT3#/:RT4#`; 8407/1.0/2.0 `:RR#` uses a multiplier and wrong width instead of the documented offset `snn.nnnn`. |
| IO18 | manual motion | Stopping one manual axis sends the global `:Q#`/`:q#`, stopping the other axis while its property stays BUSY. |
| IO17 | detection | Product table maps CEM40/CEM40EC fw 210101 to 2.5 (3.10 document: 3.x) and CEM60/CEM60EC fw 161101 to 1.0 (2.5 document: 2.5); unknown-product fallback misclassifies firmware exactly 140807/161101. |

Observations not planned for change: 8406 `:STRn#` lunar/solar order differs
from INDIGO order but `MOUNT_TRACK_RATE` is hidden on 8406; 2.5 meridian
handling is implemented but hidden because product support cannot be
distinguished reliably; CEM60/iEQ45Pro fw 171001 → 3.0 rows are not covered by
bundled documents.

### Atomic plan (stage 2)

7. COMPLETE — Simulator v2 (contract version 2): strict per-dialect command
   availability and payload width/range validation (violations logged as
   `?cmd`/`!cmd`), opt-in TCP transport (`ieq://127.0.0.1:port`, CONNECT/CLOSE
   events), `--product/--firmware/--no-mountinfo/--parked/--tracking/--away/
   --track-mode/--guide/--slew-rate/--altitude-limit`, multi-rule one-shot/sticky/
   DELAY/DROP injection and `@status`/`@close` control polled every 100 ms,
   mount clock and site (LST, side of pier, alt/az), arrow speeds 1..1440x,
   8406 `:RCn#/:RG#/:Mx#/:Qx#`, timed pulses at the guide rate, tracking-rate
   drift, park at `:SPA/:SPH#` (2.5/3.0) or last target (1.0/2.0), zero
   position home, `:CM#` ignored while slewing, per-dialect `:Q#`/`:q#`/`:qR#`/
   `:qD#` semantics. Strict `-Werror` build passed.
8. COMPLETE — Test suite v2 written test-only (101 serial cases + 3 opt-in TCP
   cases, `test_mount_ioptron_simulator --tcp`). Test-only run against driver
   `0x03000031`: **101 run, 35 passed, 66 failed**. Failing groups map to the
   suspected defects: metadata version gate; HC 8406 autodetection/motion/
   guiding/readback (IO04, IO14); 2.x/3.0 SYNC width, coordinate boundaries and
   epoch (IO05); RA pulse width (IO06); settings translation (IO07, IO08);
   status/readback validation (IO09, IO10); guider transport failure (IO11);
   GOTO polling race, overlap and progress checks (IO12); park-set visibility
   (IO13); slew-rate cache (IO15); track-rate count (IO16); product table and
   fallback boundaries (IO17); independent axis stops (IO18). Failures were
   triaged individually after the repair run (step 9).
9. COMPLETE — Driver repairs in `.driver` (version 49 → 50,
   `DRIVER_VERSION 0x03000032`), regenerated. Repair run triage: 20 of 101
   still failed; causes were test expectations (`:SRA#` payloads for 8.5 h and
   4.0 h, 2.5 park-set count, slow 8407 polling in stop checks), two simulator
   defects (`:QT#` answered by the `:AT#` branch; control file not polled while
   the pseudo terminal had no client) and three further driver defects
   (IO19–IO21 below, one of them introduced by the first IO12 repair). After
   those fixes all 20 passed individually.
10. COMPLETE — Final verification on macOS arm64 (Darwin 25.6.0):
   strict `-Wall -Wextra -Werror` universal builds of simulator and test;
   ordinary serial suite 101/101 and opt-in TCP 3/3 (`make -C indigo_test
   test-mount-ioptron-tcp` target added); ASan/UBSan (test + driver source)
   serial 101/101 and TCP 3/3 with no sanitizer report; representative x86_64
   (Rosetta) cases 5/5 (metadata, 3.0 profile, GOTO rejections, guider
   transport failure, 8407 settings); generator reproducibility: `.c`, `.h`,
   `_main.c` identical after regeneration; `git diff --check` clean;
   `PROPERTIES.md` unchanged (no property added or removed, only visibility);
   no new persistent files (simulator, test and `REFACTOR.md` already in
   Xcode); `MIGRATION_STATUS.md` 104 / 0. Linux/Windows and TSAN not run;
   race freedom is not claimed.


## Stage 2 scenario → test mapping

| Acceptance area | Cases |
| --- | --- |
| Metadata, base properties, always-defined `PROTOCOL_VERSION` | `ioptron_driver_metadata_and_base_properties` |
| Dialect profiles: identity, visibility/counts, initial guide rate, exact SYNC/GOTO commands, J2000 readback, tracking after GOTO, disconnect/reconnect, zero protocol violations | `ioptron_profile_8406/8407/0100/0200/0205/0300` |
| Manual dialect selection and wrong-dialect rollback | `ioptron_forced_protocol_selection` |
| Product table and firmware/status fallbacks, encoders, SmartEQ, AZ Mount Pro | `ioptron_detect_*` (12) |
| Open/initialization rollback, 115200 reopen, configured baud rate | `ioptron_initialization_rollback_and_reconnect`, `ioptron_configured_baudrate_requires_product_reply` |
| Initial park/tracking/track-rate/guide-rate/custom-rate readback and failures | `ioptron_initial_state_*` (5), `ioptron_initial_readback_failures_alert` |
| Epoch JNow, RA wrap and DEC limits, sign handling | `ioptron_epoch_jnow_translation`, `ioptron_coordinate_boundaries_*` (4) |
| GOTO progress/abort/restart, BUSY guard, polling race, rejections, altitude limit | `ioptron_goto_*` (5) |
| Malformed coordinate readback, side of pier from hour angle | `ioptron_malformed_readback_*` (5), `ioptron_side_of_pier_*` (3) |
| Manual motion mechanics, rates, reversal, independent axis stop, abort, speed after power cycle | `ioptron_manual_motion_*` (6), `ioptron_manual_abort_completes_both_axes_*` (3), `ioptron_slew_rate_resent_after_reconnect` |
| Tracking rates incl. King/custom, drift mechanics, state follows mount status, option failures, PEC training | `ioptron_tracking_rates_*` (5), `ioptron_tracking_state_follows_mount_status`, `ioptron_option_command_failures_recover`, `ioptron_pec_training_workflow`, `ioptron_status_poll_failures_*` (4) |
| Park workflows with parked rejection, park/home abort, park-set positions, home/search | `ioptron_park_workflow_*` (6), `ioptron_park_and_home_abort`, `ioptron_park_set_*` (3), `ioptron_home_workflow_*` (3) |
| UTC/offset/DST, host time, site incl. west longitude, guide-rate commands and rejections | `ioptron_settings_translation_*` (6) |
| Guider commands/rates per dialect, zero requests, pulse mechanics, timing under idle and GOTO load, overlap, transport failure, disconnect cancellation, shared ordering, guider-only detection, compliance | `ioptron_guider_*` (13), `ioptron_shared_connection_orders_and_last_close` |
| Transport loss, dropped acknowledgement, queued abort | `ioptron_transport_loss_and_fresh_session`, `ioptron_dropped_stop_acknowledgement_alerts`, `ioptron_protocol_3_abort_queue_checks` |
| Opt-in TCP (`--tcp`) | `ioptron_tcp_mount_commands_and_reconnect`, `ioptron_tcp_guider_keepalive_and_shared_ownership`, `ioptron_tcp_connection_loss_and_reconnect` |

Hardware-only gaps: real slew/park/home completion timing, hand-controller
behaviour on each firmware, physical guide-relay/pulse accuracy, real serial
baud switching (pseudo terminals ignore baud rate), GPS/time-source status.

## Stage 2 guider pulse-timing measurement

`ioptron_guider_directions_overlap_and_timing`, 3.0 dialect, N/S/E/W pulses of
20/100/500 ms, two retained repetitions each after one discarded 20 ms warm-up
per direction/workload, idle and while the mount GOTOs/polls; n=48. Error =
public OK callback (monotonic) − simulator command receipt − requested duration.

| Build | min | mean | median | p95 | p99/max | stddev | max abs | signed mean % | max abs % |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Ordinary | 57.024 | 66.189 | 65.316 | 70.028 | 132.220 | 10.443 | 132.220 | 133.733 | 350.060 |
| ASan/UBSan | 54.066 | 69.593 | 65.141 | 154.437 | 155.335 | 22.427 | 155.335 | 152.124 | 774.880 |

Values in ms (except %). The constant ≈55 ms offset is the 50 ms post-write
sleep plus bus latency; percentages are dominated by the 20 ms samples. This
is simulator/driver software timing only; no physical relay output was measured.

## Final test summary

Stage 2 (driver `0x03000032`): test-only run against `0x03000031` — 101 run,
35 passed, 66 failed; repair run 101 run, 81 passed, 20 failed (triaged above);
triage reruns 20 run, 20 passed; final ordinary serial 101 run, 101 passed;
ordinary TCP 3 run, 3 passed (one earlier TCP run 3 run, 2 passed before the
loss contract was corrected); ASan/UBSan serial 101 run, 101 passed; ASan/UBSan
TCP 3 run, 3 passed; x86_64 representative 5 run, 5 passed. Unique simulated
cases: 104 (101 serial + 3 TCP), all passing in the final ordinary and
sanitized runs.

Hardware tests: 0 run, 0 passed.

## Overlapping guide pulses (2026-09-20)

A guide pulse requested while another pulse on the same axis was still running was silently
discarded: the generated change branch dispatched both guide properties through the BUSY-guarded
`INDIGO_COPY_VALUES_PROCESS_PRIORITY_CHANGE` macro, so the second request never reached the handler.
`GUIDER_GUIDE_RA` and `GUIDER_GUIDE_DEC` now declare `accept_while_busy = true` and zero both axis
items in `on_change_request`, and each handler drops the finaliser of the pulse it replaces.

Without that cancellation the superseded finaliser would end the new pulse on the old deadline, and
on `HC_8406` it would send `:Qe#` / `:Qn#` into the replacement. The cancellation sits in
`on_change`, where it runs on the device queue thread and `indigo_queue_remove()` skips its blocking
wait; from `on_change_request` it would run on the bus thread and block until a running handler
finished.

`ioptron_guider_directions_overlap_and_timing` asserted that `Ms00100` was never sent after a
reversing request, which recorded the discard as expected behaviour. It now asserts the command is
sent and adds two duration-measuring cases: a 2000 ms pulse replaced after 500 ms by a 600 ms pulse
in the same direction (1259 ms measured) and by a 300 ms pulse in the opposite direction (1044 ms).

Because the request is now accepted while the property is BUSY, two requests arriving inside the
queue latency can queue two handlers that both act on the already-overwritten item values, so the
same command can reach the mount twice. The resulting pulse is still the second request's.

## MountSim 2.3 acceptance (2026-09-23)

Scope: all nine supported iOptron identifiers, sequential isolated applications through
`indigo_test/mountsim/run_mountsim.py` and `mountsim_test_common.h`.
Hardware decision: no physical hardware testing; macOS arm64 software transport only.
Baseline driver 3.0.0.53; MountSim and INDIGO pulls up to date. MountSim Debug
xcodebuild and production driver Makefile.drv builds passed. Portable 104-case
serial baseline log in `/tmp/ioptron-baseline.log` (initial
invocation from the wrong cwd was cancelled and restarted from indigo_test).
Architecture/property/lifecycle audit remains the inventory above; no migration,
queue redesign or public property addition is planned. New coverage is an opt-in
Darwin target, excluded from portable test dependencies. The existing portable
simulator covers malformed replies and fault injection; MountSim covers an
independent real protocol implementation and relay transport loss.

Atomic plan:
1. DONE: establish full portable baseline and preserve original driver
   wire logs for every model; implement named MountSim acceptance cases.
2. DONE: reproduce each discrepancy; cross-check manufacturer and independent
   INDI sources; fix only its responsible component with regression coverage.
3. DONE: run all requested models after repairs, portable and sanitizer
   regression suites, native MountSim checks, generation reproducibility.
4. DONE: register files, record per-model results, regenerate TEST_SUMMARY,
   preserve evidence, clean tests and commit without pushing.

Found defects (initial reproductions):
- MSIO1: all 2.5 profiles time out on `:CM#`; MountSim has no SYNC handler.
  Manufacturer 2.5 p8 and INDI ieqprolegacydriver.cpp sync both require `1`
  and coordinate calibration. Reproducer: ioptron_sync_goto_abort.
- MSIO2: CEM60 firmware 190716 was constructed as the 2.5 emulator, inconsistent
  with the existing driver's explicit 171001+ 3.0 branch. Initial attribution
  to the driver was withdrawn after user correction and independent INDI CEM120
  documentation corroboration. Preserve both driver firmware branches and use
  the 3.0 emulator for MountSim's advertised 190716 profile. Add portable coverage
  for 190716/3.0 alongside existing 161101/2.5. Manufacturer 2.5 and 3.01 PDF
  support lists disagree with broader firmware support described by INDI; no
  driver detection table is changed based on that incomplete PDF evidence.
- MSIO3: SmartEQ/ZEQ25 initialization fails: absent AT/QT/pS replies; legacy RA
  getter uses degrees as hours and negative sub-degree DEC loses sign. Add the
  documented queries and correct angular encodings, verified with 1.4 protocol
  and independent INDI implementations.
- MSIO4: 3.x lacks SLA/SLO, GTR, park-position and option handlers; custom rate
  selection is reported as sidereal. Extend actual state/readback, not relay replies.


Public sources:
- https://www.ioptron.com/v/ASCOM/RS-232_Command_Language2014_V2.5.pdf
- https://www.ioptron.us/ASCOM/RS-232_Command_language2013.pdf
- https://www.ioptron.com/v/ASCOM/RS-232_Command_Language2014V301.pdf
- https://github.com/indilib/indi/blob/master/drivers/telescope/ieqprolegacydriver.cpp
- https://github.com/indilib/indi/blob/master/drivers/telescope/ioptronv3driver.cpp


Additional public evidence: https://drivers.indilib.org/mounts/ioptron/cem120/cem120/

Progress: original portable baseline 104/104 passed. Expanded MountSim discovery
matrix initially 12/27 passed (3 cases × 9 models). The CEM60 190716/3.0 and
161101/2.5 portable detection cases both pass against unchanged driver 3.0.0.53.
Native motor/geometry/serial regressions pass. Model-local fixes additionally
cover SYNC encoder rebasing, home completion, configured park movement and
unparked power-up. Manual movement revealed shaft polarity being used as sky
polarity: north reversed on west pier and east reversed RA. Correct the iOptron
protocol-to-motor direction mapping; do not alter shared mechanics or other models.
Clock setters previously acknowledged but did not persist (legacy formats were
also parsed as packed numeric formats); iOptron now owns its advancing protocol
clock, independently of the host/rendering clock, with GLT/GUT regression readback.

Harness correction: Python 3.9 macOS `time.monotonic()` is process-relative,
whereas the C completion callback uses system CLOCK_MONOTONIC. Use explicit
CLOCK_MONOTONIC for relay trace timestamps. Source and local reproduction:
https://docs.python.org/3/library/time.html#time.monotonic (macOS behavior changed
in 3.10). Duration-based iOptron pulses have no OFF command; measure the documented
duration command's relay-forward timestamp to the public completion callback,
not invented ON/OFF edges or physical motor output. 72 retained samples/model:
N/S/E/W, 20/100/500 ms, 3 repetitions, discarded per-direction warmup, idle and
tracking/polling workloads. Functional checks remain independent of latency stats.

MSIO5: the expanded CEM40 manual-motion case exposed accelerated movement with
tracking disabled. `Motor.setMotorTracking` rebased its stopped accumulator to
steps that already included the entire current slew offset on every timer tick;
adding that offset again made motion depend on tick count. The native regression
in MountSim `tests/test_motor_geometry.m` asserts one application of the measured
slew offset after 20 real timer ticks and failed before the fix. Use `stop:` here,
keeping explicit GOTO/home rebases unchanged. This shared motor fix requires the
full native/control suite and a fresh nine-model acceptance matrix. Earlier matrix
was stopped after CEM40/GEM45 manual-motion failures; it is not final acceptance.
Direction/rate semantics are supported by the manufacturer manual movement and
slew-rate tables and INDI's `startMotion`/slew-rate implementations cited above;
the arithmetic reproducer isolates the simulator defect without changing driver
or protocol expectations.

### Acceptance scenario mapping and limits

| MountSim case | Evidence |
| --- | --- |
| `ioptron_identity_reconnect` | Mount interface, model/firmware, healthy coordinate/rate properties, disconnect and fresh connection |
| `ioptron_sync_goto_abort` | J2000 public coordinates transformed by the real driver, signed sub-degree SYNC, reachable GOTO with actual RA/DEC arrival, abort and fresh SYNC |
| `ioptron_manual_motion` | Four slew rates and directions, each start observed on the real wire, coordinate direction at maximum rate, simultaneous axes, independent axis stop and global abort |
| `ioptron_tracking_site_rates` | Tracking modes/on/off, site and UTC writes, site/guide-rate readback after reconnect |
| `ioptron_park_home_options` | Capability-gated park/unpark, saved park-position arrival, parked tracking rejection, actual home arrival, custom rate, meridian and PEC switches |
| `ioptron_idle_transport_loss` | Healthy idle poll, relay disconnection, coordinate ALERT/disconnection and recovery through a newly allocated PTY |
| `ioptron_active_transport_loss` | Relay loss during GOTO and a fresh command after reconnect |
| `ioptron_guider_timing_and_shared_lifecycle` | Four directions, completion timing, replacement/overlapping axes, guider alone, both logical devices, sibling survival and pending-pulse reconnect |

Malformed/short replies, fault injection, detection branches, operation failure and
queue races remain covered by the portable simulator suite inventoried above.
MountSim deliberately does not fabricate those replies through its relay. The
transport-loss expectation is a failed coordinate poll followed by explicit client
disconnect/reconnect, not an invented automatic reconnect policy. The relay failure
is neither a physical serial unplug nor an application crash.

MountSim limitations: GEM45 aliases CEM40 identity; CEM70 uses the common 3.x
simulation rather than all firmware-3.1 extensions. Meridian settings are stored,
PEC switching is modelled but PEC training acknowledges without a training cycle;
these are not claimed as mechanical meridian or physical PEC validation. Protocol
clock readback is tested independently of the host/rendering astronomy clock.
No encoders, mechanical home sensor, pointing accuracy, actual library unload,
physical guider output, Linux, Windows or x86_64 runtime are established by this
macOS arm64 run. The strict MountSim test binary is built for both macOS
architectures; execution here is arm64. Driver production sources and version
3.0.0.53 are unchanged, so regeneration and a driver version bump are not applicable.

Shared-context guider tests enumerate one logical device at a time; diagnostics
about the sibling CONNECTION not being in that cache are harness bookkeeping,
not evidence of a missing driver's property definition. Deliberate transport-loss
cases log expected PTY write errors. Baseline build also retains the existing
macOS `sprintf` deprecation in the generated driver; no unrelated rewrite was made.

Registry reconciliation: the pre-existing 3.0.0.53 source already contains 104
serial cases (three guider replacement cases were added after the older 101-case
summary). The preserved baseline log confirms 104 PASS lines. The new CEM60 case
makes 105 serial cases, plus 3 opt-in TCP cases = 108 unique portable cases.
Final status counts use the actual registry, not the stale 104 total in the prior
README/MIGRATION row. The ordinary final serial run completed 105/105.

Source baseline: INDIGO `d51dc6c08de2b3e06a9d8687c56483330bf71d78`,
MountSim `9617149934d3ef3815e0b27dd93be49df78778ce`. The original MountSim
12/27 matrix log includes full driver wire-debug exchanges. The initially named
portable baseline trace directory was not created and is not claimed as retained
evidence. A separate `INDIGO_SIMULATOR_TRACE_DIR=/tmp/ioptron-reference-traces`
run of `test_mount_ioptron_simulator ioptron_detect_cem60` captures both firmware
branches against the unchanged production driver. No driver command-sequence
change is being accepted here; changed MountSim replies/state are intentional
protocol corrections described above.

### CEM120 late-reply observation

The first final nine-model matrix completed 71/72. During CEM120 manual movement,
`:qD#` was forwarded at CLOCK_MONOTONIC 262903.943868; the valid `1` arrived at
262909.535431, 5.592 s later, along with queued GEP/GLS replies. Earlier commands
in that same case also took 0.7–0.8 s. The driver's one-second deadline expired
correctly; it must not silently turn this failed stop into OK. No malformed stop
command or wrong reply was observed. The unchanged-code isolated manual recheck
passed, and the subsequent full CEM120 rerun passed 8/8. The failed attempt remains
recorded rather than being erased by the rerun.

Manufacturer 3.01 specifies the stop acknowledgement, and the independent INDI
V3 implementation also expects that reply (sources above). Apple's App Nap guide
https://developer.apple.com/library/archive/documentation/Performance/Conceptual/power_efficiency_guidelines_osx/AppNap.html
and process activity documentation
https://developer.apple.com/documentation/foundation/processinfo/activityoptions
explain host timer/I/O throttling as one possible source of delay. The trace alone
does not establish App Nap or distinguish host scheduling from a MountSim main-loop
stall. No timing threshold was relaxed and no speculative production fix was
applied for this single non-reproduced delay. Residual limitation: occasional
multi-second application/host latency remains unlocalized; do not claim it fixed.

### Final verification and retained evidence

- MountSim Debug `xcodebuild -project MountSim.xcodeproj -scheme MountSim -configuration Debug -derivedDataPath build CODE_SIGNING_ALLOWED=NO build`: passed.
- `python3 tests/run_native.py --derived-data build`: all native checks passed, including the stopped-tracking slew regression.
- `python3 tests/test_control.py --app build/Build/Products/Debug/MountSim.app`: passed all protocol audits and 20 interrupted parser/model replacement cycles.
- Production driver `make -C indigo_drivers/mount_ioptron -f ../../Makefile.drv all`: passed; production driver sources unchanged.
- `make -C indigo_test test-mount-ioptron-mountsim`: first full run 71/72; isolated CEM120 manual recheck 1/1, then `MOUNTSIM_IOPTRON_MODELS=CEM120` full rerun 8/8. Latest per-model results are all 8/8, with `-Wall -Wextra -Werror` test builds (existing common-header unused/sign warnings suppressed).
- From `indigo_test`: `build/integration/test_mount_ioptron_simulator`: 105/105; `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 build/integration/test_mount_ioptron_simulator_sanitize`: 105/105; `make test-mount-ioptron-tcp`: 3/3 ordinary and 3/3 sanitized. The sanitizer build instruments the driver and test, not the prebuilt framework library.
- Both CEM60 reference-capture cases passed. Baseline, intermediate failure/reproducer and final logs plus final per-model session metadata, raw traffic and command traces are preserved in `/tmp/ioptron-acceptance-20260923` before test cleanup. This is local temporary evidence, not a checked-in or permanent artifact store.

Guider signed completion error statistics below are milliseconds, except the last
two percentage columns. Each row retains 72 samples (648 total), with the endpoints,
20/100/500 ms requests, warmups and idle/tracking workloads defined above. Full
requested/actual values are in each archived `test.log`. Other software regression
processes ran concurrently; these statistics include host scheduling load. They
are observations of software completion latency, not physical pulse accuracy.

| Model | min / mean / median / p95 / p99 / max (ms) | Stddev (ms) | Max absolute (ms) | Mean signed % | Max absolute % |
| --- | --- | --- | --- | --- | --- |
| CEM25 | 50.636 / 66.735 / 60.239 / 152.791 / 198.343 / 198.343 | 26.531 | 198.343 | 142.213 | 991.715 |
| CEM40 | 50.450 / 62.955 / 56.407 / 114.069 / 185.601 / 185.601 | 25.034 | 185.601 | 144.281 | 928.005 |
| GEM45 | 50.352 / 62.311 / 56.159 / 69.881 / 254.239 / 254.239 | 29.247 | 254.239 | 139.531 | 1271.195 |
| CEM60 | 50.562 / 65.973 / 58.711 / 153.044 / 165.645 / 165.645 | 26.584 | 165.645 | 142.567 | 828.225 |
| SmartEQPro | 50.412 / 61.973 / 57.822 / 79.159 / 183.589 / 183.589 | 22.223 | 183.589 | 140.560 | 917.945 |
| SmartEQ | 50.356 / 81.224 / 58.608 / 347.977 / 380.744 / 380.744 | 76.927 | 380.744 | 189.120 | 1847.720 |
| ZEQ25 | 50.363 / 104.809 / 57.467 / 543.377 / 587.189 / 587.189 | 144.722 | 587.189 | 274.812 | 2935.945 |
| CEM70 | 50.494 / 61.774 / 57.500 / 102.040 / 178.546 / 178.546 | 22.026 | 178.546 | 139.751 | 892.730 |
| CEM120 | 50.182 / 62.891 / 56.303 / 110.847 / 211.115 / 211.115 | 28.502 | 211.115 | 143.625 | 1055.575 |

Repository completion: `make -C indigo_test test-clean` passed after preserving
captures. Rebased onto remote `84bb0910c` (including the other machine's LX200
and SynScan work); resolved the adjacent MIGRATION_STATUS rows and relocated
Xcode groups while preserving both sides' changes. Regenerated TEST_SUMMARY and
validated the merged Xcode project with `plutil -lint`. MountSim fixes are commits
`5b7336c` and `581c057`. No push was performed.

Final portable simulated acceptance: 108 run / 108 passed ordinary, 108 run / 108
passed with ASan/UBSan; 108 unique cases (105 serial + 3 TCP).
Final MountSim verification attempts: 81 run / 80 passed (first matrix 71/72,
isolated manual rerun 1/1, CEM120 full rerun 8/8). Latest per-model acceptance:
72 / 72 unique cases passed (8 per model, 9 models), counted
separately from portable integration. Physical hardware: 0 run / 0 passed.


## CEM120 latency investigation (2026-09-23 follow-up)

User requested localization of the previously unexplained stop timeout. The
unchanged app failed `ioptron_manual_motion` on the first Main Thread Checker
run. Temporary monotonic tracing was then added at receive, main-queue entry,
handler entry and reply write; five manual runs and triggered `sample` captures
locate the delay before main-queue entry. In one capture 629/776 main-thread
samples are inside Motor timer -> GUI sky update -> Stars catalogue projection,
while the serial reader waits in `blockingRunInMainThread`. Measured main-queue
waits reach 0.993 s, whereas handler/reply work is short. The diagnostic worker
entry logs main=1: the earlier global-queue suspicion is not demonstrated; the
synchronous dispatch can execute inline. App Nap is not needed to explain the
observed CPU-bound rendering backlog.

Atomic repair plan: reproduce redundant rendering in a native GUI regression;
coalesce expensive sky projection at the GUI boundary while preserving live
coordinates and the existing motor/transport timing; compare identically traced
CEM120 repetitions, then remove temporary diagnostics and rerun native/control
and full CEM120 acceptance. No driver timeout or protocol change is planned.

Public cross-checks:
- https://developer.apple.com/documentation/xcode/improving-app-responsiveness
- https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/Multithreading/RunLoopManagement/RunLoopManagement.html
These describe main-thread work budgets and delayed/coalesced timer servicing;
they support the captured stacks rather than attributing the failure to a mount
firmware peculiarity. Diagnostic artifacts: `/tmp/cem120-checker-baseline` and
`/tmp/cem120-diagnostic` (temporary local files).

Completed repair is in MountSim's shared GUI: coalesce expensive catalogue
projection to at most 30 Hz, with the interval starting after projection finishes.
Live pointing is updated before the gate, preserving GUI SYNC; motor integration,
serial parsing and driver deadlines are unchanged. A native GUI regression fails
on the original implementation and passes after the fix. Temporary probes were
removed before final validation. The earlier unlocalized timeout is superseded by
this investigation; the original 5.592 s event has no recoverable in-process stack,
but the unchanged app reproduced the same stop failure with a 1.314 s reply delay.

Five identically instrumented manual-motion runs per build measured main-queue
wait median/p95/max of 10.706/195.894/993.386 ms before and
0.024/13.325/27.010 ms after; maximum handler-through-reply time was
17.209 ms before and 0.897 ms after. There were 384/360 commands respectively
because slower cases trigger more polls. These are measured host software
latencies, not a real-time guarantee. Both five-run groups passed; the separate
unchanged-app checker baseline failed.

Final clean Debug build passed; MountSim's five native assertion groups and all
control-test model framing/identity checks plus 20 interrupted replacement cycles
passed. The original shared harness then passed the complete CEM120 suite 8/8.
No production INDIGO driver change was required (version remains 3.0.0.53); the
previously passing 108 portable cases were not rerun for this GUI-only fix.
The earlier nine-model acceptance matrix remains historical evidence; this
follow-up specifically reran CEM120. Captures, stack samples, comparison metrics,
and build/test logs are retained locally under
`/tmp/cem120-latency-investigation-20260923` before test cleanup.

Follow-up MountSim attempts: 19 run / 18 passed (unchanged baseline 0/1,
instrumented before 5/5, instrumented after 5/5, final acceptance 8/8).
Final CEM120 acceptance: 8 run / 8 passed; physical hardware: 0 run / 0 passed.
