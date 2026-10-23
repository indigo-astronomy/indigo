# Driver-local helper source coverage reconciliation

Reviewer: `complete_driver_a`, source revision `f019b004769072275a3d98a87c8e9740a42c19a4`. Scope is the 207 exact driver-local candidates selected from `unmapped-source-candidates.json`, reconciled in `helper-coverage.json`. This supplements the production driver ledgers; it does not replace them. Source remained read-only. Only these two assigned task artifacts were written in this pass.

## Result and evidence meaning

The assigned C/C++/header/Objective-C helper scope is complete at functional-component source-review level. The ledger records an exact SHA-256, ownership/provenance, disposition and contract for every selected path. All 207 current file identities were rechecked while finalizing the ledger. There are no `in_progress` entries. There are 83 functional helper/adapter entries, two data adapters and seven identical manual Arduino support headers reviewed; twelve generated outputs, sixteen vendor inputs, two firmware payloads and thirty-three literal image tables have explicit provenance exclusions from independent executable-logic review. Fifty-two script/JavaScript/Arduino files are assigned to the independent manual-helper reviewer, with results in `helper-manual-review.md` and `helper-manual-coverage.json`; this report does not claim that review's completion on its behalf.

This is a source audit of maintained functional components, not a claim that every line was individually inspected or every supported model was exercised. The focused source reads included dispatch bodies, protocol framing and reply branches, asynchronous state advancement and stop behavior, ready-file publication and resource ownership. No simulators, hardware, firmware, external services, exploit reproductions or shared builds were executed in this reconciliation. Existing task test evidence retains its own revision, scope and limitations.

## Host simulator coverage

Each focuser, GPS, guider, mount, polar aligner, rotator and wheel simulator candidate has its own protocol/state/ready-file note in the ledger. This includes binary length/checksum protocols (EFA, MJKZZ, RoboFocus, WeMacro, NexStar/AUX, Trutek), textual/JSON protocols, model-specific reply quirks, motion/settle and asynchronous completion, abort/stop behavior, and explicit synthetic failure controls. They remain maintained firstparty test tools even where their header says AI-generated. Shared `serial_motion` and simulator infrastructure ownership is covered by the core/test audit, rather than counted as driver-local missing source.

The Lunatico focuser and rotator wrappers share a firstparty 401-line SLP implementation. Its review covered three-port motion and limits, speed/configuration/power/temperature/digital-pin operations, echo/status reply forms, argument/port checks, injected failures and external control/event files. It delegates PTY/UDP framing and ready-file lifecycle to the shared simulator infrastructure.

The Alpaca simulator review covered the common device/member table contract, strict typed request parameters, version/capability/connection routing, simulated clock/motion, image response roles, HTTP/discovery startup and shutdown, and each of its ten device state modules. Camera ROI/bin snapshots and exposure/readout/abort/stop behavior, focuser settle/late correction/halt, telescope tracking/manual/pulse/slew/park/home/flip/delayed states, rotator mechanics, wheel movement, dome/shutter, calibrator, observing conditions, safety and asynchronous switch behavior were examined. Requests serialize simulator state under `simulator_lock`; connection shutdown unblocks reads before joining threads, then device/record/log resources are freed. Ready files use a temporary file plus rename and publish the actual endpoint/PID. This functional review does not certify network security or all combinations of fault configuration.

## Provenance and ownership

Duktape engine/configuration and bundled Ken Tossell libuvc sources retain upstream ownership. They are dependency inputs, not independently re-audited INDIGO implementations in this scope. The INDIGO `indigo_ccd_uvc_libuvc.c/.h` shim is separate and was reviewed: its open/close forwards preserve SDK ownership while preprocessor renaming avoids generated lifecycle name collisions.

`libdsi.c/.h` is an INDIGO-maintained adaptation by Rumen G. Bogdanovski based on Roland Roberts' libdsi, not an unreviewed vendor exclusion. Enumeration/open/initialization, exposure submission, image reads and close/release were traced. Literal firmware headers remain byte payloads; firmware execution/disassembly is outside this source audit. Simulator image arrays are literal fixtures; their dimensions/include adapter was reviewed separately.

Only outputs carrying explicit generator provenance were assigned `excluded_generated_output`, with their adjacent `.driver` input as the authoritative source already covered by the production-driver audit. Standalone `_main.c` files without that provenance were reviewed as maintained XML adapters: bus start, driver initialization/attachment, request parsing, driver shutdown and bus stop, including BLOB-mode and Alpaca SIGPIPE differences. No generated output was edited or regenerated here.

The seven iOptron Arduino `common.h` copies have the identical SHA-256 `b70596e7406910dc8abaf127475385eb9f866918d979597b3fdc044987e3e465`. Their shared source was reviewed for coordinate parse/format, millis-based motion, tracking/manual rates and park/home completion. They support manually loaded Arduino simulators and have no host ready-file contract. Their source role is not a finding that they are obsolete. Arduino command sketches are independently reviewed in the manual-helper report; no firmware build/flash or physical behavior is claimed.

## H01: DSI transport failure propagation

Confirmed defensive functional maintenance observation, medium severity, nonblocking for this audit; coordinator owns canonical allocation and disposition.

`indigo_drivers/ccd_dsi/libdsi.c:1403-1473` (`dsi_start_exposure`) discards command return values while configuring/starting an exposure, then sets exposing state and returns success. The upper DSI driver treats this success as an accepted exposure. A failed setup or trigger can therefore appear to have started successfully. `libdsi.c:1504-1574` (`dsi_read_image`) checks negative bulk-transfer status but does not enforce the returned actual byte count before decoding a successful transfer, so a short successful image transfer can be treated as a complete frame using retained buffer contents. This is source evidence, not a hardware or crash reproduction.

Minimal remedy: propagate failed setup/start commands, commit exposing state only after accepted start, validate exact expected image-plane transfer lengths before decoding, and report failure to the driver. Focused synthetic transport tests can check failed setup/start and short successful frames. No new runtime reproductions were run. The coordinator indicated this observation will be allocated as H01 after final reconciliation; this report does not close or accept it.

## Remaining boundaries

No maintained C-family helper candidate remains unclassified or pending in this assigned ledger. The independent manual-helper report must be included before qualifying the combined helper coverage. Vendor implementation internals, literal data/firmware bytes, Arduino execution, hardware behavior and unsupported host platforms retain the explicit limits above. Source-review completeness does not turn these limits into runtime certification or accept existing findings.
