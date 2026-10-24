# Portable driver review: AO, auxiliary, camera, dome, GPS, guider, polar alignment, rotator, wheel and shared library

Reviewed source: `9d45bc8cc`. Independent read-only repository-reviewer/security-reviewer subagent; coordinator is the only persistent writer. Root AGENTS.md, indigo_drivers/AGENTS.md, repository-reviewer/security-reviewer roles, project map, audit brief/specification, inventory and active finding summaries loaded. No driver/source modifications, generator runs, tests, physical connections, network requests or exploit reproduction were performed.

## Method and coverage

`/tmp/audit-driver-ac-coverage.json` records 759 assigned non-excluded inventory files in 91 directories: 218 source files (84,043 lines), 390 support files, 72 generated companions, 44 embedded vendor files, 35 image/firmware tables. Generated companions are not independent source coverage. Singular `aux_joystick/external/DDHidLib`, copied `ccd_uvc/libuvc*` and utlist, and shared jsmn are explicitly classified as vendor even where the root inventory did not already exclude them. All five owner-off-limits driver directories were excluded before enumerating source contents.

The full maintained source corpus received standard-library regular-expression scans for resource open/close, connection/disconnection, allocations/copies, transport calls and response parsers, polling loops, queue/finalizer/cancellation, and property declarations/count/hidden assignments. Every authoritative .driver had its open/close/connect/disconnect and loop excerpts extracted for semantic review; small rebranded camera wrappers were traced to ccd_touptek, and large handwritten ToupTek, PTP, Pentax and joystick lifecycle/transport excerpts were separately inspected. The source file treatment is contract scans plus semantic lifecycle/control-flow review, not an exhaustive line-by-line/manual or hardware review. Some large excerpts were capped at 90 lines; suspicious paths were reread directly, including the failed-open generator call site for aux_usbdp. Support text was scanned for build/source references and documentation/status/test claims; it was not all read line by line. Driver-local documentation presence, authoritative versions, exact-name integration test source presence and matching migration table rows are recorded per directory. Eighty-five of 91 directories have matching integration test sources; source presence is not evidence of a test pass. Detailed simulator fidelity, every SDK return-value path, all documentation claims and all platform build variants were not exhaustively proved; retain these as assessment limitations.

No source audit establishes guiding timing, physical end stops, manufacturer protocol compatibility or vendor SDK behavior. The review provides a whole-assigned-corpus source-contract scan with semantic investigations. Do not describe it as exhaustive manual coverage or whole-driver certification.

## New confirmed findings

### AC-DRV-1: USB Dewpoint failed identification retains its transport

- Location: indigo_drivers/aux_usbdp/indigo_aux_usbdp.driver:103-149, especially 105-106 and 146-149.
- Severity: medium; confidence: high; category: functional/resource lifecycle. Owner: coordinator; status: open; nonblocking audit follow-up.
- Evidence: after serial open succeeds, cleanup at line 146 is inside the successful `usbdp_command(UDP_IDENTIFY_CMD)` branch. Failed writes/reads return false at 149 with PRIVATE_DATA->handle still live. Targeted inspection of generated aux_connection_handler at indigo_aux_usbdp.c:447-478 confirms the open-false branch publishes ALERT/disconnected without invoking close; the next connect overwrites the stored handle.
- Impact: failed identification and reconnect can leak serial handles/resources and interfere with a subsequent connection. No runtime failure was induced.
- Minimal remediation: make all unsuccessful open paths close the acquired handle before returning false; retain normal connected shutdown ownership. Add simulator failed-identify/reconnect resource assertions, bump driver source version and regenerate unchanged generator outputs.
- Deduplication: no matching active record found; unlike FND-000048 (UPB queued-property completion), this is USB Dewpoint resource ownership.

### AC-DRV-2: PrimaLuce token lookup assumes an unused array sentinel

- Location: indigo_drivers/shared/primalucelib/primalucelib.c:374-405, specifically 394-399; parser acceptance at 253-265; capacity in primalucelib.h:77-78,502-503.
- Severity: medium; confidence: high from source; category: security/input validation. Owner: coordinator; status: open; nonblocking audit follow-up.
- Evidence: parser accepts every positive jsmn return, including all 1024 allocated tokens; it does not retain the returned token count. The missing-key/subtree skip increments index and reads `tokens[index]` until an undefined sentinel or sibling is found, with no capacity/count bound. A completely occupied valid token array has no undefined sentinel after its last token. Searching for an absent field after a final nested subtree can therefore advance beyond tokens[1023]. The 8192-byte response buffer allows a token-dense JSON response to fill the token array.
- Prerequisite/impact: an abnormal controller reply reaching the shared JSON library can cause an out-of-bounds read and crash/undefined behavior. This is a source-boundary finding, not a demonstrated exploit or remote code execution claim. No payload or reproduction was created.
- Minimal remediation: store the positive parser token count and bound start/index/recursive traversal before every token access; return absent for traversal at end. Add focused parser boundary regression coverage for full token arrays and absent nested fields.
- Deduplication: active FND-000052 covers unescaped outbound WiFi credentials in this library, not token traversal bounds; no matching active record found.

### AC-DRV-3: SVB powerbox boot drain has no overall deadline

- Location: indigo_drivers/aux_svbpowerbox/indigo_aux_svbpowerbox.driver:141-157, especially 147-156.
- Severity: medium; confidence: high; category: functional/availability. Owner: coordinator; status: open; nonblocking audit follow-up.
- Evidence: comment promises "up to 3 seconds, 300 ms silence", but the loop is conditioned only on repeated 300000-microsecond waits. No overall deadline, byte cap or iteration limit exists. Successful arriving data resets the effective wait indefinitely.
- Impact: a controller in a reset/logging loop or continuous serial noise can keep the serialized connect handler active indefinitely, postponing disconnect and other operations. No hardware interaction or induced failure occurred.
- Minimal remediation: enforce the documented monotonic overall boot-drain deadline, detect failed reads, fail transactionally with handle cleanup when continuous data exceeds it, and add simulator continuous-boot-data/reconnect regression coverage; bump version and regenerate outputs.
- Deduplication: no matching active record found; existing FBC/FlipFlat and Pentax unbounded-operation findings concern different protocols and functions.

### AC-DRV-4: Player One snapshots are processed as streaming frames

- Location: indigo_drivers/ccd_playerone/indigo_ccd_playerone.driver:413; finish behavior at 335-355. Cross-component contract: indigo_libs/indigo_ccd_driver.c:1806-1815,2316-2328.
- Severity: medium; confidence: high; category: functional/image delivery. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: the shared snapshot/stream acquisition finalizer passes literal `true` for indigo_process_image's final `streaming` argument, even when PRIVATE_DATA->streaming is false. The base framework applies streaming update throttling and selects AVI/SER file output only when this argument is true. acquisition_finish finalizes video only when PRIVATE_DATA->streaming is actually true.
- Impact: single exposures can be throttled like streams and JPEG_AVI/RAW_SER single exposures can create video files without the ordinary single-image/finalization behavior. SDK continuous-acquisition workarounds do not make a public single exposure a streaming operation.
- Minimal remediation: pass the actual public operation's streaming flag to image processing; test snapshots with update limits and both video-capable format choices, while preserving the Saturn SDK workaround; bump source version and regenerate. No active duplicate found; no runtime reproduction performed.

### AC-DRV-5: File camera accepts a truncated RAW payload

- Location: indigo_drivers/ccd_simulator/indigo_ccd_simulator.driver:761-771. Cross-component contract: indigo_libs/indigo_uni_io.c:1995-2027; allocation semantics indigo_libs/indigo_bus.c:1795-1802 and indigo/indigo_bus.h:1041-1047.
- Severity: medium; confidence: high; category: functional/data validation. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: header and pixel indigo_uni_read return values are converted to bool. That API returns a short positive byte count at EOF and -1 on error, both truthy. Shape/signature arithmetic checks do not prove bytes were actually read.
- Impact: truncated pixel input can connect as a valid file camera and produce full-size images with a zero-filled unread tail. Allocation is zero-initialized, so this finding does NOT claim uninitialized memory disclosure. No runtime reproduction performed.
- Minimal remediation: compare header and payload reads to exact requested lengths, require representable API lengths, and keep failed-open cleanup. Add truncated header/pixel and read-error regression fixtures, bump simulator driver version and regenerate.
- Deduplication: existing audit-022 targets agent_imager/alpaca consumers, not this file simulator's read-count bug; register separately.

### AC-DRV-6: GIOTTO poll replaces a queued light/brightness request

- Location: indigo_drivers/aux_giotto/indigo_aux_giotto.driver:89-91,133-145,239-240,272. Generated call-site check: indigo_aux_giotto.c:740-743 and441-477.
- Severity: medium; confidence: high; category: functional/concurrency. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: after panel_get_state I/O, a changed state or recovery from poll_alert calls giotto_publish_light, overwriting the light switch and brightness value AND target, then sets both properties OK. No per-property BUSY/pending guard exists. A concurrent bus-thread change sets BUSY and stores the request before its queued handler runs; that handler later reads the same overwritten switch or number.target.
- Impact: an external panel-state change or poll-alert recovery concurrent with a client request can replace that request with old readback, resulting in a successful handler sending a different value. No race reproduction performed.
- Minimal remediation: adopt unsolicited/readback values only into properties with no pending change; preserve each property's request target and BUSY state until its handler completes. Add delayed-poll overlap tests, bump version, regenerate. Existing FND-000048 concerns aux_upb; no GIOTTO duplicate found.

### AC-DRV-7: AO simulator sends the untruncated formatted length

- Location: indigo_drivers/ao_sx/ao_sx_simulator/ao_sx_simulator.c:87-94,230-235.
- Severity: low; confidence: high; category: functional/simulator memory boundary. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: --firmware accepts arbitrary-length text; firmware reply is snprintf'd into buffer[8], then sim_send uses the returned would-have-written length without a capacity check.
- Impact: an overly long local firmware option makes the simulator read past the reply buffer, emitting unrelated stack bytes or failing. This requires local simulator configuration, not remote product input. No reproduction performed.
- Minimal remediation: validate the firmware reply length or clamp/reject snprintf truncation before sending; add boundary-option tests. Distinct from active AO simulator axis-limit audit-045.

### AC-DRV-8: Firmware simulator receive buffers lack terminator capacity

- Locations: indigo_drivers/aux_flipflat/aux_flipflat_simulator/aux_flipflat_simulator.ino:87-90; indigo_drivers/dome_nexdome3/dome_nexdome3_simulator/dome_nexdome3_simulator.ino:80,271,276-279.
- Severity: low; confidence: high; category: functional/simulator input boundary. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: FlipFlat zeroes a 32-byte array, then lets readBytesUntil fill all32 bytes and calls strcmp without appending NUL. NexDome3 declares ReceiveBuffer[COMMAND_LENGTH], accepts COMMAND_LENGTH data bytes and then writes NUL at index COMMAND_LENGTH when the terminator arrives.
- Impact: full-size abnormal serial input can make FlipFlat compare beyond its local buffer or make NexDome3 write one byte beyond the receive array. These are manual firmware simulators, not the portable production driver. No flashing/hardware/reproduction performed.
- Minimal remediation: reserve and explicitly write the terminator using the received count; enlarge NexDome3 storage or reduce accepted payload capacity; reject and drain overlong commands. No matching active finding found; shared boundary-contract repair can be one follow-up or split per driver by coordinator.

## Existing findings / design concerns

Known source patterns remain covered by existing records rather than duplicate findings: FND-000048 (aux_upb BUSY overwrite), FND-000052 (PrimaLuce outbound credential escaping); FND-20261023101753-bc8d-complete-repository-audit-017/-018 (FBC/FlipFlat blocking handlers), -019 (PTP device-info decode boundary), -020/-021 (Pentax unchecked open/unbounded readiness), -023 (device filter count capacities), -024 (unbounded wheel completion), -027 (stopped-short result), -028 (Falcon ACK), -044 (DSI failed transport), -045 (AO simulator limits). These were deduplicated using active summaries; not all historic per-file evidence was re-proved and no archived task was routinely loaded. Existing known findings are not closed by this review.

Protocol-specific intentional call ordering, bounded finalizers, explicit failed-open rollback and SDK workarounds are useful safeguards and were not proposed for stylistic removal. The boot comment's false time bound is a concrete documentation/control-flow mismatch; no AI-authorship inference or cleanup quota was used. Rebranded SDK wrapper reuse is real architecture, not needless duplication. Generated driver outputs require their source inputs and are not independent handwritten logic.

## Verdict

Changes required for the eight new findings; record as nonblocking repair follow-ups, without accepting their risk. The assigned inventory received complete contract scanning and per-driver lifecycle semantic review, with explicitly bounded depth and unverified hardware/platform/runtime behavior. If the full-audit definition requires exhaustive manual reading or proof of every SDK/parser path, this evidence is partial for that stronger definition. Coordinator must combine coverage with other component reviewers and qualification checks before declaring the full audit complete.

## Completed semantic waves and explicit depth limits

The follow-on review did not stop at the initial scan ledger. `/tmp/audit-driver-ac-replies.txt` contains 4,170 numbered lines of command/read/query/parse/finalizer/property initialization blocks spanning all76 .driver inputs. All nine helper bodies omitted by the 100-line extraction cap were then read directly: wbprov3_read_status; MI initialize_ccd; PlayerOne initialize_properties; qhy2_initialize_ccd; qsi_initialize_ccd; SX sx_read_pixels; Baader dome_status_poll; Beaver dome_status_poll; MLastro mlastro_parse_telemetry. `/tmp/audit-driver-ac-simulator-boundaries.txt` contains the completed 2,257-line read/receive/send-boundary wave across assigned simulator C/Arduino source, and wrapper-only simulators were traced to their shared implementations. The rebranded camera .c wrappers map into ToupTek; .m forwarding units map into the corresponding C; no independent lifecycle exists in those wrappers.

`/tmp/audit-driver-ac-table-shapes.json` proves numeric entry counts agree with mono1600x1200, RGB1600x1200x3 and each of31 Bahtinov500x500 tables. Read create_frame clamping and table accessor loops; coefficients/pixels were not judged. `/tmp/audit-driver-ac-firmware-shapes.json` records DSI512 length-prefixed records, SSAG59 bootloader and390 firmware records, each within static data extent with terminal zero; EEPROM10 bytes are separately classified. Firmware downloader bounds and static representation were inspected; actual hardware/USB firmware effects were not tested.

Remaining authored-file depth limits (all were inventoried and contract-scanned, with selected lifecycle/transport semantic reads): `ccd_dsi/libdsi.c`; `ccd_pentax/indigo_ccd_pentax.c`; `ccd_ptp/indigo_ccd_ptp.c`, `indigo_ptp.c`, `indigo_ptp_canon.c`, `indigo_ptp_fuji.c`, `indigo_ptp_nikon.c`, `indigo_ptp_olympus.c`, `indigo_ptp_sony.c`; `ccd_touptek/indigo_ccd_touptek.c`; `aux_joystick/indigo_aux_joystick.c`; shared `primalucelib.c`. Their every device-specific SDK setter/decoder mapping branch was not manually proved. Every .driver and simulator received semantic contract/boundary coverage but not a proof of each operational branch. Rotator Lunatico's imported shared driver and simulator implementation belongs to the focuser reviewer, and that dependency must be correlated by the coordinator. Root owns documentation/build/test metadata review. There are no completely unvisited assigned driver directories, but this bounded breadth assessment must not be represented as exhaustive manual/hardware/SDK certification.

## Deep closure wave for the twelve previously bounded-depth files

All executable function bodies in the twelve-file function inventory `/tmp/audit-driver-ac-deep-functions.json` were now directly inspected, including the Objective-C methods not discovered by the C function extractor. The brace scanner was corrected to ignore comments and quoted string/character contents before recording end lines. The extractor is navigation evidence, not a compiler or proof of correctness.

New contiguous body spans inspected: `ccd_dsi/libdsi.c:91-1574`; `ccd_pentax/indigo_ccd_pentax.c:315-1210`; `ccd_ptp/indigo_ccd_ptp.c:66-1123` including ICA browser methods; `ccd_ptp/indigo_ptp.c:364-1890` including ICA delegate methods; `indigo_ptp_canon.c:807-1859`; `indigo_ptp_fuji.c:329-759`; `indigo_ptp_nikon.c:720-1300`; `indigo_ptp_olympus.c:287-860`; `indigo_ptp_sony.c:896-1659`; `ccd_touptek/indigo_ccd_touptek.c:238-3010`; `aux_joystick/indigo_aux_joystick.c:184-1143` including macOS wrapper methods; `shared/primalucelib/primalucelib.c:455-1039` completing the previously inspected122-451.

Static PTP enumeration/label functions (35) were structurally enumerated and literal extents checked; maximum literal length41 characters. Their nondeclarative formatting, arithmetic, lookup and fallback paths were separately inspected. The Nikon lens-name case map944-1122 and device/model coefficient tables receive shape/accessor treatment, not a claim that hardware mappings or coefficients are correct. Mapping ledger: `/tmp/audit-driver-ac-mapping-shapes.json`.

This removes the former whole-function scan-only gaps for these twelve files. It does not supersede the original ledger's treatment of the other assigned files, certify every SDK's implementation, or add hardware/platform execution. Existing DSI failure044, Pentax open020/readiness021 and ToupTek property-count023/deadline024/stopped-success027 families were reconfirmed; do not register duplicates for those.

### AC-DRV-9 — Pentax response lengths control stack allocation and unbounded fixed-offset decode

Severity medium; confidence high; functional/security input-boundary finding. `indigo_drivers/ccd_pentax/indigo_ccd_pentax.c:551-568,582-596,605-613,621-655` obtain a uint32 length from camera readiness, allocate `uint8_t buffer[length]`, pass length to the uint16 `pentax_read` argument at503, and decode identity/firmware/full-state offsets without a minimum payload check. Oversized replies can exhaust the stack; short or length-truncated transfers can leave the subsequent fixed offsets outside valid input. The SCSI helper399-447 also does not require an exact data-stage count before accepting CSW success. Distinct from active020 and021 (open/readiness). Minimal fix: bounded response storage, exact transfer count, reject unsupported lengths before allocation/narrowing, validate all model-state offsets against received bytes; fake USB short/large status regressions. Source-only evidence, no adversarial fixture or reproduction executed.

### AC-DRV-10 — Pentax unplug frees a NULL local instead of the allocated private state

Severity low; confidence high; resource ownership. `indigo_drivers/ccd_pentax/indigo_ccd_pentax.c:1138-1149` initializes `private_data=NULL` and never assigns it; after detaching/unref it frees that NULL and frees only the device. Allocation at1113 remains leaked on every unplug and driver shutdown routed through unplug. Minimal fix: capture `device->private_data` before detach and free that captured allocation exactly once; fake repeated attach/unplug ownership check. No matching active finding identified.

### AC-DRV-11 — PTP descriptor/vendor-extension collections trust input counts beyond destination capacities

Severity medium; confidence high; input bounds. Generic descriptor `indigo_drivers/ccd_ptp/indigo_ptp.c:751-814` stores unbounded enum counts then writes `value.sw.values[i]`; capacity is1024 at `indigo_ptp.h:325`. Text enums only decode first16 but preserve larger `target->count`, later indexed through that count by update at1506-1512 and1556 onward; text storage is16 at header329. Sony duplicates numeric enum writes at `indigo_ptp_sony.c:1072-1132`, and its extension-info classifier1325-1349 appends received entries to1024-element local collections without caps. Canon `indigo_ptp_canon.c:1036-1074` validates group count/size only by assert but does not validate group ID before indexing `ex_func_group[16][1024]`, record extent, or item count; its updated pointer list has unchecked appends1071 and1176 despite outer record framing. The generic positive count/fixed-array issue and Canon group/list issue may merit separate follow-up records. Minimal fix: bounded cursor with remaining bytes plus explicit destination capacity and terminator reserve, validate group indices and counts before modifying state; reject malformed descriptors atomically. Related existing019 covers DeviceInfo decoders, not descriptor destinations or vendor group indices; extend its scope explicitly or register linked findings, do not silently call it a duplicate. Source-only; no exploit/payload/reproduction.

### AC-DRV-12 — PTP object downloads publish the metadata size instead of actual downloaded bytes

Severity medium; confidence high; security/functional length contract. Generic `indigo_drivers/ccd_ptp/indigo_ptp.c:1718-1737` reads advertised image size from ObjectInfo, then GetObject uses `data_in_size=NULL` and forwards the prior metadata size to preview/image. Sony `indigo_ptp_sony.c:1410-1437` and Fuji `indigo_ptp_fuji.c:379-392` repeat this. Transport allocates from actual data-container length (`indigo_ptp.c:1387-1410`, with1024 padding on raw USB; ICA allocates exact input length1150-1156). If metadata exceeds received length/padding, native publication or local save reads beyond that allocation. The sink trusts data_size for file write and blob size (`indigo_libs/indigo_ccd_driver.c:2610,2629-2632`). Minimal fix: request/retain actual downloaded length, validate metadata independently, and pass only verified actual bytes to all consumers. Add fake transport mismatched metadata/object tests. Distinct from active019 DeviceInfo parsing. No reproduction executed.

### AC-DRV-13 — Canon text-property encoding may scan and send beyond its stack buffer

Severity medium; confidence high; length/termination. `indigo_drivers/ccd_ptp/indigo_ptp_canon.c:1534-1540` copies256 characters with strncpy into a256-byte payload without forcing NUL, then strlen derives the transfer length. The caller1545-1547 passes the framework text item's value, whose capacity is larger than256; a valid long text property is sufficient to lose termination. This can overread stack bytes and use that inflated length for the USB/ICA outgoing copy. Minimal fix: reject an overlong PTP text value or reserve and write a terminator with a checked explicit encoded length; test longest accepted property and one-too-long. No matching active finding identified. Source-only, no payload/reproduction.

No new AI-authorship/style inference is made. The meaningful design concern is repeated length/count loss across PTP vendor layers despite a bounded transport; fixing only the transport does not establish safe consumer contracts.
