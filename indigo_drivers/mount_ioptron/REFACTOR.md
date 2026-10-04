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
either by the persistent `X_PROTOCOL_VERSION` switch or by autodetection
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
- Manual `X_PROTOCOL_VERSION` selection of every dialect: `ioptron_forced_protocol_selection`.
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
3. **PROTOCOL_VERSION (now `X_PROTOCOL_VERSION`) only defined after connect** (fixed, user-directed). The
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
bundled PDFs; `indigo_uni_is_valid()` probes TCP with a zero-length `send()`
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
| Metadata, base properties, always-defined `X_PROTOCOL_VERSION` | `ioptron_driver_metadata_and_base_properties` |
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

## Guider timing case racing the previous GOTO (2026-09-26)

`ioptron_guider_directions_overlap_and_timing` failed intermittently at
`wait_event(simulator, "MS1", before)` in its GOTO workload (the 2026-09-26 14:52
detach-abort run, and earlier on 2026-09-24): the second direction's GOTO produced no
`:SRA`/`:Sd`/`:MS1#`, only status polls. The case requested each direction's GOTO without
waiting for the previous one, and `MOUNT_EQUATORIAL_COORDINATES` requests arriving while the
property is still BUSY are dropped by the framework's `INDIGO_COPY_TARGETS_PROCESS_CHANGE`
guard. A simulator slew at 100 deg/s lasts 1.4-1.6 s after `:MS1#` while one direction's
seven pulses take about 2 s, so the case passed only while the slew and its polled OK won
that race. Test defect, no driver change: the case now tracks the mount's coordinates state
in the timed client callback (its property cache follows the guider) and waits for the
preceding slew to leave BUSY before the next GOTO. With `--slew-rate 40` the old case fails
deterministically at the same assertion and the new one passes.

## UTC time request overwritten by the status poll (2026-09-26, 3.0.0.56)

One full run while verifying the fix above failed `ioptron_settings_translation_0205` at
`wait_event(simulator, d->utc_time, 0)`: a `UTC_TIME` request for 2026-09-15T23:30:45 with offset 2
reached the mount as `:SL154912#` / `:SC260926#` / `:SG+120#`, the host clock. The status poll
refreshes `UTC_TIME` from the mount clock on the device queue and guards the write with
`MOUNT_UTC_TIME_PROPERTY->state != INDIGO_BUSY_STATE`, but the request is copied on the bus thread.
A request copied between the poll's check and its write is overwritten with the mount clock, the
poll publishes OK, and the queued handler then sends the overwritten value. The settings cases
send the request right after connecting, while the first poll is between its status read and
that check, so they hit the few-instruction window more often than chance.

The requested time is now recorded in the mount context by `indigo_mount_set_utc_target()` from
`MOUNT_UTC_TIME.on_change_request`, parsed from the incoming request on the bus thread before it is
copied and only while the property is not BUSY. The handler sends `indigo_mount_get_utc_target()`
to the mount and writes it back into the items once the mount accepted it. The poll keeps its
BUSY guard. No mutex is involved.

Proof with an instrumented copy of the driver (400 ms sleep between the poll's check and its
write, request sent 150 ms after a `:GLS#` poll): the 3.0.0.55 driver sends the host time and fails
the 0205 and 0300 settings cases at the `:SL` / `:SUT` wait; with 3.0.0.56 the mount receives the
requested time. Residual: in that window the poll still publishes OK with the mount clock before
the handler runs, so a client can see an early OK with the old time until the handler publishes the
requested one; the instrumented run fails the case's immediate cached-value check for that reason.
The production window is a few instructions, and no permanent case can hit it deterministically.

## Linux simulator run (2026-09-26, 3.0.0.58)

The first Linux x64 run of the suite failed two cases deterministically; both failed identically
with 3.0.0.56, and macOS runs did not show them.

- `ioptron_configured_baudrate_requires_product_reply`, driver defect: `:MountInfo#` was read with
  `indigo_uni_read_section()`, which has no inter-byte timeout, so the injected two-byte reply `01`
  waited for the port's termios `VTIME` of 5 s before failing. The case's 5 s connection wait expired
  first and saw BUSY instead of ALERT; on real hardware every short or foreign reply to the product
  probe cost 5 s. The reply is now read with `indigo_uni_read_section2()` and a 100 ms inter-byte
  timeout, the same one the `#` terminated replies use.
- `ioptron_guider_transport_failure_and_recovery`, test defect: after the simulator is killed, a Linux
  pseudo-terminal answers `TIOCMGET` with EIO like a removed USB serial adapter, so
  `indigo_uni_is_valid()` reports the lost port. The pulse is still answered with ALERT, but the driver
  then closes the port and disconnects the guider, deleting `GUIDER_GUIDE_*` before the property cache
  could show the ALERT. A macOS pseudo-terminal keeps answering ENOTTY and the guider stays connected.
  The case now sees the ALERT through the client callback and accepts either outcome: a disconnected
  guider without guide properties, or (macOS) further pulses failing with ALERT without a stale pulse.
  Reconnect and recovery are checked in both cases. The Linux branch is verified here; the macOS branch
  is the previous assertion sequence and still needs a macOS run.

The simulator also had to clear `current_command` after each command to build with gcc
(`-Werror=dangling-pointer`).

## Switch target prototype on MOUNT_TRACKING (2026-09-26, 3.0.0.59, branch `refactoring_targets`)

`MOUNT_TRACKING` has the same window as `UTC_TIME` above: the status poll checks
`MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE` and then writes the switch from the mount
status, while the request is copied on the bus thread. A request copied between the check and the
write is overwritten, and the handler sends the mount's current state instead of the request.
Prototype: switch items carry an internal `sw.target`, written by `indigo_property_copy_values()`
together with the value and never by `indigo_set_switch()` or sent over the protocol. The
`MOUNT_TRACKING` handler reads the request with `indigo_get_switch_target()`, sends it, and applies it to
the values with `indigo_apply_switch_targets()` once the mount accepted it. On failure it sets the switch
to the last polled mount state before publishing ALERT, so a rejected request is not left displayed
(`ioptron_option_command_failures_recover` checks it; without that write the case fails). The poll is
unchanged. Unit coverage is in `indigo_test/unit/test_bus_property.c`, the driver-side rules in
`indigo_docs/DRIVER_DEVELOPMENT_BASICS.md` ("Switch Item Target"), including that only BUSY guarded
requests may rely on the target, not the `_ANYTIME` variants.

Side question from the design note: `indigo_property_copy_targets()` does not copy `access_token` while
`indigo_property_copy_values()` does. It has no functional effect: the lock check in `indigo_bus.c`
compares the token of the incoming request before the driver sees it, and XML/JSON send a token only
with a client's outgoing change request, so the token stored in a driver's property is only shown in
bus traces. Left unchanged.

Proof with an instrumented copy of the driver (not committed): `indigo_usleep(900000)` right after the
poll's BUSY check, and in `check_tracking_rates()` the `MOUNT_TRACKING` OFF request sent 150 ms after a
`:GLS#` poll. With the handler reading `sw.value` the mount receives `:ST1#` instead of `:ST0#` and
`ioptron_tracking_rates_0205` / `_0300` fail at `wait_event(simulator, "ST0", 0)` in 6/6 runs; with
`sw.target` both pass in 6/6 runs (Linux x64), again 6/6 with the final handler using the helpers. Without the alignment the 900 ms sleep alone never hit
the window, because the case sends each request right after the previous handler's OK, before the next
poll reaches its check.

## Transport loss disconnects the mount too (2026-09-26, 3.0.0.60)

Since the generator makes the master device its own `master_device` (8531b6c on `refactoring`),
`indigo_disconnect_slave_devices()` called by `ioptron_validate_handle()` on a lost port disconnects
the mount as well as the guider, as hand-written drivers always did. On Linux a pseudo-terminal
reports the hangup of the killed simulator (EIO), so `ioptron_transport_loss_and_fresh_session` no
longer saw the ABORT's ALERT in the property cache: the mount was disconnected and its properties
deleted right after it (105/104 on Linux, macOS unaffected because its pseudo-terminal keeps answering
ENOTTY). Driver behaviour is correct; the case now sees the ALERT through the client callback, like
`ioptron_guider_transport_failure_and_recovery`, and accepts either a mount still connected or one the
driver disconnected, before reconnecting to a fresh simulator.

## Park, home and guide pulse targets (2026-09-27, 3.0.0.61, branch `refactoring_targets`)

Findings TGT-026, TGT-027 and the mount_ioptron part of TGT-B04 of
`indigo_drivers/REVIEW_SWITCH_TARGETS.md`, and the open question whether the `MOUNT_TRACKING` window of the
3.0.0.59 prototype can get a permanent test. No hardware run for this change. Baseline before the change:
the unchanged suite (3.0.0.60) passed 105/105 on Linux x64 in the recorded run of 2026-09-26 22:54.

- TGT-026 `MOUNT_PARK`, TGT-027 `MOUNT_HOME`: `ioptron_update_mount_state()` checks the property for BUSY
  and, when it is not, mirrors the park and home state the mount reports into the switch. A request copied
  on the bus thread between that check and the write was overwritten, and the queued handler read the
  overwritten value: a PARK request on an unparked mount became UNPARKED and the handler sent `:MP0#`
  (unpark) instead of `:MP1#`, an UNPARK on a parked mount would have parked it, and a HOME request
  became AWAY, so the handler reported OK without sending `:MH#`.
- TGT-B04: `GUIDER_GUIDE_RA/DEC` accept a pulse while one runs. On HC8406 the finalizer stops the pulse with
  `:Qn#` / `:Qe#` and then zeroes the values, so a pulse copied during that command was read as 0 by its
  handler and reported OK without being sent. On the other protocols the finalizer has no I/O before the
  zeroing, the same window is a few instructions wide.

Fix: the `MOUNT_PARK` and `MOUNT_HOME` handlers apply the targets (`indigo_apply_switch_targets()`) before
they decide, so they act on the request; a refused or failed park, unpark, home or home search shows the
park or home state the mount last reported with ALERT instead of the rejected request (on the one item
HC8406 park property the item is cleared as before). In `MOUNT_HOME` the call follows the BUSY assignment,
so the generated handler keeps its prologue unchanged. The guide handlers restore the values from the
number targets, which the finalizers (now commented) do not touch. The poll keeps its BUSY checks.

Regression tests in `integration/test_mount_ioptron_simulator.c` (105 -> 106 cases):

- `ioptron_guider_pulse_survives_previous_finalizer` (new): HC8406 guider, a 100 ms north (east) pulse;
  the 300 ms south (west) pulse is requested from the debug log line of the finalizer's `:Qn#` (`:Qe#`),
  on the finalizer's own thread, which models the concurrent bus thread deterministically. Against
  3.0.0.60 it failed 4/4 for DEC and 3/3 for RA (run first with a temporary switch): no `:Ms#` / `:Mw#`
  was sent. With 3.0.0.61 it passed 3/3 before the recorded run, both pulses ran 350 ms (300 ms plus the
  50 ms settle delay of `ioptron_no_reply_command()` before the finalizer is scheduled).
- `ioptron_option_command_failures_recover` (extended): a refused `:MP1#` leaves UNPARKED shown with
  ALERT, a refused `:MH#` leaves AWAY shown with ALERT.

The park and home windows, and the `MOUNT_TRACKING` window of the 3.0.0.59 prototype, cannot be hit by a
permanent case. In `ioptron_update_mount_state()` all I/O (`ioptron_get_state()`: `:GLS#`, `:GAS#` or the
HC8407 `:AP#`/`:AH#`/`:AT#`/`:SE?#` round trips, each followed by the 50 ms settle delay) happens before
the three checks; between each check (`MOUNT_TRACKING_PROPERTY->state != INDIGO_BUSY_STATE`,
`MOUNT_PARK_PROPERTY->state == INDIGO_BUSY_STATE`, `MOUNT_HOME_PROPERTY->state == INDIGO_BUSY_STATE`) and
its `indigo_set_switch()` there are only comparisons of `PRIVATE_DATA` flags and switch values, no I/O,
no log line and no lock. A delayed simulator reply only delays the poll before the checks, and a request
sent from the log of the status reply is copied before the check, which then sees BUSY and leaves the
property alone; a queue gate cannot help either, because the poll runs on the same queue as the handler.
So the windows were reproduced with a temporary copy of the generated driver (not committed) that logs
`TGT window tracking|park|home` right after each check, and temporary cases on protocol 3.0 sending the
request from that line:

- 3.0.0.60: PARK sent `:MP0#` and no `:MP1#` (4/4), HOME sent no `:MH#` (4/4); `MOUNT_TRACKING`
  with the handler temporarily reading `sw.value` again, as before 3.0.0.59: OFF sent `:ST1#`, no `:ST0#`
  (3/3).
- 3.0.0.61: `:MP1#`, `:MH#` and, with the unchanged 3.0.0.59 handler, `:ST0#` were sent (3/3 each).

This confirms the prototype's instrumented proof above with a log line instead of a sleep; the
`MOUNT_TRACKING` window stays covered only by that evidence, not by a permanent case.

MIGRATION_STATUS.md hardware-free count 108 -> 109 (105 + 1 serial cases and the 3 opt-in TCP cases).

Recorded run `TZ=Europe/Bratislava python3 tools/run_driver_test.py mount_ioptron` on Linux x64:
106/106 OK (2026-09-27 02:42).

### Final test summary for this change

Simulated tests: **106 run, 106 passed** (recorded run). Hardware tests: **0 run, 0 passed**.

## Current product codes, alt-azimuth mode and `:Z` guide pulses (2026-10-03, 3.0.0.62)

### Baseline

Driver version 61 (`DRIVER_VERSION 0x0300003D`), last recorded run `python3 tools/run_driver_test.py
mount_ioptron` on macOS arm64: 106/106 OK (2026-09-27 22:38). No iOptron hardware is available, so no
hardware testing is planned or claimed for this change.

### Protocol facts used

Mounts with main-board firmware 210101 or later answer `:MountInfo#` with these product codes:

| Code | Model | Code | Model | Code | Model |
| ---: | --- | ---: | --- | ---: | --- |
| 10 | SkyHunter (EQ mode) | 31 | HAE29 (EQ mode) | 52 | HAZ46 |
| 11 | SkyHunter (AA mode) | 32 | HAE29-EC (EQ mode) | 53 | HAE43 B/C (EQ mode) |
| 12 | HAE16 (EQ mode) | 33 | HAE29 (AA mode) | 54 | HAE43 B/C-EC (EQ mode) |
| 13 | HAE16 (AA mode) | 34 | HAE29-EC (AA mode) | 55 | HAE43 B/C (AA mode) |
| 14 | HAE18 (EQ mode) | 35 | HAZ31 | 56 | HAE43 B/C-EC (AA mode) |
| 15 | HEM15 | 36 | HAE29 B/C (EQ mode) | 57 | HAE44 B/C (EQ mode) |
| 16 | HEM15-EC | 37 | HAE29 B/C-EC (EQ mode) | 58 | HAE44 B/C-EC (EQ mode) |
| 17 | HAE27 B/C (EQ mode) | 38 | HAE29 B/C (AA mode) | 59 | HAE44 B/C (AA mode) |
| 18 | HAE27 B/C-EC (EQ mode) | 39 | HAE29 B/C-EC (AA mode) | 60 | HAE44 B/C-EC (AA mode) |
| 19 | HAE27 B/C (AA mode) | 40 | CEM40(G) | 62-69 | HAE69 variants, same order as 31-34 then 36-39 |
| 20 | HAE27 B/C-EC (AA mode) | 41 | CEM40(G)-EC | 70 | CEM70(G) |
| 22 | HAE18 (AA mode) | 42 | HEM44 | 71 | CEM70(G)-EC |
| 25 | HEM27 | 43 | GEM45(G) | 72 | CEM70(G)-EC2 |
| 26 | CEM26 | 44 | GEM45(G)-EC | 73 | HAZ71 |
| 27 | CEM26-EC | 45 | HEM44-EC | 120 | CEM120 |
| 28 | GEM28 | 46 | HEM44A | 121 | CEM120-EC |
| 29 | GEM28-EC | 47 | HEM44A-EC | 122 | CEM120-EC2 |
| 30 | HEM27-EC | 48-51 | HAE43 variants, same order as 31-34 | 123 | HAZ130 |

- Alt-azimuth (AA mode and HAZ) mounts report RA/Dec, but side of pier and pointing state in `:GEP#` are
  meaningless, and `:AG#`, `:RG#`, `:SMT#`, `:GMT#`, `:MS2#`, `:QAP#` and timed guide pulses are not
  available. In AA mode an `-EC` mount does not use its encoder.
- Periodic error correction (`:GPE#`, `:GPR#`, `:SPP#`, `:SPR#`) exists only on worm-driven equatorial
  mounts without encoders: HEM27, CEM26, GEM28, CEM40, GEM45, CEM70 and CEM120. Strain-wave mounts
  (HEM15, HEM44, HAE, SkyHunter) have none.
- `:MSH#` (search mechanical zero) exists on every current model except SkyHunter, CEM26, GEM28, HAZ31,
  HAZ46, HAZ71 and the HAE44 B/C variants.
- Main-board firmware 210101 and later takes timed guide pulses as `:ZSnnnnn#` (east, RA+),
  `:ZQnnnnn#` (west, RA-), `:ZEnnnnn#` (north, Dec+) and `:ZCnnnnn#` (south, Dec-); `:Mn/Ms/Me/Mw` are
  deprecated there.
- The codes 10, 11, 25, 26, 30, 45, 46 and 60 were used by older products (Cube II, SmartEQ Pro+, CEM25,
  CEM25-EC, iEQ30 Pro, iEQ45 Pro and its AA variant, CEM60) with firmware older than 210101, so the firmware
  date decides between the two meanings.

### Audit of version 61

1. The code 26 entry `{ 26, NULL, "CEM25EC", V2_5, ... }` precedes `{ 26, NULL, "CEM26", V3_0, ... }`, so a
   CEM26 is detected as a CEM25-EC with protocol 2.5 (source audit).
2. `CEM26` is marked `has_encoders = true`, so its PEC controls are hidden (source audit).
3. Codes 12, 33, 34, 38, 50 and 51 carry wrong names (`HEM26`, `HAE29`, `HAE29AA`, `HAE29C`, `HAE43`,
   `HAE43AA`), the HEM27, HEM44, HAE16/18/27/69, SkyHunter, CEM70-EC2, HAZ71 and HAZ130 codes are
   missing, and a HEM27-EC or HEM44-EC/HEM44A is detected as an iEQ30 Pro or iEQ45 Pro (source audit).
4. PEC is offered on every V3 mount without encoders, including strain-wave mounts that have none.
5. Alt-azimuth mounts get side of pier, meridian handling and guide rate controls and the guider sends
   timed pulses they do not accept.
6. The driver sends the deprecated `:Mn/Ms/Me/Mw` pulses to current firmware.

### Atomic plan

1. Simulator: per-product capabilities for the current codes (alt-azimuth, encoders, PEC, zero search),
   reject commands the product does not have, report pier side 2 for alt-azimuth, and accept `:Z`
   pulses on protocol 3.0 with firmware 210101 or later.
2. Driver (`.driver`, version 62): product table with `has_pec` and `altaz`, firmware gating of the
   shared codes, PEC/home/pier/meridian/guide-rate visibility from the table, `:Z` pulses for current
   firmware; regenerate.
3. Tests: detection rows for the shared codes, CEM26, strain-wave PEC, alt-azimuth mounts and
   the `:Z` pulses; update the protocol 3.0 profile and guider cases to the `:Z` pulses.
4. Build, recorded run with `tools/run_driver_test.py`, commit.

### Results

1. Simulator: done. `CURRENT_PRODUCTS` capabilities apply on protocol 3.0 with firmware 210101 or later;
   alt-azimuth products reject `:AG#`, `:RG#`, `:GMT#`, `:SMT#` and timed pulses and report pier side 2,
   products without PEC reject `:GPE#`, `:GPR#`, `:SPP#`, `:SPR#`, products without zero search reject
   `:MSH#`; `:ZS/:ZQ/:ZE/:ZC` move the axes at the guide rate like the `:M` pulses.
2. Driver version 62 (`DRIVER_VERSION 0x0300003E`): done, regenerated, builds without warnings.
3. Tests (106 -> 115 cases): new detection rows `ioptron_detect_hem44ec_current_firmware_shares_ieq45pro_code`,
   `ioptron_detect_ieq45pro_30_older_firmware_keeps_code`, `ioptron_detect_cem26_has_pec_without_home_search`,
   `ioptron_detect_cem25ec_older_firmware_shares_cem26_code`, `ioptron_detect_strain_wave_hem44_without_pec`,
   `ioptron_detect_hae29_aa_mode_is_altaz`, `ioptron_detect_haz31_altaz_without_home_search`; new guider
   cases `ioptron_guider_older_firmware_uses_m_pulses` and `ioptron_guider_altaz_mount_rejects_pulses`;
   the protocol 3.0 profile, pulse mechanics, timing, overlap, disconnect and shared-connection cases
   now expect `:Z` pulses (default simulator firmware 210605).
4. First recorded run (2026-10-03 21:58): 115/93 Failed. Code 60 is also used by the CEM60, and the new
   `{ 60, NULL, "HAE44BCECAA", ... }` entry preceded the CEM60 entries, so every CEM60 (protocol 1.0, 2.0,
   2.5 and 3.0 profiles and detection rows) was detected as an alt-azimuth HAE44. Fixed by gating code 60
   with firmware 210101 like the other shared codes. Second recorded run (2026-10-03 22:08): 115/115 OK.

Found defects fixed by this change: audit items 1-6 above (source audit, each now covered by the
detection and guider cases listed in step 3), plus the code 60 collision introduced and caught within
this change (reproduced by the first recorded run).

Guider pulse timing: `ioptron_guider_directions_overlap_and_timing` now measures `:Z` pulses; it passed
within its existing bounds (simulator software timing, not hardware).

MIGRATION_STATUS.md hardware-free count 109 -> 118 (115 serial cases and the 3 opt-in TCP cases).

### Final test summary for this change

Simulated tests: **115 run, 115 passed** (second recorded run; the first recorded run had 22 failures,
fixed as described in step 4). Hardware tests: **0 run, 0 passed**.

## Mount test standard coverage (2026-10-04, 3.0.0.63)

The suite was checked against the "Mount Drivers" chapter and the "Mount Driver Test Standard" of
`indigo_test/DRIVER_TESTING_RULES.md` (and the guider standard for pulse guiding). Simulator additions:
`--meridian <fll>` (initial `:GMT#` state) and the control line `@halt` (the controller ends a running slew
where it is, as on a limit stop or a hand controller stop).

Defects found by the new assertions and fixed in version 63 (`indigo_mount_ioptron.driver`, regenerated):

1. RA just below the wrap point was rounded to 24 h (`Sr 24:00:00`, `Sr86400000`, `SRA129600000`) and refused
   by the mount; `ioptron_device_ra()` rounds to the device unit and wraps to 0 (SYNC, GOTO, V1/V2 park).
2. An aborted GOTO ended `MOUNT_EQUATORIAL_COORDINATES` OK; it now ends ALERT ("Slew aborted") and stays ALERT
   while the mount decelerates; an abort while idle no longer touches the coordinate state.
3. A GOTO the controller ended short of the target ended OK; completion now compares the readback with the
   target (`RA_MIN_DIF`/`DEC_MIN_DIF`, judged on a readback taken after the status reported the end) and ends
   ALERT with "Slew ended short of the target". The SLEW light returns to IDLE when the slew ends (it was
   ALERT after a failed GOTO).
4. A refused slew sent no reason; the controller's reason (`0`, HC8406 `1`/`2`) now reaches the client.
5. Disconnect during a GOTO, park, homing or arrow motion left the mount moving; it now sends `:Q#` first.
   HC8406 guider pulses (arrow motion stopped by the finalizer) are stopped with `:Qn#`/`:Qe#` when the guider
   disconnects during a pulse.
6. A park or homing the mount acknowledged but never started (or stopped short) stayed BUSY forever; it ends
   ALERT after 5 s without motion (or when the motion stops) and shows unparked / AWAY.
7. `MOUNT_PARK_SET.CURRENT` stayed ON after a successful set (momentary action).
8. The meridian treatment was not read at connect (driver defaults published); `:GMT#` is read on protocol
   3.0, and a refused meridian change shows the controller's setting.
9. A refused tracking rate left the rejected rate displayed; the rate the mount uses is read back.
10. `:RR` of a custom rate truncated (1.9 sent as `RR18999`); now rounded.
11. A guide rate changed through the mount was not reflected on the guider and vice versa.
12. The status poll republished `MOUNT_STATE`, `MOUNT_TRACKING`, `MOUNT_PARK`, `MOUNT_HOME` and
    `MOUNT_SIDE_OF_PIER` every second; they are now published only when they change.

| Rule | Test |
| --- | --- |
| Connect handshake order, no controller writes at connect | `ioptron_connect_handshake_order_0300`, `_8407` |
| No update of an undefined property across connect/disconnect/reconnect | `ioptron_profile_*` |
| RA rounding carry at the wrap point | `ioptron_coordinate_boundaries_8406/8407/0100/0205/0300` |
| Aborted GOTO ALERT, never OK, stop sent once | `ioptron_goto_progress_abort_and_restart` |
| Abort while idle keeps position, tracking and coordinate state | `ioptron_idle_abort_keeps_position_and_tracking` |
| Frozen status keeps GOTO BUSY, slew ended short ALERT, next GOTO accepted | `ioptron_goto_completion_follows_slew_status` |
| Refusal reason reaches the client | `ioptron_goto_below_altitude_limit_rejected`, `ioptron_goto_refusal_reason_8406` |
| Disconnect during GOTO/park stops the mount, clean next session, interrupted park not failed | `ioptron_disconnect_during_motion_stops_mount` |
| Park/home never started ends ALERT | `ioptron_park_and_home_never_started_alert` |
| Park light and coordinates BUSY while parking, refused parked requests keep switches, unpark sends no motion and stays unparked | `ioptron_park_workflow_*` |
| Homing keeps park state | `ioptron_home_workflow_*` |
| Momentary PARK_SET | `ioptron_park_set_positions_0205/0300` |
| Prime meridian / zero-degree signs, one-item site change resends the other, site kept across reconnect | `ioptron_site_signs_8407/0200/0300` |
| Both ends of scaled guide and custom rate ranges on the wire | `ioptron_scaled_range_ends_0300` |
| Meridian treatment read at connect, refused change shows device state | `ioptron_meridian_treatment_read_at_connect` |
| Refused tracking rate shows device rate | `ioptron_track_rate_refusal_shows_device_rate_8407/0200/0300` |
| External change published once | `ioptron_external_changes_published_once` |
| RA pulse displacement, tracking kept on/off | `ioptron_guider_zero_requests_and_pulse_mechanics` |
| Guider disconnect during HC8406 pulse | `ioptron_guider_disconnect_stops_hc8406_pulse` |
| Guide rate shared by mount and guider | `ioptron_guide_rate_shared_by_mount_and_guider` |
| Manual motion released when its client detaches | `ioptron_motion_released_when_client_detaches` |
| SHUTDOWN refused while connected | `ioptron_shutdown_refused_while_connected` |

Recorded run 2026-10-04 18:52 (mac arm64, simulator): 136/136 OK. Test cases 115 -> 136 (plus 3 opt-in TCP).

Not covered, with reason: direction/sign per pier side and hemisphere (the mount maps `:mn`/`:ms` itself, the
driver has no mapping); PEC refusal showing the device state (no PEC state readback in the protocol); a
setting acknowledged but not kept (the simulated controller always keeps it, no readback after write in the
protocol flow); requests versus poll for tracking/park/home (documented above with instrumented evidence, the
window has no I/O); alignment, time-zone order (protocol does not mandate one) and device options the driver
does not implement.

## Custom property names (3.0.0.64)

The driver-specific properties now carry the required `X_` prefix (user-approved client-visible rename, no
backward-compatible alias): `PROTOCOL_VERSION` -> `X_PROTOCOL_VERSION`, `MOUNT_MERIDIAN_HANDLING` ->
`X_MOUNT_MERIDIAN_HANDLING`, `MOUNT_MERIDIAN_LIMIT` -> `X_MOUNT_MERIDIAN_LIMIT`. Items are unchanged. A saved
protocol selection stored under the old name is not loaded; select the dialect again and save the
configuration. The integration (`ioptron_driver_metadata_and_base_properties` and the per-dialect profiles)
and MountSim (`ioptron_park_home_options`) tests assert that the unprefixed names are never defined.
