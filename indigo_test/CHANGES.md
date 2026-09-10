# INDIGO Test Suite Changes

## Guider Agent RAW validation fix — DRV-124 (2026-09-10)

The driver rejects incomplete RAW headers before reading their fields, unsupported signatures, zero/oversized dimensions and incomplete pixel payloads before Bayer processing or image analysis. Division checks precede multiplication; pixel offsets and total BLOB size must fit the signed-int limits used by image-analysis and Bayer-metadata helpers. Valid trailing metadata remains supported. No property definitions changed. Per user instruction, the agent stays at version `0x0300002C` until DRV-133 is fixed; DRV-124–DRV-133 will be recorded as one combined change.

The suite now contains 66 cases. `truncated raw` checks 1-byte, 4-byte and header-minus-one lengths plus recovery. New `raw dimensions and recovery` covers zero width/height, `UINT32_MAX` width and a 65536 × 65536 product. New `raw payload formats and recovery` checks a payload one byte short for MONO8/MONO16/RGB24/RGB48, then successful guiding with each complete format; complete mono fixtures include a Bayer trailer. `invalid raw` retains the unsupported-signature regression.

Validation: the driver builds with `Makefile.drv`; all four RAW-filtered integration cases pass normally and with AddressSanitizer (`TEST_BUILD=build/drv124-asan`, `GUIDER_TEST_FLAGS='-fsanitize=address -fno-omit-frame-pointer -O1'`). These checks include clean teardown and recovery. The entire agent suite was not rerun for this focused fix; the earlier 64-case results below are historical and DRV-125–DRV-133 remain open. Test build artifacts were removed with `make -C indigo_test test-clean`.

## Guider Agent integration coverage (2026-09-10)

`integration/test_agent_guider.c` adds 64 independently isolated cases, registered in the normal integration suite and available through `make -C indigo_test test-agent-guider`. To run a subset after building, pass a case-name substring to `indigo_test/build/integration/test_agent_guider`, for example `'calibration'` or `'live dec'`. A missing match returns 2. Failed assertions, child signals/timeouts and teardown failures return nonzero; known production failures are not skipped or accepted as passes.

The test links the production Guider Agent, filter and simulator, uses public bus changes and keeps actual agent timers, image analysis and correction algorithms. The CCD simulator supplies normal preview images. An optional deterministic camera boundary supplies four Gaussian stars (or a bright extended target for full-frame centroid), while a guider spy observes pulse direction/duration and translates pulses to image displacement at 10 px/s. This is an agent integration boundary, not hardware or simulator-driver compliance. Exposure/guide failures are injected only at that boundary. Each child starts INDIGO after `fork()`, writes configuration/logs to its own `mkdtemp()` directory and disconnects/shuts down on completion; the parent removes its files even after a signalled child. Timer failures are bounded, and fresh property revisions are required for asynchronous changes. Reselecting an already selected device is a deliberate synchronous no-op in the filter and is tested without expecting a new event.

### Scenario-to-test mapping

| Agent-owned scenario | Registered cases / assertions |
| --- | --- |
| Metadata, public properties, idempotent INIT, missing camera/guider and invalid operational preconditions | `metadata`, `missing devices`, `guiding preconditions` |
| One-shot preview, RAW override/FITS restoration, exposure retries and terminal states | `single preview status and format restoration`, `single preview failure`, `single preview retry` |
| Continuous preview, abort during exposure, restart, camera disconnection/reselection | `continuous preview abort status`, `preview abort and restart`, `camera disconnect`, `camera selection reset` |
| Guide-star discovery, manual selection, dynamic multi-star item count, clear, include/exclude regions, bin-change reset and subframe restoration | `stars selection clear and resize`, `stars abort and failure`, `selection regions binning`, `selection subframe restore`, `multistar weighted`, `donuts include region` |
| All four detection modes and all applicable correction modes, finite statistics and actual correction of a known displacement | `selection PI guiding`, `weighted hysteresis guiding`, `donuts linear trend guiding`, `centroid resist switch guiding`, `selection PPEC guiding`; each checks emitted commands and residual displacement, rather than only successful frame acquisition |
| Pulse direction, seconds-to-ms conversion, maximum pulse, minimum-error/minimum-pulse suppression, integral history and DEC reversal/backlash | `correction response`, `pulse thresholds`, `pi integral history`, `backlash reversal`, `dec modes` |
| Live settings restrictions, blocked process replacement, blocked detection/correction/count/edge-clipping/log changes, supported one-direction DEC changes | `busy guards and deferred PPEC reset`, `live dec mode guards` |
| Mount declination compensation, meridian flip and optional DEC reversal | `declination scaling`, `meridian flip` compare pulse magnitudes/signs before and after coordinate changes |
| Calibration axes, measured speeds, adaptive step, missing motion, lost stars, recovery, abort and automatic handoff to guiding | `calibration`, `calibration ra only`, `calibration speed accuracy`, `calibration adaptive step`, `calibration no motion`, `calibration star failure`, `calibration reset recovery`, `calibration abort`, `calibration and guiding` |
| Loss-of-star policy exclusivity, FAIL/CONTINUE/RESET behavior and recovery | `feature policies`, `star loss fail`, `star loss continue`, `star loss reset` |
| Exposure failures during guiding, invalid image format/short image, pulse failure and timely abort during inter-frame delay | `exposure failure guiding`, `invalid raw`, `truncated raw`, `pulse error`, `guiding delay abort` |
| Random/randomized-spiral/spiral dithering and bounds, exact spiral progression/reset, RA-only projection, abort and settle timeout | `dither strategies`, `spiral sequence reset`, `dither RA projection preserves magnitude`, `dither abort`, `dither timeout` |
| PPEC selection, learning progress, reset while running/idle and restart | `selection PPEC guiding`, `ppec learning reset`, `busy guards and deferred PPEC reset` |
| Zero drift after a nonzero measurement | `zero drift statistics` checks drift and correction clear |
| Log creation/header output, algorithm-specific logging paths and inability to create a log followed by successful logging | `logging`, `logging algorithms`, `logging open failure recovery`; logs are written only inside the test's temporary directory |
| Saved settings/selection shape/correction mode reload, factory defaults, additional-instance lifecycle and independent acquisition | `configuration reload`, `reset defaults`, `additional instances lifecycle`, `simultaneous agents` |
| Related-agent filtering and enabling/disabling mount-triggered guider abort, shutdown/reinitialization during guiding | `related mount coordination`, `shutdown active` |

### Validation and remaining acceptance work

Validation uses macOS arm64, production source at `ba9259e22` and the existing built INDIGO/simulator libraries. The target also compiles the agent/filter/configuration boundary directly; the only framework replacement redirects the configuration directory. Production source and versions are unchanged by this test completion.

The final `make -C indigo_test test-agent-guider` run completed **53/64 cases successfully**, including cleanup. Eleven regressions failed for the existing production defects listed below; all four added cases and the strengthened correction checks passed. The target correctly returns nonzero. The normal `shutdown active` case passed, but its sanitizer run exposes the separate lifetime defect; an ordinary pass is not acceptance of that path.

| Open production finding already recorded in `../indigo_drivers/REVIEW.md` | Regression / observed failure |
| --- | --- |
| DRV-124 (subsequently fixed) | Original `truncated raw` failure is resolved by the RAW validation fix; see the focused RAW validation results above. |
| DRV-125 | `shutdown active`: normal execution can pass, but AddressSanitizer reports use-after-free in `indigo_restore_switch_state()` after filter-client teardown |
| DRV-126 | Both single-preview status cases publish IDLE rather than OK/ALERT |
| DRV-127 | Continuous-preview abort and camera-disconnection cases publish incorrect terminal status |
| DRV-128 | Adaptive calibration fails instead of reducing its step |
| DRV-129 | Calibration reports 12.5 px/s for a 10 px/s boundary |
| DRV-130 | RA-only dither fails the requested magnitude assertion |
| DRV-131 | Guiding delay prevents completion within the 1.5-second abort bound |
| DRV-132 | A guide-pulse ALERT does not terminate the guiding process |
| DRV-133 | Drift/correction statistics remain stale when measured drift returns to exactly zero |

Sanitizer reproduction (uses a separate build directory to prevent mixing instrumented and ordinary objects):

```sh
make -C indigo_test TEST_BUILD=build/guider-asan GUIDER_TEST_FLAGS='-fsanitize=address -fno-omit-frame-pointer -O1' build/guider-asan/integration/test_agent_guider
indigo_test/build/guider-asan/integration/test_agent_guider 'truncated raw'
indigo_test/build/guider-asan/integration/test_agent_guider 'shutdown active'
```

Both sanitizer regressions were reproduced in this run. The final instrumented build also passed `live dec mode guards`, `centroid resist switch guiding` and `spiral sequence reset` (3/3). Instrumentation covers the test, agent, filter and configuration layer, not the prebuilt simulator/library archives; this is not a whole-program memory-safety claim.

The scenario inventory is now explicit, but driver acceptance remains **Partial** until the failing regressions are fixed. Additional limitations: no network/BLOB-download transport tests (the normal suite opens no sockets), physical mount/camera timing, long-period PPEC prediction/period convergence, prolonged pulse-BUSY timeout, forced allocation/attach failure, or Linux/Windows execution. Cross-agent tests verify the guider's related-agent contract with local peers; they do not claim complete real Mount/Imager Agent workflows. Serial-protocol commands, USB hot-plug, guide-port resource ownership and manufacturer motion models belong to the underlying drivers and are not applicable to this meta driver. Shared image/math/configuration algorithms retain their separate framework test scope.


## Framework input validation (2026-09-09)

Removed generic finite/fractional/range guards and matching invalid-request cases added to wheel drivers, plus finalizer cancellation made redundant by their BUSY guards. `indigo_property_copy_values()` and `indigo_property_copy_targets()` own numeric input handling. New unit cases cover NaN/both infinities preserving accepted values/targets, valid sibling items, and unchanged finite min/max clamping. Driver tests retain device/SDK reply validation and operational conflicts.

## Driver test coverage status (2026-09-09)

One current inventory for **all driver modules** in `indigo_drivers/`, `indigo_linux_drivers/`, `indigo_mac_drivers/` and `indigo_optional_drivers/`, including agents, CCD/OEM variants, generated and hand-written drivers, and simulator drivers. Each source module has one row, sorted by driver ID; its logical devices belong to that row. Host-side protocol simulators are test boundaries, not additional driver modules. Platform-specific locations are noted in the final column.

The inventory merges the earlier CCD and non-CCD audits and incorporates their recorded follow-up results. Coverage refers to applicable driver-owned scenarios in `DRIVER_TESTING_RULES.md`, not line/branch percentages, framework codecs or hardware acceptance. This documentation update inspected the source inventory and test targets; it did not run tests or complete the pending behavior audits. Passing test groups establish only the scenarios described, not full coverage.

- **Complete**: applicable driver-owned coverage was explicitly recorded as complete, with passing validation.
- **Partial**: concrete scenarios and their validation results are recorded, but failures, remaining gaps or completion of the full standard audit are still outstanding.
- **Not audited**: an automated test exists, but its coverage has not been assessed against the complete applicable standard in this audit. This does not mean there are no tests.
- **No tests**: no dedicated automated driver target was found in `indigo_test/`; coverage is not established.
- **Shared**: a ToupTek OEM wrapper follows the shared implementation's coverage. Per the agreed scope, no separate OEM audit is required; this does not claim its vendor binary was tested by the fake ToupTek build.

Inventory: 150 modules — 2 Complete, 37 Partial, 55 Not audited, 46 No tests, 10 Shared.

| Driver | Implementation | Automated test boundary | Coverage status | Recorded validation / remaining work |
| --- | --- | --- | --- | --- |
| `agent_alpaca` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_astap` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_astrometry` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_auxiliary` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_config` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_guider` | Hand-written | Production agent/filter + CCD simulator and deterministic image/pulse boundary | Partial | 66 mapped integration cases; DRV-124 fixed with 4/4 RAW cases passing normally and under AddressSanitizer; DRV-125–DRV-133 remain open. See the Guider Agent integration coverage section for validation and remaining acceptance work. |
| `agent_imager` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_mount` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_scripting` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_snoop` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_solver` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `agent_test` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ao_sx` | Generated | PTY/protocol simulator, Fake transport | Partial | New fake transport reproduces and verifies guider RA mapping, 10 ms units, stale/short reply and initialization rollback fixes. Both connection orders, sibling survival, last-close ownership, guider error recovery and sub-10-ms quantization pass; controlled queue/disconnect, physical limit responses and CENTER/UNJAM profiles also pass. Full applicable-standard audit is not recorded as complete. |
| `aux_arteskyflat` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_asiair` | Hand-written | None | No tests | `indigo_linux_drivers/`. No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `aux_astromechanics` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_cloudwatcher` | Hand-written | PTY protocol simulator | Partial | 4/5 ordinary scenarios pass; documented humidity conversion fails (DRV-085). ASan exposes timeout buffer underflow (DRV-086). Remaining firmware/heater cases listed in AUX_PROTOCOL_TESTS.md. |
| `aux_dragonfly` | Hand-written | UDP protocol simulator (opt-in) | Partial | 5/5 ordinary scenarios pass, including eight sensors/relays and a timed pulse. ASan exposes oversized-reply overflow (DRV-087). Authentication and additional fault cases remain. |
| `aux_dsusb` | Generated | Fake USB/SDK | Partial | New fake SDK: lifecycle, failed INIT/attach/open, timed exposure, focus abort, stop error and active removal pass; remaining coverage audit in progress |
| `aux_fbc` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_flatmaster` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_flipflat` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_geoptikflat` | Generated | Fake transport | Partial | New fake transport: handshake rollback, reconnect, brightness/light commands and error recovery pass; remaining audit pending |
| `aux_joystick` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `aux_mgbox` | Generated | PTY protocol simulator | Partial | All 32 scenarios pass on native macOS arm64; nine selected driver-instrumented ASan scenarios pass. Weather/GPS/Powerbox, readback/timeouts, shared/additional instances, parser recovery and pending-operation teardown covered. Physical hardware, TCP bridges and other-platform execution remain unverified. |
| `aux_ppb` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_rpio` | Hand-written | None | No tests | `indigo_linux_drivers/`. No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `aux_rts` | Generated | Fake transport | Partial | New fake RTS: duration/start/stop/abort and start failure pass; completion/abort stop failures and explicit retry also pass; remaining audit pending |
| `aux_skyalert` | Generated | PTY/protocol simulator, Fake transport | Partial | New fake transport covers every record field/read failure, empty/malformed numbers, open/write errors, pressure units and reconnect; both fake groups and PTY validation pass. No mechanical motion or writable controls. Full applicable-standard audit is not recorded as complete. |
| `aux_sqm` | Generated | PTY/protocol simulator, Fake transport | Partial | PTY plus new fake transport: all sensor fields/units, malformed/truncated records, read/write failures, failed initialization, resource balance and recovery pass. No writable device controls or mechanical motion. Full applicable-standard audit is not recorded as complete. |
| `aux_svbpowerbox` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_uch` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_upb` | Generated | PTY/protocol simulator | Partial | Added UPB/UPB2 host protocol simulator with elapsed movement. UPB2 controls, shared focuser progress/abort/reconnect, bad identity and malformed focuser initialization pass; USB-hub v1 and full fault/control matrix pending. |
| `aux_upb3` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_usbdp` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_wbplusv3` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_wbprov3` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `aux_wcv4ec` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `ccd_altair` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. An opt-in real-SDK hardware harness also exists. |
| `ccd_andor` | Hand-written | None | No tests | `indigo_optional_drivers/`. No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_apogee` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_asi` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_atik` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_atik2` | Hand-written | None | No tests | `indigo_mac_drivers/`. No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_baccam` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_bresser` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_dsi` | Generated | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_fli` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_iidc` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_mallin` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_meade` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_mi` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_ogma` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_omegonpro` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_pentax` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_playerone` | Generated | Fake SDK/USB | Partial | 45/45 fake SDK groups passed. The earlier continuous-acquisition argument and Bayer mapping gaps are closed; cooling faults, lifecycle/races and discovery retries are covered. No remaining concrete gap is recorded in this inventory, but a final complete-standard matrix is not established here. |
| `ccd_ptp` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_qhy` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_qhy2` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_qsi` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_rising` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_rpi` | Hand-written | None | No tests | `indigo_optional_drivers/`. No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_sbig` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_simulator` | Simulator | Direct bus | Partial | 16/16 direct integration groups passed, including logical devices, RAW geometry/bin, streaming/abort/reconnect, cooling, simulation modes and generated-noise file-camera input. Cooling/shutdown sanitizer checks passed. No SDK/USB boundary exists; a final complete-standard matrix is not established here. |
| `ccd_ssag` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_ssg` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_svb` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `ccd_svb2` | Shared ToupTek | Shared fake SDK | Shared | Uses the shared `ccd_touptek` implementation and its coverage status. No independent OEM fake build is claimed; OEM-specific audit is not required for this inventory. |
| `ccd_sx` | Generated | Fake USB/SDK | Complete | 24/24 applicable CCD/guider groups passed normally and with ASan/UBSan, including cooling, readout profiles, transport faults, hot-plug/queue cleanup and SDK-entry guide timing. Coverage/N/A matrix: `../indigo_drivers/ccd_sx/REFACTOR.md`. |
| `ccd_touptek` | Hand-written | Fake SDK/USB | Partial | 28/28 fake SDK groups passed; later targeted shutdown/lifecycle checks also passed. Earlier ROI/bin/noise geometry, Bayer, initialization/read failures, cooling and multi-camera/capacity gaps were addressed. Complete-standard matrix not established here. |
| `ccd_uvc` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `dome_baader` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `dome_beaver` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `dome_dragonfly` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `dome_nexdome` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `dome_nexdome3` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `dome_simulator` | Generated | Direct bus | Partial | Metadata, GOTO, park/unpark, shutter open/close, abort and pending rotation/shutter disconnect/reconnect pass; relative direction/wrap and parked-rejection assertions also pass. |
| `dome_skyroof` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `dome_talon6ror` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_asi` | Generated | Fake SDK/USB | Partial | Ten fake SDK groups recorded: initialization/lock cleanup, limits/settings/readback, motion/sync/abort, compensation recovery and attach/capacity retry. Full current-standard coverage audit remains unfinished. |
| `focuser_askar` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_astroasis` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `focuser_astromechanics` | Generated | PTY/protocol simulator | Partial | Host simulator models elapsed movement; measured progress, completion and pending disconnect pass; error matrix in progress |
| `focuser_dmfc` | Generated | PTY/protocol simulator | Partial | Host simulator models elapsed movement; measured progress, completion, abort and pending disconnect pass; remaining audit pending |
| `focuser_dsd` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_efa` | Generated | PTY/protocol simulator | Applicable matrix implemented | 55 named cases; PlaneWave/Celestron, binary framing, calibration, queued lifecycle. See migration results below. |
| `focuser_fc3` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_fcusb` | Generated | Fake USB/SDK | Partial | New fake SDK: lifecycle, failed INIT/attach/open, power/frequency/direction, timed completion, abort, stop error and active removal pass; remaining coverage audit in progress |
| `focuser_fli` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `focuser_focusdreampro` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_ioptron` | Generated | PTY/protocol simulator | Applicable matrix implemented | 40 named scenarios; migration validation below. Hardware wire assumptions remain explicit. |
| `focuser_lacerta` | Generated | PTY/protocol simulator | Applicable matrix implemented | 43 scenarios; final migration validation below. Hardware and other-platform execution remain separate. |
| `focuser_lakeside` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_lunatico` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `focuser_mjkzz` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_mjkzz_bt` | Hand-written | None | No tests | `indigo_mac_drivers/`. No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `focuser_moonlite` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_mypro2` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_nfocus` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_nstep` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_optec` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_optecfl` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_primaluce` | Generated | PTY/protocol simulator | Partial | Host simulator models elapsed focuser/rotator movement; focuser progress/abort tests pass; remaining shared-device/control audit pending |
| `focuser_prodigy` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_qhy` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_robofocus` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_steeldrive2` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_usbv3` | Generated | PTY/protocol simulator | Partial | Host simulator models elapsed movement; bounded driver motion, measured progress, abort and pending disconnect pass; error matrix in progress |
| `focuser_wemacro` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `focuser_wemacro_bt` | Hand-written | None | No tests | `indigo_mac_drivers/`. No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `gps_gpsd` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `gps_nmea` | Generated | PTY/protocol simulator, Fake transport | Partial | New fake transport covers short/malformed/checksum/oversized-token input, finite numeric validation, all eight source selections, fix transitions, DOP/satellite counts, coordinate/time conversion, UTC rollover, open/read loss and reconnect. PTY baseline also passes; remaining mixed-source/partial-input audit in progress. |
| `gps_simulator` | Generated | Direct bus | Partial | Metadata/properties and no-fix → 2D → 3D, coordinate/time/DOP output, advanced visibility, polling cancellation and reconnect reset pass. No transport/parser/failure injection boundary exists in this simulator. Full applicable-standard audit is not recorded as complete. |
| `guider_asi` | Hand-written | Vendor-header-based fake SDK | Partial | 3/9 scenarios pass; six failures reproduce DRV-090–DRV-094. macOS runs supported x86_64 driver via Rosetta. Multi-device and full timing coverage remain. |
| `guider_cgusbst4` | Generated | PTY protocol simulator + existing fake transport | Partial | PTY: 4/5 scenarios pass. Explicit INDIGO dialect works; numeric PHD2 dialect rejects emitted commands (DRV-095, protocol confirmation needed). Replacement/timing audit remains. |
| `guider_gpusb` | Generated | Fake USB/SDK | Partial | New fake SDK: lifecycle failures, four directions, cross-axis pulses, reversal, zero stop and active removal pass; failed-replacement preservation and single-write zero-stop regressions pass; timing/capacity audit pending |
| `mount_asi` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `mount_ioptron` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `mount_lx200` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `mount_mxhd` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `mount_nexstar` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `mount_nexstaraux` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `mount_pmc8` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `mount_rainbow` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `mount_simulator` | Simulator | Direct bus | Partial | Existing full simulator suite and new pending manual-motion/guider disconnect, reconnect and pulse reversal tests pass after timer cleanup fixes; remaining shared-order/motion audit pending. |
| `mount_starbook` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `mount_synscan` | Generated | PTY/protocol simulator | Partial | Full existing serial/UDP suite passes after zero-delta HOME fix and isolated park storage; standard coverage audit pending |
| `mount_temma` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `polaralign_simulator` | Simulator | Direct bus | Partial | Metadata/properties, dual-axis nonzero movement, abort, pending disconnect/reconnect and fresh move pass; configured travel-limit and reset-during-motion assertions also pass. |
| `rotator_asi` | Generated | Fake SDK/USB | Partial | 11 fake SDK groups passed. Explicit gaps remain: executing-handler/disconnect overlap, queued hot-plug shutdown, queue/registration failures, invalid discovery counts and commands racing initial delayed reads. |
| `rotator_falcon` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `rotator_lunatico` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `rotator_optec` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `rotator_simulator` | Generated | Direct bus | Partial | Metadata/visibility, normal/reversed mapping, SYNC, shortest-path wrap, progress/completion, abort, pending disconnect/reconnect and fresh motion pass. No SDK/transport or additional device controls exist. |
| `rotator_wa` | Hand-written | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `system_ascol` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `wheel_asi` | Generated | Fake SDK/USB | Partial | Five fake SDK groups passed: attach/capacity retry, ordinary movement, required initialization/slot-count errors, motion/calibration errors and interrupted calibration/reconnect. Full current-standard coverage audit remains unfinished. |
| `wheel_astroasis` | Generated | Fake SDK/USB | Partial | Ten fake SDK groups passed. Normal teardown does not establish concurrent hot-plug/shutdown coverage; hidden Bluetooth setters are outside normal public-bus coverage. Full current-standard audit remains unfinished. |
| `wheel_atik` | Generated | Fake USB/SDK | Partial | New fake SDK/HID: lifecycle failures, slot movement, command/read errors, pending removal and reconnect pass; remaining capability/identity audit pending |
| `wheel_fli` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `wheel_indigo` | Generated | PTY/protocol simulator, Fake transport | Partial | Elapsed mechanical simulator, measured/target separation, BUSY conflict, pending disconnect/reconnect and fresh move pass. Fake transport covers initialization, lost echo, malformed poll and bounded timeout; all three fake transport groups and simplified PTY tests pass. |
| `wheel_manual` | Generated | Direct bus | Complete | Applicable driver-owned coverage complete: metadata, eight-slot capability, all slot selections, custom-name prompt, reconnect pass. No transport, physical motion, calibration or shared handle exists. |
| `wheel_mi` | Hand-written | None | No tests | No dedicated automated driver test target found in `indigo_test/`; coverage not established. |
| `wheel_optec` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `wheel_playerone` | Generated | Fake SDK/USB | Partial | 12 fake SDK groups passed normally and with sanitizers. Arbitrary concurrent hot-plug/shutdown coverage is not established; full current-standard audit remains unfinished. |
| `wheel_qhy` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `wheel_quantum` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `wheel_sx` | Generated | Fake USB/SDK | Partial | New fake HID: lifecycle failures, slot movement, command/read errors, pending removal and reconnect pass; remaining capability/identity audit pending |
| `wheel_trutek` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |
| `wheel_xagyl` | Generated | PTY/protocol simulator | Not audited | Existing automated test target; full applicable-standard audit not completed. |

## ToupTek shared hot-plug shutdown (2026-09-08)

The hand-written ToupTek/OEM lifecycle now follows the generated SDK sequence: connection and discovery handlers share the driver task mutex with disconnected-device verification; rejected shutdown preserves the existing callback registration; accepted shutdown deregisters, drains queued events, then detaches and deletes the queue. SDK-id discovery and multi-class camera/guider/wheel/focuser teardown remain driver-specific. The fake SDK lifecycle test asserts no deregistration/re-registration on rejection. Its pending-shutdown test verifies all 64 accepted notifications execute (96 SDK enumerations), followed by clean detach and successful reinitialization. Targeted lifecycle, pending-shutdown and hot-plug/partial-attach/active-removal tests passed without hardware.

## All generated hot-plug transports drain before detach (2026-09-08)

The generator now shares SHUTDOWN emission across libusb, SDK and HID transports. The architecture regression covers all three plus SDK discovery retries: serialized connected-device verification, rejection before deregistration, queue drain before detach/delete, and retry cancellation without holding the task mutex. The PlayerOne fake SDK queued-shutdown scenario also detects any detach while its blocked queue callback is unfinished. Validation passed: the generator suite (including 86 DSL cases and four hot-plug variants), the three targeted PlayerOne fake SDK scenarios, and full root `make all` for macOS x86_64/arm64. Hardware tests were not run.

## Shared handler-queue drain (2026-09-08)

`indigo_queue_drain()` waits for both pending and running tasks, including delayed and callback-enqueued work, without canceling tasks. Producers/recurring tasks must be stopped first and the caller must own the queue lifetime. Calls from the worker or with NULL return false. Queue notifications broadcast to wake the worker and all drain/remove waiters.

The timer unit suite covers an empty/reusable queue, a blocked running callback, delayed and callback-enqueued tasks, two concurrent drain callers, self-call rejection and wakeup after removal of pending tasks. The generator emits a single drain call after USB deregistration and before detach/delete; its regression test rejects the old generated condition variable/callback. The existing SX pending-discovery and rejected-shutdown tests exercise the integration. No hardware tests are used. Validation passed: complete timer unit suite, generator regression suite, SX pending-discovery shutdown and shared-lifecycle/rejected-shutdown scenarios, and the macOS SX driver build.

## SX complete applicable-standard fake USB suite (2026-09-08)

The SX target now contains 24 named groups covering the applicable CCD and guider standard scenarios. The coverage/N/A matrix and validation details are in `../indigo_drivers/ccd_sx/REFACTOR.md`. Normal and ASan/UBSan runs passed all 24 groups; final metadata and zero-valued guide completion assertions passed narrow reruns. The suite includes 80 monotonic USB ON/OFF pulse samples with discarded warm-ups, both idle and during acquisition. Hardware tests were not run.

Regressions fixed in the SX `.driver` include failed-open resource rollback, config-descriptor and ICX453 buffer cleanup, incomplete control packets, propagation of read/clear errors, zero-signal interlaced normalization, invalid-bin preservation, target/readback separation, unavailable cooling, and guider replacement/coalescing/error/stop handling. C is regenerated, not hand-edited. User-approved generator fixes cover duplicate arrivals, failed master attach, INIT rollback and SHUTDOWN synchronization/draining. Emitted cleanup uses INDIGO allocation helpers.

The generator test retains its 86 attribute/value/block checks and adds libusb lifecycle invariants; SX's fake tests execute those generated paths with actual failures and controlled concurrency. Configuration codecs and image containers remain framework responsibilities.

## CCD cooling and fake-boundary extensions (2026-09-08)

The camera standard explicitly requires cooler ON/OFF, separate target/measured temperatures and unit conversion, supported power readback, polling/settling, individual read/write failures and recovery, unsupported capability profiles and disconnect cleanup. Apply only controls the driver implements: SX does not report cooler power; the simulator has no SDK communication errors to inject. Hardware tests were not repeated. Altair remains covered as the shared ToupTek implementation, without a separate OEM target as requested.

- Player One: existing cooler polling, individual SDK failure and slow-initialization cases remain; added exact Bayer mapping and continuous SDK acquisition argument assertions.
- ToupTek: independent measured temperature, target conversion, temperature and both power-read failures, recovery and settling; failed initialization reads and acquisition setup, Bayer mapping, ROI/bin noise payloads, multiple-camera identity and capacity recovery.
- SX: new fake USB target covers cooled/uncooled profiles, target conversion, measured temperature, settling, ON/OFF, command/read/short-reply failure recovery, shared guiding, progressive/interlaced/ICX453 readout, ROI/bin/shutter and transfer failure recovery. Regressions exposed lost targets during polling, visible unsupported cooling controls, short cooling replies accepted as success and pixel read failures treated as success. Fixes live in `ccd_sx/indigo_ccd_sx.driver`; C is regenerated.
- CCD simulator: added RAW geometry/bin, streaming/abort/reconnect, cooler target/settling/power, camera simulation modes and file-camera generated-noise inputs. Shutdown now detaches slave devices before freeing their master.

Fake image data uses coordinate-addressable deterministic noise from `integration/ccd_test_noise.h`; no simulator image arrays are linked into fake SDK tests. New files are included in Xcode. ToupTek framework encoding/video/upload matrices were removed from the driver suite. The unified Driver test coverage status table above incorporates these additions and supersedes the initial CCD gap list; these additions do not claim exhaustive branch coverage.

Validation: Player One 45/45, ToupTek 28/28, SX 5/5 and CCD simulator 16/16 normal tests pass. The final ToupTek power/initial cooler-read assertions and simulator per-property revision waits passed narrow reruns. Simulator cooling/shutdown also passes ASan/UBSan (test and driver instrumentation; prebuilt dependencies excluded). SX, ToupTek and simulator driver builds pass. Repeated SX generation produces identical C/header/main files; Xcode project lint and whitespace checks pass. No hardware tests ran.

## Driver test scope by class (2026-09-08)

The same driver-only scope now includes AO corrections/reset/limits/shared guider ownership and GPS parsing/fix lifecycle/source selection/reader cleanup, with separate hardware acceptance. AO steps are distinguished from guider pulse durations. No implementation or hardware tests were run for this documentation update.

`DRIVER_TESTING_RULES.md` now defines shared driver-only scope and separate fake SDK/protocol and real-hardware acceptance for mounts, wheels, focusers, rotators and guiders. Camera pulse scenarios and SDK-entry timing measurements moved into the guider standard, referenced by camera and mount sections. Framework behavior and optical/electrical performance measurements are outside driver acceptance. These are coverage requirements, not claims of newly implemented tests. This documentation-only change runs no driver or hardware tests.

## Player One final plan audit (2026-09-08)

`test_ccd_playerone_sdk` now has 44 named fixture-isolated cases. The added groups cover required initialization/attribute/preset read failures with untouched outputs, optional mode failures, geometry failures, SDK identity and bounded strings, individual cooler failures, partial control writes/readback, partial guider attachment, before-handler/setup/readout aborts, queued shutdown, bounded discovery retries, invalid/aligned ROI/bin behavior and RAW16-only capability, and slow initialization crossing accelerated polling intervals. All camera SDK calls are checked for overlapping access and use after close; fixture cleanup failures affect the executable's exit code and an unmatched filter fails.

At that checkpoint images used the shared CCD simulator fixtures; the current fake tests use generated noise. RAW pixels and Bayer metadata are checked. Framework codec, container and upload-destination tests were removed; the suite now contains 44 driver-focused cases. The modified frame-type/exposure-unit, finite-stream-count and RAW/ROI/bin cases passed after scope reduction; hardware tests were not repeated. Timing retains 120 measured guide pulses without a machine-dependent error threshold. Normal, safe-readout and ASan/UBSan validation passes; final test-only additions are rerun narrowly in each configuration.

The generator suite now checks 86 attribute/value/block cases, including opt-in `discovery_retries`. The Player One integration test verifies late SDK visibility without another arrival, six-retry exhaustion, removal cancellation, and pending-retry shutdown cleanup. Reference-driver regeneration changes only the already approved registration/queue rollback paths. Hardware test modes `--acceptance` and `--suffix` isolate physical interruption and flash/name acceptance; `test-ccd-playerone-reload-hw` uses a shared bus and actual dynamic driver unload/reload, excluding configuration SAVE to avoid user settings. These modes remain outside normal integration tests and use existing Xcode-referenced source files.

The earlier gap lists below are historical checkpoints. The final migration matrix and named coverage mapping are in `ccd_playerone/REFACTOR.md`; hardware-only and platform deferrals are recorded there and in `TESTING.md`. Electrical ST4 measurements, other/colder models, multiple real cameras and unavailable OS runners remain unverified. Driver tests do not replace exhaustive shared-framework codec or every hypothetical future SDK capability test.

## Camera test standard and generator parsing (2026-09-08)

Camera fake SDK/USB and hardware scenarios now live in `DRIVER_TESTING_RULES.md`; root and test `AGENTS.md` contain references only. The standard now requires generated noise for fake images and defines guider timing at SDK entry.

The generator architecture suite checks all supported named DSL attributes and code blocks with 85 table cases, including true/false settings, generated behavior and parser traces. Separate fixtures check `name`/`name_value` in either order and `name` alone. Tests exposed `handler` being consumed as `handle`; the user-approved fix parses `handler` first. Both declaration orders now have regression coverage. The prefix matching implementation remains unchanged.

## Expanded Player One coverage (2026-09-08)

The fake SDK suite now has 32 fixture-isolated cases. Added coverage includes property metadata, repeated shared lifecycle, simulator-backed RAW pixel verification in four SDK formats with ROI/bin mapping, frame types and output encodings, controls and failures, suffix boundaries, simultaneous guide axes, registration/global-lock rollback, active removal, upload destinations and video headers, controlled final-frame/abort orders, cooled-profile polling, discovery bursts and queue failure, the real readout deadline, capability rebuilding and busy controls. Guider timing now measures 120 pulses at 20/50/100/250/500 ms, all directions, three repeats, idle and during exposure.

The temperature case exposed a polling race: an asynchronous handler read the measured value after polling could overwrite the requested value. The driver now preserves measured values and consumes the requested target. Shared open is transactional and no longer needs a private `opened` flag.

Validation: all 32 cases pass in the normal macOS build, explicit `POA_SAFE_READOUT=1` build, and arm64 ASan/UBSan build after the temperature fix. Instrumentation covers the test, production driver and separately compiled framework objects; prebuilt dependencies remain uninstrumented. The generator architecture target passes all five groups, including its 85 attribute/value/block fixtures.

Remaining standard gaps include exhaustive property/item boundaries and SDK read/partial-write failures, duplicate model names and bounded SDK strings, full payload decoding for every output encoding, queued-discovery shutdown races, and proof of every SDK call's shared-handle serialization. These remain deferred coverage, not implied passes. Hardware coverage is recorded separately in `TESTING.md`.

## Player One camera generator migration (2026-09-08)

Added `integration/test_ccd_playerone_sdk.c`, initially with three passing hardware-free cases using separately compiled production driver/framework and SDK/USB substitutes: CCD property definitions, RAW BLOB acquisition and isolated CONFIG save; guider-first shared connection, pulse completion and CCD disconnect while guider remains connected; exact three-frame streaming. Added opt-in `test-ccd-playerone-hw`, excluded from normal integration targets. Full capability/error/payload/configuration-reload/queue-interleaving coverage remains planned in `ccd_playerone/REFACTOR.md`. Physical Mars-C II baseline acquired images but failed later lifecycle validation with a libusb mutex assertion; physical output and hot-unplug tests remain pending.

The generated driver now additionally passes finite streaming, bounded SDK short-wait retry, abort/reacquire and guiding during long exposure, readout failure recovery, Open/Init/config-enumeration rollback, optional guider/capacity and SDK-id-based removal, failed master attach recovery, busy guards, configuration reload/suffix boundaries, and pulse replacement/disconnect cleanup. Guider completion uses `guider_ra_finalizer`/`guider_dec_finalizer`; regression checks explicitly observe BUSY before completion so missing initiation updates after generator suppression cannot silently pass. The real SDK lifecycle test subsequently passed, including driver shutdown/reinitialization (the SDK library remains loaded). The full adversarial matrix in REFACTOR.md is not yet claimed complete.

Added a thirteenth case measuring guider ON-to-OFF duration at entry to fake `POASetConfig`, using `CLOCK_MONOTONIC` and a mutex-protected per-camera/per-direction edge recorder. It ignores redundant OFF writes, excludes one warm-up per phase, and reports each requested/measured duration, signed error in ms/percent, and min/mean/median/p95/p99/max/standard deviation. Three repeats of NORTH/SOUTH/EAST/WEST at 20/100/250 ms run both idle and during a 30-second exposure (72 measured pulses). Completion and acquisition coexistence are assertions; host scheduling precision is reported without a flaky fixed error threshold. This measures driver-to-SDK command timing, not physical ST4 output. On macOS arm64, 2026-09-08: idle mean +2.426 ms, max +5.039 ms; exposure mean +3.049 ms, max +5.041 ms. All 13 cases passed. Source remains in the existing Xcode integration-test reference.

This document records the automated test suite added under `indigo_test/` and the remaining follow-up work. The suite is intentionally hardware-free: it links against the built INDIGO library and simulator driver archives, then exercises public APIs through unit and in-process integration tests.

## Astroasis wheel generator migration (2026-09-08)

Added `integration/test_wheel_astroasis_sdk.c` and the normal integration target
`build/integration/test_wheel_astroasis_sdk`. The generated driver is compiled
separately against Oasis SDK/libusb stubs, with real bus, queue and wheel/base
handlers. Configuration is redirected by a test-local framework object to a
fresh temporary directory; the fixture removes it without changing HOME.

Ten fixture-isolated groups cover:

- public metadata, inherited properties/configuration/profile roundtrip,
  custom-property visibility and factory-reset confirmation hint;
- five/16-slot counts, allocation bounds, 50/51-byte filter-name limits,
  one-based positioning and actual bus clamping;
- global-lock/open/required-read failures, optional Bluetooth NOT_IMPLEMENTED,
  repeated connect/disconnect and shutdown rejection while connected;
- movement start/read failures, BUSY suppression, fractional/NaN requests,
  wrong-target/invalid status, timeout and checked recovery;
- calibration no-op/start/read errors, completion/timeout and competing
  movement/reset requests;
- suffix lengths 0/1/31/32/33, failed-write rollback, reconnect readback,
  reset no-op/failure/success/readback failure while remaining connected;
- default five-device capacity and recovery, independent USB/SDK ordering,
  multiple removals, failed/inconclusive scans and USB reference balance;
- probe open/version/model/suffix failures, attach retry, duplicate events,
  descriptor/product filtering, full-length naming and replacement SDK IDs;
- disconnect/unplug during operation and held SDK work, cancelled polling,
  no late property updates and balanced SDK/global-lock ownership;
- moving/calibrating/benchmarking initialization, invalid completion,
  and ignored direct requests to hidden Bluetooth properties.

All ten groups passed normally and with AddressSanitizer/UndefinedBehaviorSanitizer.
Instrumentation covers the test, generated driver and test-local base/framework
objects; the shared INDIGO library and vendor SDK are not instrumented (the
vendor SDK is replaced by stubs). The timeout cases accelerate callback dispatch
while retaining the production 240-poll budget. Existing ASI (5 cases) and Player
One (12 cases) SDK suites passed as regressions.

Added `integration/test_generator_architecture.c`, runnable with
`make -C indigo_test test-generator-architecture`, for the user-approved optional
`supported_architecture` attribute. The C test uses `test_runner.h` and is also
part of the normal integration suite. It creates a minimal synthetic AUX driver in
a temporary directory, independently of production drivers and SDKs. An intentionally
missing SDK header verifies that the fallback excludes implementation includes.
It verifies nine platform/CPU combinations
(including Intel-only and Apple-Silicon-only macOS examples), reverse extraction
for three expressions, a compiled/executed unsupported INFO/INIT/SHUTDOWN fallback
without SDK linkage, and omission of the attribute. Generation without the new
attribute was also compared with the prior generator for six existing drivers;
all C/header/main outputs were byte-identical.

Hardware remains unavailable. SDK readiness on initial USB arrival, actual
name payload limits and operation/reset timing remain unverified. Probe failure
is retryable on a subsequent arrival event; no new automatic discovery retry is
claimed. Bluetooth stays hidden and its activation/write/readback hardware
matrix is deferred in `wheel_astroasis/REFACTOR.md`; the normal public bus cannot
exercise its dormant setters while the properties are undefined. Normal teardown
tests do not establish arbitrary concurrent hot-plug/shutdown safety.

## Player One wheel generator migration (2026-09-08)

Added `integration/test_wheel_playerone_sdk.c` and the normal integration target
`build/integration/test_wheel_playerone_sdk`. The generated driver is compiled
as a separate object against Player One SDK/libusb stubs. Tests use public driver
entry points and bus requests, a mutex-protected multi-wheel property cache,
and real wheel/device base handlers. A test-local framework object redirects
configuration files to a temporary directory without changing HOME; files are
removed after the run.

Twelve test cases cover:

- all exposed properties (`INFO`, `CONNECTION`, `CONFIG`, `PROFILE`,
  `PROFILE_NAME`, `WHEEL_SLOT`, `WHEEL_SLOT_NAME`, `WHEEL_SLOT_OFFSET`,
  `X_RESET`, `X_CUSTOM_SUFFIX`) and hidden/disconnected property visibility;
- slot names and offsets, configuration save/load/remove, profile naming and
  selection, five/16-slot metadata and the 50-byte filter-name boundary;
- repeated lifecycle requests, global-lock/open/metadata/position/suffix errors,
  invalid slot counts, asynchronous initial motion, bounded timeout and reconnect;
- first/last slots, bus range clamping, fractional/NaN rejection, BUSY request
  suppression, SDK movement/read failures, invalid and wrong-target replies;
- reset false-switch no-op, rejection during movement, failure, successful
  completion before disconnect/property deletion, and reconnect;
- empty/one/24-byte suffixes, 25-byte rejection, failed writes, reconnect/replug
  and full-length attached names;
- default five-wheel capacity, handles above 23, reverse connection order, reordered enumeration
  of attached wheels, duplicate USB arrival, capacity/retry, descriptor/enumeration
  errors, failed attach retry and malformed SDK names;
- disconnect and unplug during movement, including a held SDK read proving close
  waits for running work, cancelled polling and ignored disconnected requests;
- reversed USB/SDK arrival order, inconclusive SDK enumeration on removal,
  several removals in one USB event and separate per-USB-pointer reference balance;
- balanced SDK close/global locks and USB references during fixture teardown.

Verification: all 12 cases passed in the universal macOS build and in a native
AddressSanitizer/UndefinedBehaviorSanitizer build. The existing ASI wheel SDK
suite (5 cases) and timer/queue suite (86 cases) also passed. Sanitizers cover the
test, driver and test-local base/framework objects; the shared INDIGO library
itself is not rebuilt with instrumentation. The initialization-timeout test
accelerates delayed dispatch while retaining the production poll count.

Physical hardware is unavailable. Real SDK readiness and physical reset behavior
remain deferred. The optional `sdk.unplug_match` generator block is used only
by Player One; SDK-handle presence determines removal even if USB and SDK
arrival order differ. No new shutdown synchronization is introduced. No claim
is made that the stubs establish real SDK readiness timing or that arbitrary
simultaneous hot-plug/shutdown races are covered. Test outputs were cleaned.

## ToupTek guide coalescing and connection serialization (2026-09-08)

Added two hardware-free cases to `test_ccd_touptek_sdk.c`: queued full-vector guide replacements on both axes must issue exactly one SDK pulse per axis (DRV-067), and an immediately due temperature task must survive held camera initialization and run after connection completes (DRV-068 false-positive check). The guide test reproduced duplicated commands before the fix. The temperature test uses the real queue/master-mutex path and shortens only the first monitoring deadline, avoiding a five-second sleep.

Validation: native arm64 SDK suite 25/25 passed, including both new scenarios; universal ToupTek production build passed. No Rosetta or hardware run. Test build artifacts cleaned.

## ToupTek review regressions (2026-09-08)

Extended `integration/test_ccd_touptek_sdk.c` for DRV-061–DRV-066: opposite-direction replacement and zero-vector cancellation on both guide axes; replay of actual recurring callbacks after disconnect; stale ERROR/NOFRAMETIMEOUT callbacks during acquisition setup followed by successful same-mode exposures; combined camera/ST4/wheel/focuser discovery; manual relative motion after switching out of automatic compensation while still moving; and all-off CONFIG followed by SAVE on CCD, wheel and focuser. The guide, focuser-transition and CONFIG tests reproduced failures before their respective fixes. The recurring-task test deliberately replays the survivor task after disconnect; it does not force the queue's internal cancellation interleaving. Combined flags are synthetic and do not establish support for untested physical hardware.

Validation: native arm64 23/23 SDK scenarios and 86/86 timer/queue cases passed. Native AddressSanitizer passed the three existing races and all three new standalone scenarios (prebuilt libraries uninstrumented; leak detection disabled). No Rosetta or hardware execution. Test artifacts cleaned with `make -C indigo_test test-clean`.

## ToupTek SDK queues and finalizers (2026-09-08)

`integration/test_ccd_touptek_sdk.c` builds as `build/integration/test_ccd_touptek_sdk`, with separately compiled production driver, simulator image fixture and framework dispatcher objects. Twenty-five hardware-free scenarios use public bus requests, the real handler queues and SDK/USB replacements. The observer, test cases and config-path replacement are consolidated into `test_ccd_touptek_sdk.c`; no auxiliary ToupTek test source/header files are needed. After consolidation, the universal binary rebuilt without warnings and the configuration persistence/upload scenario passed.

Coverage:

- Lifecycle: CCD+guider connection in both orders, shared handle ownership, wheel/focuser teardown, queue/thread affinity, failed Open/queue creation/registration, unload/reload, rejected shutdown and callback re-registration, duplicate arrival, partial attach rollback and unplug during active acquisition, guiding and wheel initialization.
- Properties: enumerate all four logical devices, validate published vectors/items and round-trip passive writable properties through the real base/driver dispatch. Dedicated scenarios cover active workflows. CCD coverage includes all nine advanced controls, gain, offset, fan, heater, cooler, temperature, LED, conversion gain, all advertised RAW depths and RGB8, ROI, binning policies and frame types. Tests preserve gain fall-through versus offset early return.
- Acquisition: exposure completion, admission while BUSY, abort, watchdog, Trigger/PullImage failures, obsolete Stop notifications, reconnect/recovery, finite/infinite streaming, all seven image formats and SDK error/no-frame/no-packet events. The image callback runs on a separate thread. RAW pixels are compared with a 16 × 16 sample from `ccd_simulator/indigo_ccd_simulator_data.c`; FITS headers are checked. Local SER/AVI files are finalized and their signatures checked.
- Guider: all four directions, SDK direction/duration arguments, replacement while BUSY, all-zero cancellation, subsequent requests, reconnect and pulse completion timing.
- Wheel: 5/7/8-slot models, endpoint requests, calibration, SDK errors and cancellation. Focuser: absolute/relative motion, sync, limits, backlash, reverse, beep, abort, automatic/manual property permissions, actual temperature compensation and fallback to the internal sensor.
- Persistence/output: real CONFIG SAVE/LOAD for all logical devices, restored CCD/wheel settings, CLIENT/LOCAL/BOTH/NONE upload modes and temporary image/video files. The separately compiled framework dispatcher redirects only its config-directory lookup; serialization and parsing remain real. The test does not change HOME or use the user's config directory.

Three additional deterministic race scenarios close the previously deferred step 6 cases:

- Final frame versus abort: test-only gates force both orders on a one-frame stream. Abort first produces no frame and the standard finite-stream ALERT state; frame first delivers exactly one frame, completes streaming and reports ALERT for the now-inapplicable abort. Both orders allow a subsequent successful exposure.
- Disconnect inside the real SDK callback: pause its enqueue call after capturing the old generation, start disconnect, and assert SDK Stop is waiting to join that callback. Release it, verify no stale frame is pulled/published and the guider remains connected, then reconnect the CCD and acquire a new image.
- Rapid hot-plug/pending shutdown: eight full reconnect cycles with 1,024 alternating arrival/removal notifications, followed by shutdown with 64 explicitly pending USB events. Assert cancellation without further enumeration, balanced handles/global locks, no attached devices, and successful reload/connect/disconnect/shutdown.

The synchronization gates exist only in the harness and have 10-second deadlines. The production callback enqueue API is redirected through a gate that delegates to the real API; no production queue implementation or driver logic is modified.

Guider timing uses `CLOCK_MONOTONIC` at the fake SDK ST4 command and at the matching zero-valued OK property update. Each run measures 20 pulses: EAST/WEST/NORTH/SOUTH at 20, 50, 100, 250 and 500 ms. Output includes each measured duration and signed error, plus mean signed error, mean absolute error, p95 absolute error and maximum absolute error. A broad -5 to +250 ms completion bound catches gross regressions; these measurements characterize software completion latency, not electrical ST4 signal duration or an accuracy guarantee under arbitrary system load.

Only watchdog waits of at least 25 seconds are shortened by the test hook, while their original duration is checked. Guiding and normal completion delays are not accelerated. Injected SDK/queue errors and watchdog log messages are expected. `INDIGO_TEST_FILTER` selects cases by a substring of their displayed title.

Initial validation: the universal test binary compiled without compiler warnings; the full native macOS arm64 run passed all 17 scenarios. Its 20 pulse samples measured mean +3.703 ms, mean absolute 3.703 ms, p95 absolute 5.048 ms and maximum absolute 5.066 ms. The full x86_64/Rosetta run also passed all 17 scenarios: mean +2.947 ms, mean absolute 2.947 ms, p95 absolute 5.048 ms and maximum absolute 5.063 ms. Both runs exited 0; `git diff --check` passed. Test build artifacts were removed with `make -C indigo_test test-clean`.

Race-extension validation: the full 20-scenario suite passed on native macOS arm64 and x86_64/Rosetta, both exiting 0. All three new race cases passed without gate timeouts. The universal build emitted no warnings; `git diff --check` passed and `make -C indigo_test test-clean` removed build artifacts. Repeat guiding mean errors were +3.443 ms (arm64) and +3.899 ms (Rosetta).

The tests retain current behavior: reverse-motion SDK failure is reported as OK by the driver; after cancelling a guide pulse by disconnect, the reconnect test submits both axis items to replace the retained direction value. The fake SDK cannot validate real USB transport, vendor SDK races, electrical timing, physical Intel hardware or sustained multi-camera load. Those remain hardware validation work.

## Opt-in ToupTek physical camera test (2026-09-08)

`hardware/test_ccd_touptek_hw.c` is deliberately outside the hardware-free integration suite. Run it only with `make -C indigo_test test-ccd-touptek-hw`; this target builds `build/hardware/test_ccd_touptek_hw` and invokes it with `--run`. Direct execution without `--run` exits 2 before initializing INDIGO or USB. Neither `make test` nor `test-integration` builds or runs this executable. Set `INDIGO_TEST_DEVICE` to an exact discovered camera name if multiple cameras are attached; otherwise exactly one camera with its matching guider is required.

The test compiles the production driver separately and links real INDIGO, libtoupcam and libusb, with no SDK/USB replacements. It connects the guider before the camera, acquires three 0.1-second RAW exposures, aborts a 5-second exposure, captures a five-frame stream, aborts an indefinite stream after at least three frames, sends 100 ms pulses in all four directions, verifies guider operation after CCD disconnect, closes the shared handle, reconnects camera first and acquires another image. RAW signatures, dimensions, payload sizes and frame counts are checked. Only CCD_IMAGE updates count as new frames; reconnect property definitions are metadata. Cleanup disconnects both logical devices and shuts down the driver. The test does not save configuration or image files, but it does operate the physical ST4 outputs.

Native arm64 validation passed on Touptek GPCMOS01200KMB (camera suffix 04E69B): 12 valid RAW frames at 1280 × 960, 1,228,812 bytes each, including the post-reconnect frame. Both camera and guider disconnected and driver shutdown succeeded. The sandbox could see USB inventory but the SDK could not enumerate the camera; the successful run used explicitly approved execution outside the sandbox. Rosetta was not run. Compilation/linking succeeded; the linker reported that the bundled SDK targets macOS 11.0 while the repository build flags target 10.10. `make -n test` confirmed absence of the hardware executable, direct execution without `--run` returned 2, and `git diff --check` passed. Build artifacts were removed with `make -C indigo_test test-clean`.

Step 7 extension: `HW_DRIVER=altair` selects the real Altair SDK; `HW_HOTPLUG=1` additionally requires operator-assisted cable removal/reconnection during streaming (180-second deadlines per phase). The test verifies deletion of camera/guider properties, rediscovers the same devices and captures a new image plus guiding pulse after replug. All normal hardware runs now also verify complete driver unload/reload and a new exposure in the same process. The executable remains excluded from default tests.

Altair ALTAIRGP224C #E61F50 passed the native physical hot-plug run with 1280 × 960 RGB RAW payloads (3,686,412 bytes), then passed a separate native run including the newly added SDK unload/reload check. Both exited 0. No Rosetta run was performed. The earlier ToupTek run preceded the additional unload/reload check; no physical ToupTek cable-removal test is claimed.

Step 7 software verification: all 20 native SDK cases and all 86 native timer/queue cases passed. The three deterministic SDK race cases also passed under AddressSanitizer with instrumented driver/harness/framework dispatcher/image fixture; prebuilt libraries are not fully instrumented. All eleven branded drivers compiled and linked for arm64/x86_64. Full initialization-call comparison against the refactor baseline found no public schema changes. `git diff --check` passed; test and ASan artifacts were cleaned.

Physical wheel/focuser hardware, simultaneous multiple physical cameras, Linux/Windows toolchains and native Intel hardware remain unverified. Hardware ST4 electrical pulse duration is not measured.

## Goals

- Provide repeatable tests that run without physical astronomy hardware.
- Keep the harness dependency-free and compatible with the existing make build.
- Separate fast unit tests from simulator and bus-level integration tests.
- Validate driver lifecycle, property enumeration, property changes, and protocol adapters through public APIs.
- Preserve manual hardware validation in `TESTING.md` for real devices and vendor SDK behavior.

## Current Layout

```text
indigo_test/
  CHANGES.md
  Makefile
  test_runner.h
  fixtures/
    protocol/
  unit/
  integration/
  benchmark/
```

Unit tests live in `indigo_test/unit/`. Integration tests live in `indigo_test/integration/`. Protocol parser fixtures live in `indigo_test/fixtures/protocol/`. Timing benchmarks live in `indigo_test/benchmark/`.

## Build Targets

- `make -C indigo_test test` runs unit and integration tests.
- `make -C indigo_test test-unit` runs pure unit tests.
- `make -C indigo_test test-integration` runs bus and simulator-driver integration tests.
- `make -C indigo_test benchmark` builds and runs the timing benchmarks.
- `make -C indigo_test test-clean` removes generated test binaries and dSYM files.

Run `make all` from the repository root first if `build/lib/libindigo` or the required simulator driver archives are missing.

## Benchmarks

`indigo_test/benchmark/` holds measurement tools rather than tests. They report numbers, never assert, and always exit `0`, so they are deliberately excluded from the `test` target: results depend on machine load and on kernel timer behavior, and are meaningful only when compared against another run on the same machine.

All benchmarks report min, mean, median, p95, p99, max, and standard deviation, discard warm-up samples, and repeat the whole scenario set several times so that one-time costs and outliers stay visible. Timings come from a monotonic clock read inside the benchmark, so they are independent of the clock the library uses internally. Because they call only long-stable API, the same sources can be built against a second `libindigo` to compare two implementations on one machine.

`benchmark/bench_timer.c` measures per-fire latency and jitter of `indigo_set_timer()` and `indigo_reschedule_timer()`: a one-shot timer, a zero-delay timer that isolates dispatch cost, a self-rescheduling 10 ms timer, the interval between its fires, and a burst of timers that all come due at the same instant.

Comparison results are written up beside the sources. `benchmark/TIMERS_AND_QUEUES_MERGE_REVIEW.md` records the `master` against `refactoring` measurements for both benchmarks, the correctness differences behind them, and the open items before that branch merges.

`benchmark/bench_queue.c` measures the same properties for handler queues: the cost of `indigo_queue_add()` against a backlog of pending tasks, dispatch latency on an idle queue, lateness of a delayed task, a task that re-adds itself every 10 ms with the interval between its fires, and the per-task cost of draining a batch of ready tasks. The drain is measured both on an otherwise idle library and while another thread continuously creates and cancels timers, which exposes any lock shared between the timer and queue subsystems.

## Test Harness

`indigo_test/test_runner.h` provides a small dependency-free C test runner with local assertion macros. Each test executable returns `0` on success, returns non-zero on failure, and prints the failing test and assertion location.

## Unit Coverage

Implemented unit suites:

- `test_align_math.c`: parallactic angle against IAU SOFA/ERFA reference values and its meridian behavior, and the derotation rate against the measured rate of change of the parallactic angle (the quantity the mount agent derotates with), including reference values, the zero crossings due east and west, and the growth towards the zenith.
- `test_aux_math.c`: dewpoint and Bortle-scale helper behavior.
- `test_base64.c`: known vectors, binary round trips, padding, and newline-tolerant decoding.
- `test_bus_helpers.c`: numeric/string conversion, sexagesimal conversion, pixel scale, local service trimming, switch helpers, and property value/target copying.
- `test_bus_property.c`: text, number, switch, light, and BLOB property initialization, matching, copying, resizing, and release behavior, with every test case exercising all vector types.
- `test_dome_azimuth.c`: hour wrapping, azimuth distance, dome azimuth range checks, mirror symmetry of the pivot offsets across the equator, agreement with an independent vector model of the same geometry over a latitude, hour angle, declination and OTA offset sweep, and side of pier handling (the OTA staying on the reported side, no azimuth jump when a mount tracks past the meridian, agreement with counterweight down for a normal mount).
- `test_md5.c`: known MD5 vectors, partial MD5, and file-prefix MD5.
- `test_polynomial_fit.c`: polynomial value, derivative, extrema, minimum search, string output, and exact line fitting.
- `test_protocol_json.c`: JSON escaping, device/server adapter serialization, parser routing for number and switch changes, BLOB URL output, and malformed input handling.
- `test_protocol_xml.c`: XML escaping, device and client adapter serialization, parser routing for text changes and BLOB URL mode, remote property events, and malformed input handling.
- `test_raw_image.c`: RAW type constants, Bayer extension detection, Bayer channel equalization, saturation masks, and contrast.
- `test_timer.c`: timer delay conversion, callback execution, data callbacks, mutex-wrapped callbacks, cancel/reschedule behavior including data-to-plain callback transitions, device-wide timer cancellation, fork-time scheduler restart and inherited-timer cleanup while a callback is active, signed queue priority ordering, queue handler run-time limit adjustment, queue pending-task limit configuration/reporting, queue removal, and queue deletion.
- `test_token.c`: token parsing, device token add/update/remove, and master-token fallback.

## Integration Coverage

Implemented bus and simulator integration suites:

- `test_bus_lifecycle.c`: `indigo_start()` / `indigo_stop()`, client and device attach/detach, enumeration, property definition/update/delete delivery, change routing, and invalid lifecycle calls.
- `test_ccd_simulator.c`: CCD simulator driver metadata, imager device lifecycle, full visible/hidden imager property sets, connection/disconnection, short exposure checks, and compliance checks for exposed imager, wheel, focuser, guider camera, guider, AO, Bahtinov camera, DSLR, and file-camera devices.
- `test_dome_baader_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Baader Classic Dome driver.
- `test_dome_beaver_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome Beaver driver, including azimuth motion, shutter, park/home, calibration, abort, and Beaver safety/failure properties.
- `test_dome_nexdome_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome driver, including shutter, azimuth motion, abort, reverse-direction, and NexDome status/control properties.
- `test_dome_nexdome3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the NexDome 3 firmware driver, including shutter, azimuth motion, abort, slaving threshold, and NexDome 3 status/settings properties.
- `test_dome_skyroof_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Interactive Astronomy SkyRoof driver, including shutter, abort, and heater control.
- `test_dome_talon6ror_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Talon6 ROR driver, including roof open/close/abort handling and Talon6 sensor/configuration properties.
- `test_dome_simulator.c`: dome simulator metadata, lifecycle, property enumeration, connection/disconnection, visible and hidden properties, dome property item/range checks, shutter/slaving/park commands, azimuth motion, and abort handling.
- `test_gps_simulator.c`: GPS simulator metadata, lifecycle, property enumeration, connection/disconnection, GPS property item/range checks, status lights, and advanced-status coverage.
- `test_gps_nmea_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free NMEA 0183 stream parsing for the Generic NMEA GPS driver, including selected positioning system, coordinates, UTC time, 3D fix status, satellite counts, and advanced DOP/status updates.
- `test_mount_ioptron_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, one configurable iOptron protocol simulator covering HC 8406, HC 8407, protocol 1.0, 2.0, 2.5, and 3.0 dialect connection paths, plus protocol 3.0 mount and guider property/action coverage.
- `test_mount_lx200_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, configurable LX200/OnStep protocol simulator coverage for the LX200 mount, guider, focuser, and OnStep AUX logical devices.
- `test_mount_nexstar_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, configurable NexStar protocol simulator coverage for both Celestron and Sky-Watcher dialect connection paths, plus Celestron mount and guider property/action coverage.
- `test_mount_nexstaraux_simulator.c`: external TCP simulator launch, ready-file discovery using a `nexstar://` loopback URL, and hardware-free NexStar AUX binary protocol coverage for mount and guider logical devices.
- `test_mount_pmc8_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, optional loopback TCP/UDP simulator endpoints, and hardware-free PMC-Eight protocol coverage for mount and guider logical devices, including disconnected connection-mode changes, serial/TCP/UDP mount connection paths, disconnected default `MOUNT_TYPE=AUTO`, `MOUNT_TYPE=AUTO` autodetection, manual mount-type override, basic mount operations, abort during coordinate tracking, manual RA/DEC motion, and guider pulses on both axes.
- `test_mount_rainbow_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free RainbowAstro serial protocol coverage for mount lifecycle, identity, guide-rate, tracking, track-rate, coordinate, and abort paths.
- `test_mount_simulator.c`: mount simulator metadata, main mount and guider-device lifecycle, property enumeration, connection/disconnection, mount compliance checks, and guider compliance checks.
- `test_mount_starbook_simulator.c`: external loopback HTTP simulator launch, ready-file discovery, and hardware-free Vixen StarBook protocol coverage for mount and guider logical devices.
- `test_mount_temma_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free Takahashi Temma serial protocol coverage for mount and guider logical devices.
- `test_polaralign_simulator.c`: polar aligner simulator metadata, lifecycle, property enumeration, connection/disconnection, property item/range checks, direction commands, offset no-op handling, reset commands, and abort command handling.
- `test_rotator_simulator.c`: rotator simulator metadata, lifecycle, property enumeration, connection/disconnection, direction reversal position mapping, shortest-path movement across 0/360, and rotator compliance checks.
- `test_rotator_optec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free Optec Pyxis rotator serial protocol coverage for direction, absolute position, home, rate, and relative rotate controls.
- `test_rotator_wa_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, and hardware-free WandererAstro rotator serial protocol coverage for direction, backlash, absolute/sync/relative position, zero position, and abort controls.
- `test_focuser_astromechanics_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the ASTROMECHANICS focuser driver, including position/aperture property coverage and the aperture command path.
- `test_focuser_askar_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the Askar-WAF focuser driver.
- `test_focuser_efa_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Celestron / PlaneWave EFA focuser driver, including position sync/goto, relative steps, fan control, temperature, and abort.
- `test_focuser_dmfc_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the PegasusAstro DMFC focuser driver, including speed, backlash, reverse motion, motor type, encoder/LED controls, position sync/goto, relative steps, temperature, and abort.
- `test_focuser_dsd_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Deep Sky Dad AF focuser driver, including AF2 speed, backlash, reverse motion, step mode, coils mode, current/timing controls, temperature, position sync/goto, relative steps, and abort.
- `test_focuser_focusdreampro_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the AstroGadget FocusDreamPro focuser driver, including speed, duty cycle, position sync/goto, relative steps, temperature, and abort.
- `test_focuser_ioptron_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the iOptron iEAF focuser driver, including status polling, reverse motion, absolute and relative moves, abort, zero sync, and temperature.
- `test_rotator_falcon2_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the Falcon2 rotator driver, including that a goto completes only once the rotator is idle and that a goto to the current position finishes instead of staying busy.
- `test_wheel_xagyl_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Xagyl filter wheel driver.
- `test_wheel_indigo_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Pegasus Indigo filter wheel driver.
- `test_wheel_quantum_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Brightstar Quantum filter wheel driver.
- `test_wheel_trutek_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Trutek filter wheel driver.
- `test_wheel_qhy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the QHY CFW1, CFW2, and CFW3 filter wheel driver modes.
- `test_wheel_optec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, wheel property enumeration completeness, and compliance checks for the Optec filter wheel driver.
- `test_focuser_fc3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the FocusCube 3 focuser driver.
- `test_focuser_qhy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the QHY Q-Focuser driver, including mode/compensation settings, position sync/goto, relative moves, speed, reverse motion, temperature, limits, and abort.
- `test_focuser_optecfl_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for both Optec FocusLynx focuser channels, including focuser type, reverse motion, position sync/goto, relative moves, temperature, and abort.
- `test_focuser_lacerta_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the LACERTA Motorfocus focuser driver.
- `test_focuser_lakeside_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the LakesideAstro focuser driver, including temperature polling, relative moves, abort, backlash, mode, compensation settings, and active slope.
- `test_focuser_mjkzz_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and binary-frame serial compliance checks for the MJKZZ Rail focuser driver, including speed, absolute position, relative movement, and abort.
- `test_focuser_moonlite_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the MoonLite focuser driver, including speed, stepping mode, compensation, mode, temperature polling, absolute and relative moves, and abort.
- `test_focuser_nfocus_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Rigel Systems nFOCUS focuser driver, including speed, temperature, relative moves, and abort.
- `test_focuser_nstep_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Rigel Systems nSTEP focuser driver, including speed, stepping mode, phase wiring, backlash, compensation mode, temperature, relative moves with position polling, and abort.
- `test_focuser_optec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and serial compliance checks for the Optec TCF-S focuser driver, including manual/automatic mode changes, compensation, reverse-motion state, temperature, and relative moves with position polling.
- `test_focuser_primaluce_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free JSON serial driver connection, and per-device compliance for both PrimaLuceLab logical devices — the focuser device (configuration/state/WiFi/LED/preset/hold-current controls, backlash, speed, absolute and relative moves, temperature, and abort) and the connection-sharing rotator device (property coverage and abort).
- `test_focuser_prodigy_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for both PegasusAstro Prodigy logical devices — the focuser device (speed, backlash, park, temperature, position sync/goto, relative moves, and abort) and the powerbox device (power outlets, USB ports, outlet naming, and reboot).
- `test_focuser_robofocus_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free fixed-frame serial driver connection, and serial compliance checks for the RoboFocus driver, including power channels, configuration/backlash, temperature, limits, absolute and relative moves, reverse motion, and abort.
- `test_focuser_steeldrive2_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free SteelDrive II serial driver connection with CRC responses, and per-device compliance for both logical devices — the focuser device (saved values, temperature-compensation controls, end-stop/zeroing controls, position sync/goto, relative moves, temperature, and abort) and the connection-sharing AUX heater device (heater output, auto-dew/PID controls, PID settings, and sensor selection).
- `test_focuser_usbv3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free compact-command serial driver connection, and serial compliance checks for the USB_Focus v3 driver, including step size, compensation, mode, speed, temperature, limits, absolute and relative moves, and abort.
- `test_focuser_wemacro_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free binary-frame serial driver connection, and serial compliance checks for the WeMacro Rail driver, including rail configuration, shutter fire, relative movement, batch execution, speed, reverse motion, and abort.
- `test_aux_svbpowerbox_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the SVBONY PowerBox AUX driver.
- `test_aux_wbplusv3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the WandererBox Plus V3 AUX driver.
- `test_aux_wbprov3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, class property enumeration completeness, and compliance checks for the WandererBox Pro V3 AUX driver.
- `test_aux_wcv4ec_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, lightbox property enumeration completeness, and compliance checks for the WandererCover V4-EC AUX lightbox driver.
- `test_aux_arteskyflat_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, lightbox property enumeration completeness, and compliance checks for the Artesky Flat Box AUX lightbox driver.
- `test_aux_astromechanics_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, SQM weather property enumeration completeness, and timer-driven `V#` reading compliance for the ASTROMECHANICS LPM AUX driver.
- `test_aux_fbc_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `: I #`/`: P #`/`: V #` handshake, lightbox property enumeration completeness, and light-intensity change compliance for the Lacerta FBC AUX driver.
- `test_aux_flatmaster_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `#`/`V` handshake, lightbox property enumeration completeness, and light switch and intensity change compliance for the Pegasus Astro FlatMaster AUX driver.
- `test_aux_flipflat_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `>POOO`/`>VOOO` handshake, lightbox and cover property enumeration completeness, and light, intensity, and timed cover-open (BUSY-to-OK via `>SOOO` status polling) compliance for the Optec/Alnitak Flip-Flat AUX driver.
- `test_ao_sx_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the character-framed `X`/`V` handshake, and per-device compliance for both logical devices of the StarlightXpress AO driver — the AO device (tip/tilt pulse and reset) and the connection-sharing guider device (guider pulse).
- `test_mount_synscan_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for the SynScan EQ8 driver's logical devices — the mount device (property completeness including `MOUNT_STATE`, representative changes, model-code identification, aux-encoder coordinate sourcing, capability-gated autohome property visibility, park completion after initialized axis status replies, coordinate updates and coordinate BUSY states during slew/park/home/autohome, home-state busy/completion after home/autohome, tracking stop after home, serial-loss disconnect, and tracking/state publication after coordinate slews that request tracking), the connection-sharing guider device (guide pulse), and the connection-sharing AUX shutter device (snap-port exposure and abort).
- `test_aux_upb3_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-device compliance for both logical devices of the Ultimate Powerbox 3 driver — the AUX device (power, USB, heater, dew control) and the connection-sharing focuser device (position move).
- `test_aux_ppb_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection, and per-model compliance for all three PegasusAstro Pocket Powerbox variants — PPB (power outlets, heater outlets, dew control), PPBA (same plus DSLR power selection), and SPB (single power outlet, heater outlets, dew control) — using the `--model ppb|ppba|spb` flag.
- `test_aux_skyalert_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `\r`-terminated `send` command, and property enumeration completeness for the Interactive Astronomy SkyAlert AUX weather driver.
- `test_aux_sqm_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `x`-terminated `ix` handshake, timer-driven `rx` reading compliance, and property enumeration completeness for the Unihedron SQM AUX driver.
- `test_aux_uch_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `P#`/`PV`/`PL:1`/`PA` handshake, USB port toggle compliance, and property enumeration completeness for the PegasusAstro USB Control Hub AUX driver.
- `test_aux_usbdp_simulator.c`: external pseudo-terminal simulator launch, ready-file discovery, hardware-free serial driver connection through the `SWHOIS`/`SGETAL` handshake (6-byte fixed-length commands, `\n`-terminated responses), and per-version compliance for both USB Dewpoint variants — V2 (three controllable heater outlets, dew control mode, calibration, thresholds, channel linking, aggressivity, weather, and dual temperature sensors) and V1 (weather and single temperature sensor only) — selected via `--model v1|v2`.

`integration/simulator_test_common.h` provides the shared in-process client, property cache, lifecycle helpers, and compliance-style assertions used by simulator tests.

## Compliance Model

The simulator compliance checks are derived from the shell-based routines in `indigo_tests/`. They verify interface bitmasks, mandatory class properties and items, numeric ranges, connection/disconnection paths, representative state transitions, and restoration of changed values where practical.

Coverage currently includes:

- GPS compliance on the GPS simulator.
- Mount compliance on the mount simulator.
- Guider compliance on the mount simulator guider and CCD simulator guider.
- Rotator compliance on the rotator simulator.
- Wheel, focuser, guider, and AO compliance through devices exposed by the CCD simulator.
- CCD camera compliance through the CCD simulator's imager, guider camera, Bahtinov camera, DSLR, and file-camera devices.
- Dome compliance on the dome simulator.
- Polar-aligner compliance on the polar-aligner simulator.
- External serial simulator compliance on Baader Classic Dome, NexDome Beaver, NexDome, NexDome 3, Interactive Astronomy SkyRoof, Talon6 ROR, Generic NMEA 0183 GPS, iOptron mount/guider, ASTROMECHANICS focuser, Askar-WAF focuser, Celestron / PlaneWave EFA focuser, AstroGadget FocusDreamPro focuser, Falcon2 rotator, Optec Pyxis rotator, WandererAstro rotator, Xagyl filter wheel, Pegasus Indigo filter wheel, Brightstar Quantum filter wheel, Trutek filter wheel, QHY CFW1/CFW2/CFW3 filter wheel modes, Optec filter wheel, FocusCube 3 focuser, QHY Q-Focuser, Optec FocusLynx focuser, Optec TCF-S focuser, PrimaLuceLab focuser/rotator, PegasusAstro Prodigy focuser/powerbox, RoboFocus focuser, SteelDrive II focuser/AUX, USB_Focus v3 focuser, WeMacro Rail focuser, LACERTA Motorfocus focuser, MoonLite focuser, myFocuserPro2 focuser, Rigel Systems nFOCUS focuser, Rigel Systems nSTEP focuser, SVBONY PowerBox AUX, WandererBox Plus V3 AUX, WandererBox Pro V3 AUX, WandererCover V4-EC AUX, Artesky Flat Box AUX, ASTROMECHANICS LPM AUX, Lacerta FBC AUX, Pegasus Astro FlatMaster AUX, Optec/Alnitak Flip-Flat AUX, StarlightXpress AO, SynScan EQ8 mount, Ultimate Powerbox 3 AUX, PegasusAstro Pocket Powerbox PPB/PPBA/SPB AUX, Interactive Astronomy SkyAlert AUX, Unihedron SQM AUX, PegasusAstro USB Control Hub AUX, and USB Dewpoint v1/v2 AUX drivers through pseudo terminals exported with `INDIGO_SIMULATOR_PORT`.

Standalone simulator archives are not present for every device class in this checkout, so some class coverage intentionally uses multi-device simulator drivers.

## Current Behavior

- The suite is hardware-free.
- Tests do not launch `indigo_server` or open network sockets.
- Protocol tests use fixed fixtures and temporary files through `indigo_uni_io`.
- Integration tests exercise public driver entry points and bus APIs; they do not include production `.c` files directly.

## Verification

Last verified locally on macOS:

- `make -C indigo_test test-unit`
- `make -C indigo_test test-integration`
- `make -C indigo_test test`
- `make -C indigo_test test-clean`

All tests passed at the time this document was cleaned up.

## Deferred Work

- Add binary fixtures for `indigo_dslr_raw.c`, `indigo_fits.c`, `indigo_tiff.c`, `indigo_avi.c`, and `indigo_ser.c`.
- Add carefully sourced expected values for `indigocat` coordinate, precession, and time transforms.
- Add slower live `indigo_server` socket tests with process management, port allocation, and strict timeouts.
- Add driver-specific fake I/O tests for hardware drivers with complex parsing or command sequencing.
- Wire a root-level `make test` target once the suite is accepted as part of the normal project workflow.
- Update `README.md` and `TESTING.md` with the finalized automated test commands and their relationship to manual hardware testing.

## 2026-09-07 focuser_asi refactoring verification

- Added `integration/test_focuser_asi_sdk.c` to the integration suite. It compiles the generated driver separately, exercises the public driver entry point and bus APIs, and replaces SDK calls, USB events and hardware locks without linking the vendor SDK or accessing USB hardware.
- Six regression groups cover failed initialization cleanup and lock release; confirmed position/step limits and failed limit/backlash readback; termination of failed polling and SDK preflight before retry; abort switch reset and delayed stop confirmation; accepting compensation settings, retaining the temperature baseline and recovery after a transient move error; five devices plus retry after failed attach and capacity exhaustion.
- Narrow run: `make -C indigo_test build/integration/test_focuser_asi_sdk` then `indigo_test/build/integration/test_focuser_asi_sdk`. All six groups passed on macOS. Fresh x86_64/arm64 compilation and archive/dynamic-library/executable linking with the real SDK also passed; existing vendor deployment-target warnings remain.
- Real EAF firmware timing, physical USB enumeration order, hand-controller behavior and end-to-end configuration persistence still require hardware validation.

## 2026-09-07 — ASI EFW hot-plug regression

Added `integration/test_wheel_asi_sdk.c` for DRV-051. It compiles the production driver separately with SDK/USB stubs and exercises its public lifecycle. It verifies retry after a failed attach and after exhausting all five slots then freeing one, without reinitializing the driver. No vendor SDK or hardware is needed for this test. The regression fails on the original driver at failed-attach retry and passes with the fix.

Run `make -C indigo_test build/integration/test_wheel_asi_sdk` then `indigo_test/build/integration/test_wheel_asi_sdk`. Real USB enumeration and vendor SDK behavior still require hardware validation.

The ASI EFW SDK suite also covers connection, changing slot 1 to slot 3, and disconnection through the public property API. It checks SDK open/close calls, lock acquisition/release, conversion from INDIGO slot 3 to SDK index 2, and the slot property's BUSY-to-OK transition with the confirmed final value. Both integration cases passed with the SDK stubs.

## 2026-09-07 — ASI EAF functional scenarios

Extended `integration/test_focuser_asi_sdk.c` with four public-bus functional cases: connect/absolute move/disconnect, relative moves in both directions, position synchronization without an SDK move, and limits/backlash/reverse/beep settings with readback after reconnect. Stateful SDK stubs record requested positions and settings. The tests verify SDK open/close and lock release, motion BUSY-to-OK completion, and confirmed positions. Existing abort and compensation regressions remain in the same suite (ten cases total).

Run `make -C indigo_test build/integration/test_focuser_asi_sdk` followed by `indigo_test/build/integration/test_focuser_asi_sdk`. These tests require no hardware; physical motion and vendor SDK behavior are not covered.

## 2026-09-07 — ASI EFW SDK error regressions

The wheel SDK suite now has five cases. Added required initialization read failures and oversized SDK slot counts with close/lock-release checks; motion polling failure preserving the confirmed slot and stopping retries; calibration read failure/reset/retry; START=false completion; and interrupted calibration followed by reconnect while the wheel is still moving. Existing normal motion and attach/capacity retry cases remain. All five cases passed, and production universal compilation/linking passed separately.

## 2026-09-07 — ASI CAA hardware-free regression (rotator refactoring Step 8)

Added `integration/test_rotator_asi_sdk.c` and registered `test_rotator_asi_sdk` in `INTEGRATION_TESTS`, including the standard `test-integration` and `test` targets. As with EAF/EFW, the generated production C is compiled into a separate test object with local preprocessor replacements for USB discovery, attach/detach and global locks. Stateful CAA functions are supplied by the test executable, which links the real INDIGO library without the vendor CAA binary. No production source or generator changes are needed.

Each of the 11 named scenarios starts a fresh driver lifecycle and uses public driver/bus APIs, `test_runner.h` and `simulator_test_common.h`. Cleanup runs even after a failed assertion. Per-device counters detect SDK calls on closed handles, duplicate opens, unbalanced global locks and leaked USB references; successful SDK opens and closes must balance after every scenario.

Implemented coverage:

- Metadata/version, interface, custom schemas and connected-only visibility, hidden backlash, SDK version and repeated connect/disconnect.
- Failed SDK open and all four required connection reads (maximum, position, reverse, beep), followed by a successful retry; failed initialization must leave CONNECTION alert/disconnected and release handles/locks.
- Fractional absolute and signed relative motion, separate current value/target, overlapping requests, computed relative targets outside effective limits, and continued BUSY while the SDK motor flag remains active even at the target angle.
- Failed move start, status/position polling failures, preservation of confirmed position, termination of failed polling and safe retry while the motor remains active.
- Sync command failure, failure of readback after successful sync, and successful sync without a move command.
- Failed stop, delayed motor-stop confirmation, abort switch reset and hand-controller movement that cannot be stopped through the SDK.
- Maximum writes and failed write/readback consistency, effective motion-limit rejection, reverse/beep writes and failures, and settings readback on reconnect.
- Beep persistence selection through CONFIG_SAVE. Test-local save/base-dispatch hooks observe the driver's save request and bypass base CONFIG handling to avoid writing user configuration; all other base property handling delegates to the real rotator base driver. This checks selection for persistence, not disk serialization.
- Empty/eight-byte suffixes, overlength rejection without an SDK write, failed-write rollback, and the resulting device name after replug.
- Disconnect/reconnect and connected unplug with pending movement polling; no further SDK polling after close. Wrong USB vendor/product, failed/invalid SDK IDs, failed open/property probes, failed attach/retry, duplicate arrival, default five-device capacity, slot reuse, and removal of one connected device while another stays open.
- Unplug queued behind a deliberately held SDK probe, using a bounded gate; normal driver shutdown and resource balance after every scenario.

Validation: `make -C indigo_test build/integration/test_rotator_asi_sdk` compiled the test and separate production object for x86_64/arm64. `indigo_test/build/integration/test_rotator_asi_sdk` passed all 11 scenarios on native macOS arm64. Expected SDK error logs are from injected failures. The final run includes suffix replug and motor-active-at-target checks. Dynamic dependencies contain INDIGO/libusb and system libraries, with no CAA vendor binary. `git diff --check` passed; `make -C indigo_test test-clean` removes the generated test artifacts.

Remaining coverage: forced overlap of an already executing device handler with disconnect, shutdown with queued/in-flight hot-plug work, USB callback registration/queue allocation failures, invalid startup product-count responses, and vendor timing/physical USB-to-SDK identity. The connection helper waits for the initial delayed SDK read before issuing ordinary command assertions; commands racing that initial read are not covered. No physical CAA, Linux execution, x86_64 execution or full-suite run was performed in this step. Hardware validation remains refactoring Step 9.

Player One follow-up validation (2026-09-08): all 13 cases also passed with `POA_SAFE_READOUT=1` in an isolated build tree and with arm64 AddressSanitizer/UndefinedBehaviorSanitizer. The test, driver and test-local framework objects were instrumented; bundled static dependencies were not. After finalizer naming changes, pulse error was idle mean +1.928 ms / max +5.060 ms and exposure mean +2.684 ms / max +5.054 ms. Physical streaming unplug/replug, reacquisition, guider command and subsequent unload/reload passed separately (TESTING.md); electrical ST4 output remains unmeasured.

## AUX serial/network protocol tests (2026-09-09)

Added standalone PTY simulators for `aux_cloudwatcher` and `aux_mgbox`, and a loopback UDP simulator for `aux_dragonfly`, with public-bus tests linked to unchanged production drivers. New sources and the protocol coverage note are included in Xcode. Serial suites are in `test-integration`; `test-aux-dragonfly-simulator` is opt-in. No Linux-only or system-HID driver was included, and no production driver or generator was changed.

[Protocol sources, exact targets, fixture assumptions and remaining coverage](AUX_PROTOCOL_TESTS.md) distinguish manufacturer documentation from supplementary driver/existing simulator information. The existing Dragonfly Perl simulator was consulted for undocumented reply forms. Tests intentionally report current failures rather than treating bugs as expected passes.

Validation on macOS arm64: universal arm64/x86_64 test builds succeeded; 15 ordinary scenarios ran (9 pass, 6 fail). Four additional selected driver-instrumented AddressSanitizer scenarios reproduce CloudWatcher timeout underflow, Dragonfly UDP overflow and both MGBox truncated-message crashes. The framework library remains uninstrumented. See open `DRV-085`–`DRV-089` in `indigo_drivers/REVIEW.md`. These are partial coverage results, not hardware validation or full-standard completion.

## MGBox migration preparation (2026-09-09)

Rebuilt the unchanged `aux_mgbox` driver and existing PTY suite for macOS arm64/x86_64, then reran all five scenarios on native arm64. All five failed: Weather/calibration and GPS data assertions passed but disconnect/shutdown failed; both truncated-message scenarios crashed with SIGSEGV; invalid-port rejection and teardown failed. This reconfirms existing `DRV-088`/`DRV-089`; no production or test behavior was changed in this preparation step.

The migration sequence is recorded in `../indigo_drivers/aux_mgbox/REFACTOR.md`. The user identified missing PBox coverage: the simulator currently accepts pulse commands and stores a deadline, but there are no pulse command/duration/reset assertions and it always identifies as MGPBox. Planned coverage includes standalone PBox/model capability profiles, observable pulse commands, both shared connection orders, parser recovery, fix/advanced GPS status, forwarding, reboot, reconnect and pending-operation teardown. These remain planned, not implemented or passing. No new ASan, TCP or hardware run was performed.

## MGBox Step 2: Powerbox simulator coverage (2026-09-09)

Implemented the Powerbox subset of the preparation plan: `pbox` and `mbox` profiles alongside normal MGPBox, an optional flushed event journal, three independent PTY simulator checks and four public-bus driver scenarios. Powerbox tests cover outlet-label propagation, changing pulse length without activating the outlet, one exact `:pulse,1500*` command, timed completion and switch reset, rejection on MBox without a wire pulse, and PBox GPS-connection rejection. The manual confirms the MGPBox pulse command/units; standalone model capabilities and identity reply spelling remain explicitly supplementary fixtures. See [protocol details](AUX_PROTOCOL_TESTS.md) and `../indigo_drivers/aux_mgbox/REFACTOR.md` for the result table.

Universal macOS arm64/x86_64 build passed. Full native arm64 execution: 12 scenarios, 3 independent simulator checks pass and 9 driver scenarios fail. Powerbox label/pulse/reset and MBox no-pulse assertions pass, followed by the existing failed disconnect. PBox GPS rejection also cannot complete its connection rollback. The five original failure scenarios remain unchanged. No expected-failure masking, production driver fixes or generator changes were introduced. Added the driver archive as a narrow test prerequisite so future changes relink automatically.

After tightening the reset check to reject completion before the pulse duration (50 ms observation tolerance), rebuilt and reran the three `powerbox_` scenarios. Their control assertions passed again; all three still failed only at teardown. Shared connections, overlapping pulses and pending-operation cleanup, parser recovery/fix transitions, reboot/forwarding and TCP remain deferred to subsequent migration steps. No ASan or hardware validation was performed in this step; `DRV-088`/`DRV-089` remain Open.

## Guider simulator/SDK follow-up (2026-09-09)

Added `test_guider_asi_sdk` against the bundled vendor SDK contract and a standalone `guider_cgusbst4_simulator` with real-PTY integration tests. Re-ran the existing GPUSB fake SDK suite. New files are referenced in Xcode. Production `.c`/`.driver` sources and the generator were not changed.

[Protocol evidence, simulator profiles, run commands and remaining coverage](GUIDER_PROTOCOL_TESTS.md). ASI: 3/9 pass, six failing scenarios reproduce five bugs. CG-USB-ST4: 4/5 pass; remaining scenario establishes a dialect discrepancy pending manufacturer confirmation. GPUSB: all four existing groups pass. Findings `DRV-090`–`DRV-095` are Open in `indigo_drivers/REVIEW.md`; the folder's reviewed range was updated without advancing its baseline. macOS ASI validation uses x86_64/Rosetta because the driver does not implement the native arm64 branch.


## MGBox Steps 3–8: generated migration acceptance (2026-09-09)

Migrated production code to `indigo_aux_mgbox.driver`, regenerated C/header/main, and raised the version from `0x03000004` to `0x0300000A`. Intermediate lifetime fixes reduced the 12-case failures to the two parser crashes; bounded parsing then passed all 18 cases. Generated queue/lifecycle migration passed those 18 and subsequently 29 expanded cases. The final archive-linked native arm64 run passes all 32 scenarios.

New coverage includes both shared connection orders using the AUX master's port, rollback of failed GPS capability detection, invalid-port recovery and three reconnect cycles, silent identification, independent additional-instance PTYs, all calibration targets and no premature OK on stale readback, weather/GPS forwarding in both directions, missing-reply ALERT, both reboot controls, pulse overlap/conflicting reboot, disconnect during pulse/reboot, transport EOF, GPS fix loss/2D/3D recovery, advanced satellites/DOP visibility, signed coordinates/negative altitude, UTC year rollover and fragmented frames. Parser faults include truncated RMC/GGA/GSA/GSV/XDR/CAL/LOG, non-finite numbers, malformed/incorrect checksums, excess tokens and overlong input, followed by valid recovery data. Independent model and Powerbox pulse checks from Step 2 remain in the suite.

Final universal macOS production/test/ASan builds passed. Nine native arm64 ASan scenarios passed: `short_weather`, `short_gps`, `parser_weather`, `parser_gps`, `split_weather`, `split_gps`, `pulse_disconnect`, `reboot_disconnect` and `instances`. The driver/test are instrumented; the existing framework is not. The complete 32-case ordinary run and every selected ASan command exited 0. `DRV-088`/`DRV-089` are closed with this scoped evidence; review baselines are unchanged.

The simulator's physical pulse duration uses its 0.5-second tick and does not prove hardware timing. Reboot completion is the driver's two-second settling delay, not device acknowledgement. Standalone model fixtures retain the documented provenance limits. TCP bridges, real hardware/firmware variants, Linux/Windows and x86_64 execution remain deferred; this is not full GPS standard or hardware acceptance. Migration details and final cleanup/checks are in `../indigo_drivers/aux_mgbox/REFACTOR.md`.


## LACERTA migration preparation and simulator motion (2026-09-09)

Primary protocol source: `indigo_drivers/focuser_lacerta/Lacerta_Mfoc-Fmc_API_2024.xlsx`, sheet `Munka1` (backlash/debug rows 5–12, limits 17–18, halt/identity/motion 19–22, query/SYNC/reversal 25–29, temperature NC 32, firmware 35 and 46–49). The workbook is read directly; legacy driver/simulator framing remains supplementary evidence.

The unchanged API 2 production driver's existing smoke test passed after a universal macOS rebuild. Replaced the host simulator's background thread/mutex with shared elapsed-time `serial_motion` at 1000 steps/s (minimum nonzero duration 0.5 s). Added header dependencies and an archive prerequisite for reliable integration relinking.

| Coverage | Named test / status |
| --- | --- |
| Entire currently implemented simulator command subset, MFOC/FMC identity and selected firmware, settings write/readback, temperature, motion progression without queries, stop, SYNC during motion, no-op, both travel limits and short-move completion | `lacerta_simulator_mfoc_protocol`, `lacerta_simulator_fmc_protocol`; direct fixture validation, not production-driver compliance. |
| Production driver enumeration/connect, SYNC, backlash, reverse, maximum limit, absolute GOTO and normal cleanup | `lacerta_focuser_passes_serial_compliance_checks`; existing smoke coverage. |
| Debug/interleaved completion messages, split/short/overlong/malformed replies, initial query/write/poll/stop failures and recovery | Planned Step 1–3 regressions in `focuser_lacerta/REFACTOR.md`; not yet covered. |
| Production relative directions/reversal, no-op, actual limit boundaries, abort/new move, overlapping properties and externally changed position | Planned Step 3–4; not yet covered. |
| Firmware v1 range and temperature NC semantics, rollback/reconnect, pending-operation teardown and independent instances | Planned Step 2–6; not yet covered. |

Full applicable focuser coverage is an acceptance requirement for the migration. The three current cases alone do not meet it. Shared helper internals and generic framework validation are not duplicated in driver tests. Real FMC/MFOC timing, Linux/Windows and x86_64 runtime remain unverified.

Validation of this simulator change: macOS universal simulator/test builds and strict simulator syntax checks passed; both direct protocol cases and the unchanged production smoke case passed on native arm64. A CR-trimming error in the first direct-test observer was corrected before the successful rerun. Standalone simulator build passed. Test artifacts were cleaned; ASan and full production regression coverage remain planned.


## LACERTA generated migration: final coverage (2026-09-09)

This supersedes the preparation coverage gaps above. Tests exercise the production driver through public bus requests with real handler queues. Fresh property revisions distinguish completion from cached OK. Each named scenario gets its own simulator and watchdog; the parent reaps both PTYs in the instance case. The unchanged generator emits the authoritative DSL's C/header/main.

| Applicable focuser/common standard area | Named scenarios |
| --- | --- |
| Independent protocol fixture, MFOC/FMC commands, timed movement and asynchronous completion | `simulator_mfoc`, `simulator_fmc` |
| Interface, connection battery, visible/hidden properties, metadata, firmware v1/v2/v3 ranges, setting readback and SYNC without M | `normal`, `capabilities_mfoc`, `capabilities_fmc`, `capabilities_mfoc2`, `debug`, `split` |
| Absolute/relative moves, direction and reversal combinations, target/measurement, zero/no-op, both clipped boundaries, overlapping POSITION/STEPS in both orders | `normal`, `noop`, `relative`, `overlap` |
| Abort active/idle, stopped readback, switch reset, restart, rejected stop, lost transport, poll failure and stalled movement | `abort`, `stop_failure`, `transport_loss`, `motion_poll_failure`, `stalled_motion` |
| Backlash/reversal/limits/SYNC failure and recovery; settings conflicts during movement | `backlash_failure`, `limits_failure`, `reverse_failure`, `sync_failure`, `settings_during_motion` |
| Malformed, short, overlong, missing, unterminated and flooding replies with bounded recovery | `poll_malformed`, `poll_short`, `poll_overlong`, `poll_silent`, `poll_partial`, `poll_flood` |
| Temperature NC at connection/runtime, invalid response, recovery, external hand-controller position | `sensor_absent`, `temperature`, `external_position` |
| Failed open/initialization and descriptor rollback, invalid port, repeat reconnect and device setting persistence | `init_version`, `init_reverse`, `init_limits`, `init_position`, `init_backlash`, `unknown_identity`, `short_identity`, `overlong_identity`, `silent_identity`, `reconnect` |
| Disconnect during motion/read, no commands while closed, independent ports/position/temperature and surviving peer | `disconnect_motion`, `disconnect_read`, `instances` |

Speed, automatic mode, temperature compensation, motor current, beep/heater and home are not exposed by this driver and are non-applicable. Generic range validation/configuration storage is framework scope. M has no immediate protocol acknowledgement: transport loss and ignored movement are tested through actual polling, without inventing an ACK. Deterministic partial OS writes are not directly injected by a PTY; production checks the full write count. Hardware motor timing, mechanical safety/travel and physical sensor behavior require the small-travel hardware acceptance described in the class standard. No claim of exhaustive C branch coverage or hardware certification is made.

Reproduce from `indigo_test`: `make build/integration/test_focuser_lacerta_simulator build/integration/test_focuser_lacerta_simulator_asan`, then `./build/integration/test_focuser_lacerta_simulator`. `LACERTA_TEST_FILTER` selects scenario-name substrings, including `identity`, `init_`, `disconnect`, `instances` and `poll_` for targeted ASan. ASan instruments the driver and test executable; the framework library is the normal build.

Final result: 43/43 ordinary scenarios and 19/19 targeted ASan scenarios passed on macOS arm64. Universal driver/test build and strict driver warning checks passed for arm64/x86_64. See driver `REFACTOR.md` for intermediate failures, fixes and platform limits.


## iOptron focuser generated migration (2026-09-09)

Production `indigo_focuser_ioptron.driver` replaces handwritten lifecycle/timers with generated handler queues. Simulator motion now uses shared serial_motion and advances without queries. Protocol provenance and hardware assumptions are in driver REFACTOR.md: manufacturer manual documents functions but no independent wire specification was available. Fixed-width identity/status and direction polarity follow the existing implementation. Tests use public bus requests, actual production queues and isolated parent-owned PTYs; fresh revisions distinguish completion from cached OK. The custom property is now X_FOCUSER_ZERO_SYNC, replacing ZERO_SYNC.

| Applicable common/focuser matrix | Named tests |
| --- | --- |
| Independent simulator command grammar, model 2/3, status, reversal, timed progress, halt, zero during motion, no-op, completed short move without polling | `simulator_ieaf`, `simulator_iafs` |
| Interface, connect-scoped properties/custom rename, hidden unsupported controls, metadata/ranges, absolute GOTO, wire command and BUSY/completion | `normal`, `iafs`, `split` |
| Relative inward/outward in both reversal states, zero/no-op, lower/upper clipping, external coordinate changes | `movement` |
| Zero coordinate update without FM, momentary switch reset/false request, fresh move | `zero` |
| Abort moving/idle, actual stopped target, switch reset, fresh move | `abort` |
| Duplicate and cross-property POSITION/STEPS requests both orders, zero/reverse while moving, pending zero versus new move | `overlap`, `pending_control` |
| Ignored commands and lost readback with recovery; avoid toggling reversal twice after lost reply | `zero_failure`, `reverse_failure`, `stop_failure`, `zero_readback_failure`, `reverse_readback_failure`, `abort_readback_failure` |
| Ignored move, transport loss, moving read failure, bounded stalled-motion handling and restart | `start_failure`, `transport_loss`, `motion_read_failure`, `stalled_motion` |
| Malformed, short, overlong, unterminated, silent, invalid movement/direction, impossible position, trailing data; bounded failure and recovery | `poll_malformed`, `poll_short`, `poll_overlong`, `poll_partial`, `poll_silent`, `poll_badflag`, `poll_baddir`, `poll_badpos`, `poll_trailing` |
| Kelvin-to-Celsius conversion, negative/invalid temperature and recovery without overwriting confirmed values | `temperature` |
| Invalid port, reconnect, pending motion/read disconnect, no traffic after close and peer-independent ports/model/position/temperature | `reconnect`, `disconnect_motion`, `disconnect_read`, `instances` |
| Unknown identity, short/overlong/partial/silent identity, required initial status failure, descriptor rollback and successful retry | `unknown_identity`, `init_identity_short`, `init_identity_overlong`, `init_identity_partial`, `init_identity_silent`, `init_status` |

Non-applicable: arbitrary position SYNC, speed, backlash, configurable limits, automatic mode/compensation, beep/heater/current/home are not exposed. Relative sign remains coordinate-relative irrespective of physical reversal, matching the legacy driver. Generic input validation/configuration storage is framework scope. PTYs cannot deterministically force partial OS writes; the driver checks exact write length. No ACK is invented for write-only commands. The driver/test ASan executable uses the normal framework library. No physical travel/timing, Linux/Windows or x86_64 runtime claim is made. Hardware acceptance is the class-standard small-travel move, reverse, zero, abort/settings/reconnect check; validate documented wire assumptions on each available model.

Reproduce from `indigo_test`: `make build/integration/test_focuser_ioptron_simulator build/integration/test_focuser_ioptron_simulator_asan`; run `./build/integration/test_focuser_ioptron_simulator`. `IOPTRON_TEST_FILTER` selects scenario-name substrings, e.g. `readback_failure`, `init_`, `poll_`, `instances`, `disconnect` for ASan. Baseline original smoke passed; new `init_status` and `poll_badflag` failed on the handwritten driver and pass after migration. Final totals are recorded in driver REFACTOR.md after the full run.

Final iOptron result: 40/40 ordinary and 20/20 targeted driver-instrumented ASan scenarios passed on macOS arm64. Universal build and expanded O0/O2 strict warning checks passed for both architectures. Test artifacts were cleaned. See driver REFACTOR.md for exact commands and platform/protocol limitations.


## EFA generated migration (2026-09-09)

Authoritative `.driver` with real generated queues, handler + finalizer motion/calibration, portable serial I/O and a shared serial_motion simulator. Primary PlaneWave protocol is the bundled PDF (pages 1–7); Celestron extensions and two-byte temperature compatibility have explicit provenance limitations in driver REFACTOR.md. Public-bus tests use fresh property revisions, parent-owned PTYs, descriptor rollback checks and a watchdog. The calibration timeout case allows 220 seconds to validate the actual 180-second driver deadline, without test-only timing hooks.

| Applicable common/focuser standard | Named scenarios |
| --- | --- |
| Independent binary simulator protocol, model firmware, position, elapsed-time motion without polling, stop, SYNC, fan/temperature and calibration | `simulator_efa`, `simulator_celestron` |
| Interface, connect-scoped model properties, INFO, ranges/permissions, unsupported hidden controls, GOTO/SYNC, fan writes/readback and BUSY/completion | `normal`, `celestron`, `split`, `echo`, `uncalibrated_efa`, `uncalibrated_celestron` |
| No-op/zero relative, inward/outward, coordinate units, both boundaries and external signed position | `movement_efa`, `movement_celestron`, `limits`, `external_position` |
| Positive/negative coarse slew, stop/fine transition and measured final target | `long_move` |
| Abort idle/moving on both models, actual stopped target, reset switch and fresh movement | `abort_efa`, `abort_celestron` |
| Duplicate/cross-property movement both orders, changed limits while busy, calibration versus motion | `overlap`, `calibration_abort` |
| Positive/negative/zero temperature, NC and recovery; three-byte and legacy two-byte shape | `temperature`, `temperature_legacy` |
| Checksum/short/overlong/partial/missing/source/destination/command errors with bounded recovery and preserved measurement | `poll_checksum`, `poll_short`, `poll_overlong`, `poll_partial`, `poll_silent`, `poll_wrongsrc`, `poll_wrongdst`, `poll_wrongcmd` |
| Fan/SYNC/stop/start rejection, transport loss, motion read errors, invalid motion state and stall recovery | `fans_failure`, `sync_failure`, `stop_failure`, `start_failure`, `transport_loss`, `motion_read_failure`, `motion_badstate`, `stalled_motion` |
| Celestron calibration success, start/controller/read/limit failure, abort, real deadline and subsequent calibration/motion | `calibration`, `calibration_start_failure`, `calibration_failed`, `calibration_read_failure`, `calibration_limits_failure`, `calibration_abort`, `calibration_timeout` |
| Disconnect during motion/read/calibration, no traffic after close, reconnect and independent EFA/Celestron ports/metadata/position | `disconnect_motion`, `disconnect_read`, `disconnect_calibration`, `instances` |
| Unknown model, bad initial checksum/length/silent identity, required position/fan/calibration/stopdetect/limits initialization and descriptor rollback/retry | `unknown_identity`, `init_checksum`, `init_overlong`, `init_short`, `init_silent`, `init_position`, `init_fans`, `init_calibration`, `init_stopdetect`, `init_celestron_limits` |

Non-applicable: speed, backlash, reversal, automatic compensation, arbitrary Celestron SYNC and AUX-port transport are not exposed. Local software limit changes do not invent a hardware minimum command. Generic request validation/configuration persistence is framework scope. PTYs do not force partial OS writes (exact count is checked) and cannot validate electrical CTS/RTS; unsupported modem-line fallback is exercised. Actual hardware variants, mechanical calibration/travel and Linux/Windows/x86_64 execution remain separate acceptance. ASan instruments production driver C and test, with a normal framework library.

Reproduce from `indigo_test`: `make build/integration/test_focuser_efa_simulator build/integration/test_focuser_efa_simulator_asan`, then run `./build/integration/test_focuser_efa_simulator`. `EFA_TEST_FILTER` selects name substrings. Targeted ASan filters: `init_`, `poll_`, `calibration_abort`, `calibration_read_failure`, `disconnect`, `instances` (23 distinct cases). `calibration_timeout` intentionally waits for the real deadline and is part of the ordinary full suite. Baseline bad-checksum acceptance and ASan response-buffer overflow were reproduced before migration. The two direct protocol scenarios also probe CTS on the PTY before data exchange: unsupported line status must not latch a data-I/O error. Final validation is recorded in REFACTOR.md.

Final EFA result: 55/55 ordinary and 23/23 targeted driver-instrumented ASan scenarios passed on macOS arm64, including the real calibration deadline. Universal library/driver/test builds and strict O0/O2 driver warning checks for arm64/x86_64 passed. Generation is reproducible and Xcode/Windows project structure validated; Linux, Windows execution and electrical CTS/RTS remain unverified. See REFACTOR.md for intermediate regressions and hardware assumptions.

## Prodigy generated migration (2026-09-09)

Production DSL uses generated master/slave queues and shared serial lifetime, private receive buffer, preferred uniform I/O, motion/park finalizer and reboot finalizer. Protocol fixtures follow the bundled one-page Prodigy serial command table: the second power output is boolean, speed is read via B, and Z is encoder-zero motion. Simulator movement uses shared serial_motion independently of polling. Test parents own PTYs, isolated journals/fault files, forked scenarios and watchdogs; public bus helpers use fresh property revisions and explicitly await momentary-switch reset after the framework's initial BUSY request.

| Applicable behavior | Scenarios |
| --- | --- |
| Independent simulator wire/elapsed-time movement/second power output | protocol |
| Interface/model/settings/visibility, split transport and dotted firmware version | capabilities, split, firmware_minor |
| GOTO/SYNC/relative signs, measured versus target, zero/no-op, boundaries/local limits | movement, limits |
| Overlap in both motion entry directions and park rejection while busy | overlap |
| Abort idle/moving, stop failure and subsequent recovery | abort, stop_failure |
| Encoder-zero park, BUSY/completion, stop and failure | park, park_abort, park_failure |
| Rejected/ignored start, stalled motion, read/bad-state failure, transport loss | start_failure, ignored_start, stalled_motion, motion_read_failure, motion_badstate, transport_loss |
| SYNC/settings rejection/readback and value preservation | sync_failure, speed_failure, speed_readback, backlash_failure |
| Bad position/status, partial/overlong/silent replies, invalid temperature and recovery | poll_bad_position, poll_bad_state, poll_partial, poll_overlong, poll_silent, poll_temperature_nan |
| External signed position and negative/zero temperature | external |
| Four independent port controls, partial writes, malformed readback and labels before connection | powerbox, power_first_failure, power_second_failure, usb_first_failure, usb_second_failure, ports_read_failure, labels |
| Slave-first connection, no second descriptor/open, peer-preserving rollback and either disconnect order | shared, shared_failure |
| Reboot BUSY/reset/completion, timeout and rejection during motion | reboot, reboot_timeout, reboot_busy |
| Disconnect during motion/read/park/reboot, no later traffic and reconnect | disconnect_motion, disconnect_read, disconnect_park, disconnect_reboot |
| Separate ports/state and peer operation across base disconnect | instances |
| Identity, aggregate/status shape, oversized/missing replies, speed and port initialization, descriptor rollback/retry | init_identity, init_status, init_short, init_overlong, init_silent, init_speed, init_ports |

53 ordinary scenarios. Targeted ASan filters: init_, poll_, shared, disconnect, instances, transport_loss, park_abort, reboot_timeout (23 distinct scenarios). Build `make -C indigo_test build/integration/test_focuser_prodigy_simulator build/integration/test_focuser_prodigy_simulator_asan`; run from indigo_test, using PRODIGY_TEST_FILTER for substrings. The direct protocol test uses read_section2 and explicitly validates its retained terminator. Final outcomes are recorded in driver REFACTOR.md. Baseline original smoke passed; init_identity fails on original C/header because it accepts OK_OTHER.

No reverse/automatic compensation support is invented. Local limits are software-only; physical direction/encoder travel, port power and firmware reboot timing need hardware. Generic range validation/config persistence and focus quality are outside driver acceptance. Windows project structure is validated, not Windows execution.

## Lacerta and iOptron preferred-I/O follow-up (2026-09-09)

Both production DSLs now use shared private receive buffers, indigo_uni_discard, indigo_uni_printf and indigo_uni_read_section2 instead of custom byte/drain loops. Complete CR/# termination and existing protocol validation remain required; missing/truncated/NUL replies fail. Lacerta retains bounded asynchronous D/M/p frame skipping. Both versions advance from 0x03000005 to 0x03000006. Existing full simulator suites and targeted ASan parser/initialization/lifecycle/readback cases are rerun; final results are recorded in their REFACTOR.md files. No simulator/protocol capability change or framework validation duplication.

Follow-up refinement: all three command helpers are variadic, forwarding through uni_vprintf/uni_vtprintf. iOptron readback failures reproduced the shared uni_discard serial implementation incorrectly flushing TX as well as RX; fixed it to the documented input-only operation (TCIFLUSH/PURGE_RXCLEAR). The zero/reverse/abort readback scenarios verify commands survive an immediate following query. Prodigy FOCUSER_POSITION now uses preserve_values so generated BUSY requests retain the measured coordinate. Full suites and selected ASan are rerun with these final behaviors.

Final preferred-I/O follow-up results: Lacerta 43/43 ordinary + 19/19 ASan; iOptron 40/40 ordinary + 20/20 ASan. Both pass strict O0/O2 arm64/x86_64 warning checks and reproduce generated output exactly. Lacerta also passes normal and ASan motion_poll_failure after its warning-only local-initialization refinement.

Final Prodigy acceptance: 53/53 ordinary simulator cases and 23/23 targeted ASan cases pass with variadic I/O, input-only discard and measured-position preservation. Strict universal warning checks, reproducible generation and project validation pass. Full results, intermediate failures/fixes and remaining hardware/platform limits are recorded in REFACTOR.md.

## Shared background queue and CCD countdown (2026-09-09)

Framework coverage is in `integration/test_ccd_countdown.c`, linked against libindigo: both cameras continue countdown while one device mutex is held; a generic background callback also executes; an overdue countdown reaches zero and clears its deadline without changing BUSY or target; a driver-supplied zero terminates countdown; suspend/resume and subsecond replacement work; pending countdowns are canceled before detach; another camera survives detach; the queue is recreated after a complete bus stop/start cycle. Snapshot callbacks observe state on the same background queue.

`simulator_countdown_progress_abort_and_restart` in `integration/test_ccd_simulator.c` observes intermediate BUSY exposure updates from a real three-second simulator exposure, completion at zero, abort, a subsequent exposure and reconnect. The complete CCD simulator suite covers its other existing exposure, streaming, imaging and logical-device scenarios. Existing timer unit tests cover priority ordering and queue cancellation. This is framework work, not a driver migration or a claim of new hardware coverage. No properties were added or removed. Monotonic clock behavior uses POSIX CLOCK_MONOTONIC and Windows QueryPerformanceCounter; Windows execution and actual wall-clock adjustment are not exercised on the macOS test host.

Validation: rebuilt libindigo on macOS (arm64/x86_64); both countdown lifecycle cases, the complete CCD simulator suite, timer unit suite and bus lifecycle integration suite passed. `git diff --check` passed. No new mutexes or device locks are introduced by the final implementation.

## URGENT abort dispatch in generated drivers (2026-09-10)

Scope: 27 asynchronous abort branches in 25 generated drivers use URGENT priority. Hand-written drivers, explicit synchronous abort (including rotator_falcon), versions and property definitions are unchanged by user scope. No new production mutex or lock is introduced. This is not a migration or hardware acceptance.

The driver abort blocks cancel their associated pending starts under the existing operation/abort guards. This matters because property BUSY is published before the start is queued. Priority alone initially failed the existing dome/rotator compliance tests: abort overtook a pending start, which subsequently restarted motion. Moving cancellation outside existing switch guards was also incorrect; false abort requests must preserve the driver's previous behavior.

Cancellation before hardware start must terminate properties without relying on a callback that the canceled start would have scheduled. The changes handle that case for Player One, SkyRoof, dome simulator, Primaluce, USBv3, EFA calibration, Prodigy park and mount park/home operations. Existing unconditional abort semantics remain unconditional in drivers without a switch guard.

Coverage mapping:
- `test_generator_architecture`: generates all six standard abort properties and checks URGENT dispatch, explicit synchronous dispatch, normal-property and guiding controls.
- `test_timer`: holds the worker, queues NORMAL and TIME work before the real urgent macro, then checks abort-first order, copied switch value, immediate BUSY and rejection of another change while BUSY.
- `test_ccd_playerone_sdk`: queues a TIME observer before abort and checks abort completion before the observer, then successful reacquisition. The setup/wait/removal case verifies that an overtaken exposure start never reaches the SDK. Existing abort/readout-ordering and long-exposure/guiding cases run with the abort filter.
- `abort_queue_test_common.h`: a test-only device-queue barrier makes pending-start ordering deterministic through public property changes. `test_usb_outputs` (DSUSB) and `test_serial_outputs` (RTS) verify both true cancellation and false abort preservation; EFA and Prodigy exercise pending calibration/park; dome simulator exercises pending relative motion/park; SkyRoof exercises pending shutter; mount suites exercise pending park, plus home for iOptron/SynScan. Checks require terminal operation/abort states after a normal-priority queue marker. These are behavioral tests, not source-text assertions.
- Existing simulator/fake-SDK suites cover the affected drivers' ordinary operations, abort and available failure/lifecycle paths. All 25 drivers build on macOS. DSI has build coverage only; no matching hardware-free suite is available.

Affected drivers: aux_dsusb, aux_rts, aux_upb, aux_upb3, ccd_dsi, ccd_playerone, ccd_sx, dome_simulator, dome_skyroof, focuser_asi, focuser_dmfc, focuser_efa, focuser_fc3, focuser_fcusb, focuser_ioptron, focuser_lacerta, focuser_primaluce, focuser_prodigy, focuser_usbv3, mount_ioptron, mount_nexstaraux, mount_pmc8, mount_synscan, rotator_asi, rotator_simulator.

Validation notes: initial TCP/UDP runs were blocked by the sandbox and passed when rerun with local port access. The first combined run stopped Prodigy at an external 300-second limit; subsequent validation uses a 900-second suite limit. New barrier fixtures initially passed NULL to enumeration; corrected to the framework's INDIGO_ALL_PROPERTIES selector before accepting results. Final results are recorded below.

Limits: no physical-camera latency measurement, Windows/Linux runtime validation, or proof that the reported extra frames are eliminated. Priority cannot preempt an already executing handler or SDK call. Simulated transports and SDKs do not measure physical stop latency.

Final validation: all 25 affected drivers built; all regenerated C/header/main outputs reproduce exactly; both DSL and generated versions match HEAD; normalized generated diffs contain only abort dispatch/body changes and required declarations. Generator architecture and timer unit suites pass. Of 24 integration suites, 23 passed in full, including EFA 56/56 and Prodigy 54/54. iOptron focuser passed 39/40 in concurrent runs; its unchanged `overlap` test failed at the reverse-during-motion expectation, then passed in isolation with IOPTRON_TEST_FILTER=overlap. That scenario never invokes abort; the non-abort driver bodies are unchanged. Record this scheduling-sensitive test result rather than claiming an entirely green concurrent run.

After the final pending-state refinements, affected focuser abort cases and ASI/Lacerta suites passed; all four mount suites passed with the new queued park/home assertions, and NexStar AUX/PMC-Eight passed again after their terminal-state refinement. Prodigy's queued_abort additionally rejects the stop command after canceling a pending position request and checks that neither operation nor abort remains BUSY. No production change was made to hide the iOptron overlap failure. Hardware and platform limitations above still apply.

## Physical Player One abort measurements (2026-09-10)

`hardware/test_ccd_playerone_hw.c` adds explicit `--abort-latency` and `--abort-agent` modes. These perform 20 phase-varied trials per acquisition path at 0.1 s, count CCD_IMAGE deliveries after abort send/camera BUSY/terminal states, and observe a 600 ms post-completion interval. The direct client stops scheduling further exposures before sending abort; Imager Agent manages its own batch. Original image-format/upload settings are restored, configuration stays temporary, and no guiding/suffix/hotplug workflow runs in these modes. Hardware target linking includes Imager Agent; ordinary hardware workflows remain available.

All final sets delivered zero extra frames: 20 stream, 20 client-series and 20 agent-batch trials. Maximum terminal time was 133.656 ms via the agent. The observer also exposed an idle/inter-exposure camera abort lacking terminal publication (9/20 final agent trials), which is logged as unavailable camera completion rather than treated as an agent-process failure. This initial measurement preceded the terminal-publication fix described below. Detailed physical results and reproduction commands are in root TESTING.md. This is not a hardware-free test or a claim about the original reporting user's client/network path.

The Player One idle-abort branch now explicitly publishes its terminal ALERT after resetting the switch. The new SDK regression fails on the previous driver (client remains BUSY), then passes with the fix. It covers false and true abort requests before acquisition and after a completed exposure, verifies no SDK stop is issued while idle, and checks subsequent acquisition. All five abort-filtered SDK cases pass. Driver version remains unchanged. The hardware harness additionally checks idle abort before the trials and after a fresh completed exposure; `--abort-previews` exercises Imager Agent PREVIEW and indefinite STREAMING, 20 phase-varied trials each.

Fixed-driver physical preview validation passed: 20 PREVIEW and 20 indefinite STREAMING trials; every camera abort published a terminal state. PREVIEW delivered no extra frames; STREAMING delivered one frame after send/BUSY in one trial, none after camera or agent completion. Maximum camera completion was 111.194 ms. Idle abort before acquisition and after fresh reacquisition both passed. Both static and dynamic hardware executables compile. Full timing ranges are in TESTING.md; no network/UI claim is made.

## Imager Agent integration suite (2026-09-10)

Added `integration/test_agent_imager.c`, registered in the normal integration list and available through `make -C indigo_test test-agent-imager`. Its 35 isolated cases execute the real agent, real CCD/wheel/focuser/Bahtinov simulators, and controlled external-shutter/related-agent peers. They cover capture/preview/batch/streaming, all four autofocus estimators and failure/repeat policies, bracketing, selection/statistics, pause and all six breakpoints, dithering, transit/solver coordination, multiple instances, file handling, settings persistence, device loss and cleanup. Faults are injected at the camera property boundary; production queues, timers and algorithms remain in use. See [the scenario-to-test mapping](../indigo_drivers/agent_imager/AGENT_IMAGER_COVERAGE.md) for exact assertions and limits.

Each case has its own process, a 180-second outer watchdog, bounded property waits and a unique temporary configuration/image directory cleaned by the parent even after a crash. Switch/number helpers reject nonexistent items and wait on fresh property revisions. The suite statically links the framework, including a separately compiled configuration-path override, so framework-internal configuration calls are isolated on macOS as well. An unmatched test filter fails. Optional `IMAGER_TEST_FLAGS` and separate build directories support Clang coverage and AddressSanitizer without changing ordinary targets.

The tests exposed related-agent admission rejecting Imager Agent instances. At the user's request this was fixed first with a three-line validation change, without a version bump. The actual two-agent barrier scenario now passes: self-exclusion, no forced reverse relation, breakpoint/trigger propagation, synchronized capture, abort propagation, deselection and shutdown. Instance removal remains tested independently and exposes a separate framework double-free confirmed by ASan. Other observed production failures remain strict failing regressions, listed in [indigo_drivers/REVIEW.md](../indigo_drivers/REVIEW.md#current-findings) as DRV-111 through DRV-119; DRV-116 is fixed. No expectations were weakened to make these defects pass.

Measured Clang coverage of the production Imager Agent after the admission fix and focused reruns: 69/69 functions (100%), 2524/3220 lines (78.39%), 5761/6988 regions (82.44%), 1160/1744 branches (66.51%). This is complete function execution, not 100% branch coverage or proof of every configuration combination. Source coverage combines the full run with the final estimator/header/barrier regression runs. The agent's sequence fields/constants are unused in this checkout, so there is no sequence process to test. Remote URL BLOB transport, real hardware timing, exhaustive pixel-format combinations and Linux/Windows execution are not claimed.

Final validation: the complete normal run finished 25/35 before the user-prioritized admission fix was linked into that running executable. The fixed barrier case then passed in normal and coverage builds, giving final per-case results of 26 passing and 9 failing cases across 35 scenarios. The nine failures represent eight remaining production defects (the two delay breakpoints share one cause); they are not test skips. Both the agent driver and test executable build successfully. The latest autofocus convergence and FITS-header assertions also pass. ASan confirmed DRV-111 independently. No full-suite success is claimed.

DRV-111 follow-up: the filter now clears the additional-client slot after removal, matching its shutdown cleanup. The existing independent-instances/reselection regression passes normally and under ASan. The test build compiles the production filter separately so IMAGER_TEST_FLAGS instruments the affected code too. A new additional-instance lifecycle case performs three 0→2→1→0 cycles, checks the surviving instances, recreates two instances and shuts down with them attached. The three instance cases pass under ASan (leak detection disabled; other library/simulator archives remain uninstrumented). Normal testing passed the original regression and barrier case, but the new lifecycle case hit its 180-second watchdog; a separate traced rerun passed. This intermittent failure is recorded as DRV-120, not hidden by the successful rerun. The suite now contains 36 cases; no new full-suite or coverage measurement is claimed.

DRV-112 follow-up: exposure_batch now returns false after exhausting its three attempts instead of advancing to the next frame or publishing success. The expanded `exposure failure retry exhaustion and recovery` case failed before the fix and passes afterward. It verifies successful retry on the third attempt in preview and a two-frame batch, exactly three failed requests and no images for counts 1, 2 and -1, and successful reacquisition after every failure. The finite-capture and abort-all-capture/reacquire cases also pass (3/3 focused cases including cleanup). No version change, new lock, full-suite rerun or new coverage measurement.

DRV-113 follow-up: streaming_batch now returns success only for terminal INDIGO_OK_STATE; the existing pause/abort paths retain their handling. The expanded streaming-failure regression failed before the fix and passes afterward for counts 1, 3 and -1. It checks ALERT propagation, cleared STREAMING switch, no images from injected failures and successful three-frame recovery after each failure. Finite capture, abort/reacquire and pause/resume/BUSY guards also pass: 4/4 focused cases including cleanup. No driver version change or new full-suite/coverage measurement.

DRV-114 follow-up: the external-shutter branch in abort_process now sends AUX_1_CCD_ABORT_EXPOSURE while retaining the camera abort. Expanded external-shutter regression failed before the fix and passes afterward: held shutter exposure aborted in preview and exposure batch, exactly one camera and shutter abort, terminal shutter ALERT and cleared abort switch, successful reacquisition, then deselection followed by camera-only abort. General abort/reacquire and two-agent barrier cases also pass (3/3 focused cases including cleanup). No version change or new full-suite/coverage measurement; shutter validation uses the in-process peer, not hardware.

DRV-115 follow-up: deletion of the agent camera exposure/streaming property now invalidates its cached operation state with ALERT. Empty-name deletion handles both caches and local-folder cleanup. Expanded camera-disconnect regression failed before the fix and passes afterward: disconnect during BUSY preview/exposure batch/streaming, bounded process completion without explicit abort, cleared start switch, reconnect and one delivered image. Preview retains its existing completion state and retry timing; exposure/streaming batches report ALERT. Retry exhaustion/recovery, streaming failure/recovery and abort/reacquire also pass (4/4 focused cases including cleanup). No driver version change, new locks or full-suite/coverage rerun.

DRV-117 follow-up: narrowed controlled-instance suppression to dithering, allowing PRE_DELAY/POST_DELAY and configured delay to execute in TRIGGER mode. Abort after either delay breakpoint immediately returns failure, including zero delay and the final frame. Both delay cases exercise count 1/2 × delay 0/0.01 with resume, abort, no new requests while paused/after abort, and reacquisition. Pre-fix testing reproduced missing pause publication. The expanded helper initially retried camera selection between subcases and timed out because unchanged selection produces no update; connection is now established once per case. Final delay cases pass, as do both capture-breakpoint cases, both dither cases and the two-agent barrier (7/7 focused cases). No version change, new locks or full-suite/coverage rerun.

DRV-118 follow-up: exposure_batch checks abort state after POST_BATCH so its caller executes the existing abort-finalization branch. Expanded POST_BATCH regression failed before the fix and passes afterward for counts 1 and 2: resume to OK, abort with the breakpoint still enabled to process ALERT and abort OK, cleared start/abort switches, exact request/image counts and successful subsequent batch. All six breakpoint cases plus two-agent barrier and general abort/reacquire pass (8/8 focused cases including cleanup). No driver version change, new locks or full-suite/coverage rerun.

DRV-119 follow-up: attach initializes use_hfd_estimator and use_ucurve_focusing alongside the default U-Curve switch. The original default-preview assertion also lacked a selected star; the regression now discovers/selects a star without changing the estimator. This corrected test fails on a separately compiled copy without the two initialization lines and passes with them, including HFD and default U-Curve autofocus convergence from 60 steps out of focus. Configuration reload now saves non-default RMS, reloads it and verifies selection, nonzero RMS preview and zero HFD-star limit. Explicit autofocus estimators and Bahtinov preview/focus also pass (4/4 focused cases). No driver version change, new locks or full-suite/coverage rerun.

DRV-120 follow-up: thread sampling reproduced a bus-mutex/queue-completion deadlock during synchronous additional-instance removal: the bus callback waited in indigo_queue_remove while disk_usage_timer_callback waited to publish a property under the same bus mutex. Imager Agent now processes ADDITIONAL_INSTANCES on its existing device queue using INDIGO_COPY_VALUES_PROCESS_CHANGE; configuration saving remains after the completed change. No new locks or driver version change. The new public-bus regression deliberately starts a secondary-instance queued publication while the removal request holds the bus lock: the separately compiled pre-fix agent hits the 20-second watchdog, while the fixed agent passes normally and with ASan. All four instance cases pass in both builds; ten repeated lifecycle/publication/barrier runs also pass (30 case executions). ASan instruments agent/filter/configuration framework/harness, with leak detection disabled and the other library/simulator archives uninstrumented. The test suite now has 37 cases.

DRV-120 final validation: complete normal Imager Agent suite 37/37 passed, including cleanup. The deterministic callback owns/releases its test property; after this test-only refinement its targeted normal and ASan reruns passed, and the final pre-fix control still hit the watchdog. Historical line/branch coverage was not remeasured.

PlayerOne shared countdown: ordinary exposure setup now uses indigo_ccd_exposure_setup rather than duplicate image/file BUSY updates; acquisition_finalizer publishes remaining seconds only for streaming. New fake-SDK regression blocks the camera queue during a 2.2-second exposure, verifies independent integral countdown progress, then checks completion and subsecond reacquisition. Driver input and generated output remain synchronized; no version bump or generator implementation change.

Validation: the new blocked-device-queue countdown regression fails against the HEAD driver without this change and passes with it. The complete fake-SDK rerun passes all 48 test bodies, but its exit status remains failure because cleanup of Final slow_initialization_and_polling records a gate timeout. That same cleanup failure reproduces against the unchanged HEAD driver in isolation. The first full run additionally saw a cooler failure-state assertion; its isolated rerun and the second full run pass. All abort-filtered cases pass. Cleanup failures now identify their case in the test output. No full-suite success or hardware validation is claimed.
