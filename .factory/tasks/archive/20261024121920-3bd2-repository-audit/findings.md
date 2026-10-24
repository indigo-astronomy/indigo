# Canonical new findings

Reviewed source: `9d45bc8ccadaf0c3aab6c454c38429fec21d1699`. Origin: `20261024121920-3bd2-repository-audit`.

All44 records remain open, coordinator-owned for a separately scoped repair and regression task. Nonblocking refers to this review-only artifact; product remediation remains required. No risk acceptance, release approval or source repair is implied. Source claims retain component evidence and independent security validation where assigned.

| ID suffix | Alias | Severity/category | Finding |
| --- | --- | --- | --- |
| 001 | CORE-1 | high / security | Property copy reuse loses destination capacity and text ownership |
| 002 | CORE-2 | high / security | HTTP resource lookup outlives its registry ownership |
| 003 | CORE-3 | high / security | Fixed name and text suffixes do not reserve destination capacity |
| 004 | CORE-4 | medium / functional | Numeric formatter reports failure for ordinary successful output |
| 005 | CORE-5 | medium / functional | Identity image stretch divides by a singular midpoint |
| 006 | CORE-6 | medium / functional | Standard unified I/O handles initialize the wrong fields |
| 007 | CORE-7 | medium / functional | JSON decoding loses escaped backslashes |
| 008 | CORE-8 | medium / security | Mount alignment file loading trusts unchecked counts and rows |
| 009 | CORE-9 | medium / functional | Profile selection probes the availability of profile zero |
| 010 | CORE-10 | high / security | Text helper and FITS paths interpret already formatted data again |
| 011 | CORE-11 | high / security | CCD filename expansion exceeds intermediate and caller capacities |
| 012 | CORE-12 | medium / functional | RGB48 endian conversion swaps only one channel worth of samples |
| 013 | CORE-13 | medium / functional | Exponent notation changes numeric zero into a nonzero value |
| 014 | CORE-AWB | medium / functional | RGB stretch selects the wrong reference on tied channel totals |
| 015 | CORE-MULTI | medium / functional | Exposed multipoint mount alignment returns success without coordinates |
| 016 | AC-1 | medium / functional | USB Dewpoint failed identification retains its transport |
| 017 | AC-2 | medium / security | PrimaLuce token lookup assumes an unused array sentinel |
| 018 | AC-3 | medium / functional | SVB powerbox boot drain has no overall deadline |
| 019 | AC-4 | medium / functional | Player One snapshots are processed as streaming frames |
| 020 | AC-5 | medium / functional | File camera accepts a truncated RAW payload |
| 021 | AC-6 | medium / functional | GIOTTO poll replaces a queued light/brightness request |
| 022 | AC-7 | low / functional | AO simulator sends the untruncated formatted length |
| 023 | AC-8 | medium / functional | Firmware simulator receive buffers lack terminator capacity |
| 024 | AC-9 | medium / security | Pentax response lengths control stack allocation and unbounded fixed-offset decode |
| 025 | AC-10 | low / functional | Pentax unplug frees a NULL local instead of the allocated private state |
| 026 | AC-11 | medium / security | PTP descriptor/vendor-extension collections trust input counts beyond destination capacities |
| 027 | AC-12 | medium / security | PTP object downloads publish the metadata size instead of actual downloaded bytes |
| 028 | AC-13 | medium / security | Canon text-property encoding may scan and send beyond its stack buffer |
| 029 | FM-1 | medium / security | Firmware version concatenation can overwrite the focuser INFO item |
| 030 | FM-2 | medium / functional | Raspberry Pi private data never constructs its std::string |
| 031 | FM-3 | medium / security | Alpaca camera mapping writes past its fixed property capacity |
| 032 | FM-4 | medium / functional | HC8406 guiding reports successful completion after a failed stop |
| 033 | FM-5 | medium / functional | Identical U-curve minima are all discarded and produce NaN focus |
| 034 | FM-6 | low / functional | Guider detach leaks its Multi Kernel GP reset property |
| 035 | FM-7 | medium / security | Scripting queued property callbacks borrow cache pointers that can be reallocated |
| 036 | FM-8 | medium / functional | MJKZZ Bluetooth replies skip the protocol checksum used by the serial driver |
| 037 | MATH-1 | medium / security | DONUTS accepts narrow valid regions that violate its FFT/background window bounds |
| 038 | MATH-2 | medium / functional | Three-point polar alignment divides by a zero intermediate for solvable geometry |
| 039 | MATH-3 | medium / functional | Platesolver reports an exposure started after its acceptance wait expires |
| 040 | ROOT-1 | low / quality | Driver test dry-run documentation promises no file changes |
| 041 | ROOT-2 | medium / functional | OmniSim startup failure loses ownership of the launched process |
| 042 | CORE-BROWSER | high / functional | Browser manual mount motion can miss release outside its button |
| 043 | ROOT-FACTORY | low / quality | Factory role and archive guidance mismatches the adopted layout and version |
| 044 | ROOT-FOLLOWER | low / quality | Multicamera guide overstates the follower inter-frame delay bypass |

## FND-20261024121920-3bd2-repository-audit-001

`indigo_libs/indigo_bus.c:1618-1635`, public contract `indigo/indigo_bus.h:667`, live cache reuse `indigo_filter.c:1006`. Reused destination grows only to active source count, then header memcpy overwrites its allocated_count with source reserve capacity. Release and later reuse trust a capacity the destination may not own. Old destination long_text allocations are overwritten without freeing. Fix capacity-preserving deep assignment and release previous text ownership. Existing copy test exercises NULL destination only. Root independently confirmed.

Evidence: [core-report.md](core-report.md); original alias CORE-1. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-002

`indigo_server_tcp.c:408-424`, PUT lookup500ff; remove584-596/free595, remove-all606-616. Lookup unlocks registry before dereferencing/calling borrowed resource, while a normal guider log refresh removes/readds `/guiding` (`indigo_agent_guider.c:435-436`) and Alpaca detach removes handlers. Snapshot/reference ownership and retire only after active response releases; coordinate handler unload lifetime. Root independently confirmed.

Evidence: [core-report.md](core-report.md); original alias CORE-2. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-003

XML adapter `indigo_client_xml.c:44` prepends `@ ` with sprintf into same-size name; actual server names are fixed-size too (`indigo_client.c:473`). Related service port suffix443-450, filter proxy property/group prefix956-966, baudrate `-8N1` suffix `indigo_driver.c:1734`, CCD directory separator820-824, bus make_name_unique strcat nearEOF. Generic bounded property copying does not reserve suffix room. Use capacity-aware construction/rejection.  Canonical disposition: one explicitly enumerated core prefix/suffix capacity family, separate from Raspberry Pi FND-000023.

Evidence: [core-report.md](core-report.md); original alias CORE-3. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-004

`indigo_client.c:572-599` (596): snprintf result compared to1, so ordinary valid multi-character numeric output reports FAILED. Header contract client.h165. Accept nonnegative in-capacity output, test regular numeric formats as well as sexagesimal. Root independently confirmed and owns ordinary functional check. Short-format suffix pointer arithmetic also lacks length guard; do not reproduce memory fault.

Evidence: [core-report.md](core-report.md); original alias CORE-4. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-005

`indigo_stretch.cpp:233-234,273-275`: parameter computation may emit midtones0.5, while stretch divides by2*midtones-1. Ordinary identity setting transforms [0,64,128,255] into allzero in existing library. Fix algebra/explicit midpoint branch; test identity and zero dynamic range. Related AWB616-656 chooses B when R=G>B because strict comparisons; allzero totals cause0/0.  Bayer-corner defect remains old audit010 duplicate. AWB reference/totals behavior is a distinct canonical finding014.

Evidence: [core-report.md](core-report.md); original alias CORE-5. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-006

`indigo_uni_io.c:87-89`; public handle struct is index,type,fd-union but standard handle positional initializers supply FILE,1,0 / FILE,2,0. stdout is COM descriptor0 and stderrTCP descriptor0; generated standalone mains use stdin/stdout XML adapters. Use designated type/fd/index initialization. Source-only; no stream writes performed.

Evidence: [core-report.md](core-report.md); original alias CORE-6. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-007

`indigo_json.c:510-533` checks fresh backslash before existing is_escaped; escaped backslash is never emitted, ordinary Windows paths lose separators and terminal escaped backslash can confuse closing quote. Same file encoder627ff and browser JSON.stringify emit escaped backslashes. Process escaped byte first, then fresh escape; test standard string roundtrips. Unicode escapes also unsupported but not separately claimed.

Evidence: [core-report.md](core-report.md); original alias CORE-7. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-008

`indigo_mount_driver.c:425-451`, alignment_points capacity100 `indigo_mount_driver.h:479,505`. Read/sscanf results ignored, count can be uninitialized/outside capacity, then assigned to context and property counts and used for indexing. Local truncated/stale file is sufficient. Parse transactionally with bounded count/complete finite rows; publish only after success. No bad-file reproduction.

Evidence: [core-report.md](core-report.md); original alias CORE-8. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-009

`indigo_driver.c:1695` selected-profile loop probes test_config_file(...,0,...), independent of selectedi. With saved nonzero profile and absent profile0, CONFIG_LOAD disappears; start_config_restore724 returns early when count1. Probe selectedi; test absent0/present1 and converse.

Evidence: [core-report.md](core-report.md); original alias CORE-9. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-010

public wrappers `indigo_bus.c:2328,2349` format into tmp then hand tmp to another formatted function; new FITS-header path `indigo_ccd_driver.c:862` hands writable header value directly as init_text_item format. Header length<=58 check does not alter format semantics. Public snoop text helpers `indigo_agent.c:79-82,97-100` and server service text initialization also use data as format. Use raw setters after initial formatting; tests ordinary percent-bearing data.  Active registry dedup found no existing text-reformat record; legacyprintf016 concerns returned length and is distinct.

Evidence: [core-report.md](core-report.md); original alias CORE-10. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-011

`indigo_ccd_driver.c:1420` create_file_name uses PATH_MAX intermediate and no destination capacity; copies1693/1791 into callers' INDIGO_VALUE_SIZE arrays2332/2479/2561. Independent bounded dir/prefix and expansions can exceed caller capacity. `%J`: buffer2[15]1558 passed strftime max32 at1559 (ordinary output16+NUL). `%C`:50byte buffer1605 copies permitted<=58byte text1609; `%P`15byte strncpy1618 lacks guaranteed NUL. Bound every append/expansion and pass output capacity; no unsafe runtime input used. No active filename-engine record located.

Evidence: [core-report.md](core-report.md); original alias CORE-11. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-012

`indigo_ccd_driver.c:1829-1834`: for48bit input and little_endianfalse, swaps size=width*height uint16 samples while RGB48 has3*size. Remaining two thirds byte-reversed before output/preview conversion. Iterate channels*samples, ordinary multichannel endian tests. No active endian record located; source-only.

Evidence: [core-report.md](core-report.md); original alias CORE-12. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-013

`indigo_bus.c:2736-2738` indigo_atod forces parsed zero to1 when exponent present. Normal0e0=>1,0e5=>100000,-0e3=>-1000. XML input and metadata handlers use this API. Remove forced1; test zero exponent forms and ordinary nonzero. Existing-library functional output agrees with source; no malformed input involved.

Evidence: [core-report.md](core-report.md); original alias CORE-13. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-014

indigo_libs/indigo_stretch.cpp:616-670 uses strict largest-channel comparisons. R=G>B selects B as reference; all-zero totals divide by zero. Select a stable maximal channel and handle zero totals explicitly. Verify tied maxima, black input and ordinary RGB24/RGB48 data. Source-confirmed, no runtime AWB test.

Evidence: [core-report.md](core-report.md); original alias CORE-AWB. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-015

indigo_libs/indigo_mount_driver.c:1193,1250,1273,1329 contains MULTI_POINT success stubs without assigning output coordinates. Mount simulator exposes all four alignment choices and uses translations for motion/park/home. Implement the contract or hide/reject unsupported mode and test every direction. Source and live caller reachability confirmed by two reviewers; no movement executed.

Evidence: [math-closure-report.md](math-closure-report.md); original alias CORE-MULTI. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-016

- Location: indigo_drivers/aux_usbdp/indigo_aux_usbdp.driver:103-149, especially 105-106 and 146-149.
- Severity: medium; confidence: high; category: functional/resource lifecycle. Owner: coordinator; status: open; nonblocking audit follow-up.
- Evidence: after serial open succeeds, cleanup at line 146 is inside the successful `usbdp_command(UDP_IDENTIFY_CMD)` branch. Failed writes/reads return false at 149 with PRIVATE_DATA->handle still live. Targeted inspection of generated aux_connection_handler at indigo_aux_usbdp.c:447-478 confirms the open-false branch publishes ALERT/disconnected without invoking close; the next connect overwrites the stored handle.
- Impact: failed identification and reconnect can leak serial handles/resources and interfere with a subsequent connection. No runtime failure was induced.
- Minimal remediation: make all unsuccessful open paths close the acquired handle before returning false; retain normal connected shutdown ownership. Add simulator failed-identify/reconnect resource assertions, bump driver source version and regenerate unchanged generator outputs.
- Deduplication: no matching active record found; unlike FND-000048 (UPB queued-property completion), this is USB Dewpoint resource ownership.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-1. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-017

- Location: indigo_drivers/shared/primalucelib/primalucelib.c:374-405, specifically 394-399; parser acceptance at 253-265; capacity in primalucelib.h:77-78,502-503.
- Severity: medium; confidence: high from source; category: security/input validation. Owner: coordinator; status: open; nonblocking audit follow-up.
- Evidence: parser accepts every positive jsmn return, including all 1024 allocated tokens; it does not retain the returned token count. The missing-key/subtree skip increments index and reads `tokens[index]` until an undefined sentinel or sibling is found, with no capacity/count bound. A completely occupied valid token array has no undefined sentinel after its last token. Searching for an absent field after a final nested subtree can therefore advance beyond tokens[1023]. The 8192-byte response buffer allows a token-dense JSON response to fill the token array.
- Prerequisite/impact: an abnormal controller reply reaching the shared JSON library can cause an out-of-bounds read and crash/undefined behavior. This is a source-boundary finding, not a demonstrated exploit or remote code execution claim. No payload or reproduction was created.
- Minimal remediation: store the positive parser token count and bound start/index/recursive traversal before every token access; return absent for traversal at end. Add focused parser boundary regression coverage for full token arrays and absent nested fields.
- Deduplication: active FND-000052 covers unescaped outbound WiFi credentials in this library, not token traversal bounds; no matching active record found.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-2. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-018

- Location: indigo_drivers/aux_svbpowerbox/indigo_aux_svbpowerbox.driver:141-157, especially 147-156.
- Severity: medium; confidence: high; category: functional/availability. Owner: coordinator; status: open; nonblocking audit follow-up.
- Evidence: comment promises "up to 3 seconds, 300 ms silence", but the loop is conditioned only on repeated 300000-microsecond waits. No overall deadline, byte cap or iteration limit exists. Successful arriving data resets the effective wait indefinitely.
- Impact: a controller in a reset/logging loop or continuous serial noise can keep the serialized connect handler active indefinitely, postponing disconnect and other operations. No hardware interaction or induced failure occurred.
- Minimal remediation: enforce the documented monotonic overall boot-drain deadline, detect failed reads, fail transactionally with handle cleanup when continuous data exceeds it, and add simulator continuous-boot-data/reconnect regression coverage; bump version and regenerate outputs.
- Deduplication: no matching active record found; existing FBC/FlipFlat and Pentax unbounded-operation findings concern different protocols and functions.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-3. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-019

- Location: indigo_drivers/ccd_playerone/indigo_ccd_playerone.driver:413; finish behavior at 335-355. Cross-component contract: indigo_libs/indigo_ccd_driver.c:1806-1815,2316-2328.
- Severity: medium; confidence: high; category: functional/image delivery. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: the shared snapshot/stream acquisition finalizer passes literal `true` for indigo_process_image's final `streaming` argument, even when PRIVATE_DATA->streaming is false. The base framework applies streaming update throttling and selects AVI/SER file output only when this argument is true. acquisition_finish finalizes video only when PRIVATE_DATA->streaming is actually true.
- Impact: single exposures can be throttled like streams and JPEG_AVI/RAW_SER single exposures can create video files without the ordinary single-image/finalization behavior. SDK continuous-acquisition workarounds do not make a public single exposure a streaming operation.
- Minimal remediation: pass the actual public operation's streaming flag to image processing; test snapshots with update limits and both video-capable format choices, while preserving the Saturn SDK workaround; bump source version and regenerate. No active duplicate found; no runtime reproduction performed.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-4. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-020

- Location: indigo_drivers/ccd_simulator/indigo_ccd_simulator.driver:761-771. Cross-component contract: indigo_libs/indigo_uni_io.c:1995-2027; allocation semantics indigo_libs/indigo_bus.c:1795-1802 and indigo/indigo_bus.h:1041-1047.
- Severity: medium; confidence: high; category: functional/data validation. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: header and pixel indigo_uni_read return values are converted to bool. That API returns a short positive byte count at EOF and -1 on error, both truthy. Shape/signature arithmetic checks do not prove bytes were actually read.
- Impact: truncated pixel input can connect as a valid file camera and produce full-size images with a zero-filled unread tail. Allocation is zero-initialized, so this finding does NOT claim uninitialized memory disclosure. No runtime reproduction performed.
- Minimal remediation: compare header and payload reads to exact requested lengths, require representable API lengths, and keep failed-open cleanup. Add truncated header/pixel and read-error regression fixtures, bump simulator driver version and regenerate.
- Deduplication: existing audit-022 targets agent_imager/alpaca consumers, not this file simulator's read-count bug; register separately.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-5. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-021

- Location: indigo_drivers/aux_giotto/indigo_aux_giotto.driver:89-91,133-145,239-240,272. Generated call-site check: indigo_aux_giotto.c:740-743 and441-477.
- Severity: medium; confidence: high; category: functional/concurrency. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: after panel_get_state I/O, a changed state or recovery from poll_alert calls giotto_publish_light, overwriting the light switch and brightness value AND target, then sets both properties OK. No per-property BUSY/pending guard exists. A concurrent bus-thread change sets BUSY and stores the request before its queued handler runs; that handler later reads the same overwritten switch or number.target.
- Impact: an external panel-state change or poll-alert recovery concurrent with a client request can replace that request with old readback, resulting in a successful handler sending a different value. No race reproduction performed.
- Minimal remediation: adopt unsolicited/readback values only into properties with no pending change; preserve each property's request target and BUSY state until its handler completes. Add delayed-poll overlap tests, bump version, regenerate. Existing FND-000048 concerns aux_upb; no GIOTTO duplicate found.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-6. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-022

- Location: indigo_drivers/ao_sx/ao_sx_simulator/ao_sx_simulator.c:87-94,230-235.
- Severity: low; confidence: high; category: functional/simulator memory boundary. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: --firmware accepts arbitrary-length text; firmware reply is snprintf'd into buffer[8], then sim_send uses the returned would-have-written length without a capacity check.
- Impact: an overly long local firmware option makes the simulator read past the reply buffer, emitting unrelated stack bytes or failing. This requires local simulator configuration, not remote product input. No reproduction performed.
- Minimal remediation: validate the firmware reply length or clamp/reject snprintf truncation before sending; add boundary-option tests. Distinct from active AO simulator axis-limit audit-045.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-7. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-023

- Locations: indigo_drivers/aux_flipflat/aux_flipflat_simulator/aux_flipflat_simulator.ino:87-90; indigo_drivers/dome_nexdome3/dome_nexdome3_simulator/dome_nexdome3_simulator.ino:80,271,276-279.
- Severity: low; confidence: high; category: functional/simulator input boundary. Owner: coordinator; status: open; nonblocking follow-up.
- Evidence: FlipFlat zeroes a 32-byte array, then lets readBytesUntil fill all32 bytes and calls strcmp without appending NUL. NexDome3 declares ReceiveBuffer[COMMAND_LENGTH], accepts COMMAND_LENGTH data bytes and then writes NUL at index COMMAND_LENGTH when the terminator arrives.
- Impact: full-size abnormal serial input can make FlipFlat compare beyond its local buffer or make NexDome3 write one byte beyond the receive array. These are manual firmware simulators, not the portable production driver. No flashing/hardware/reproduction performed.
- Minimal remediation: reserve and explicitly write the terminator using the received count; enlarge NexDome3 storage or reduce accepted payload capacity; reject and drain overlong commands. No matching active finding found; shared boundary-contract repair can be one follow-up or split per driver by coordinator.
Also see FM-09 in driver-fm-report.md for the same full-buffer termination contract in manual iOptron/LX200/Rainbow/SynScan simulators. Repair all listed receive boundaries together; no firmware flashing authorized.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-8. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-024

Severity medium; confidence high; functional/security input-boundary finding. `indigo_drivers/ccd_pentax/indigo_ccd_pentax.c:551-568,582-596,605-613,621-655` obtain a uint32 length from camera readiness, allocate `uint8_t buffer[length]`, pass length to the uint16 `pentax_read` argument at503, and decode identity/firmware/full-state offsets without a minimum payload check. Oversized replies can exhaust the stack; short or length-truncated transfers can leave the subsequent fixed offsets outside valid input. The SCSI helper399-447 also does not require an exact data-stage count before accepting CSW success. Distinct from active020 and021 (open/readiness). Minimal fix: bounded response storage, exact transfer count, reject unsupported lengths before allocation/narrowing, validate all model-state offsets against received bytes; fake USB short/large status regressions. Source-only evidence, no adversarial fixture or reproduction executed.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-9. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-025

Severity low; confidence high; resource ownership. `indigo_drivers/ccd_pentax/indigo_ccd_pentax.c:1138-1149` initializes `private_data=NULL` and never assigns it; after detaching/unref it frees that NULL and frees only the device. Allocation at1113 remains leaked on every unplug and driver shutdown routed through unplug. Minimal fix: capture `device->private_data` before detach and free that captured allocation exactly once; fake repeated attach/unplug ownership check. No matching active finding identified.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-10. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-026

Severity medium; confidence high; input bounds. Generic descriptor `indigo_drivers/ccd_ptp/indigo_ptp.c:751-814` stores unbounded enum counts then writes `value.sw.values[i]`; capacity is1024 at `indigo_ptp.h:325`. Text enums only decode first16 but preserve larger `target->count`, later indexed through that count by update at1506-1512 and1556 onward; text storage is16 at header329. Sony duplicates numeric enum writes at `indigo_ptp_sony.c:1072-1132`, and its extension-info classifier1325-1349 appends received entries to1024-element local collections without caps. Canon `indigo_ptp_canon.c:1036-1074` validates group count/size only by assert but does not validate group ID before indexing `ex_func_group[16][1024]`, record extent, or item count; its updated pointer list has unchecked appends1071 and1176 despite outer record framing. The generic positive count/fixed-array issue and Canon group/list issue may merit separate follow-up records. Minimal fix: bounded cursor with remaining bytes plus explicit destination capacity and terminator reserve, validate group indices and counts before modifying state; reject malformed descriptors atomically. Related existing019 covers DeviceInfo decoders, not descriptor destinations or vendor group indices; extend its scope explicitly or register linked findings, do not silently call it a duplicate. Source-only; no exploit/payload/reproduction. Canonical scope explicitly includes generic/Sony descriptor enum storage, Sony extension classifier arrays and Canon nested group/updated-list destinations as subcases of one PTP destination-capacity repair family. Existing DeviceInfo019 remains separate.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-11. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-027

Severity medium; confidence high; security/functional length contract. Generic `indigo_drivers/ccd_ptp/indigo_ptp.c:1718-1737` reads advertised image size from ObjectInfo, then GetObject uses `data_in_size=NULL` and forwards the prior metadata size to preview/image. Sony `indigo_ptp_sony.c:1410-1437` and Fuji `indigo_ptp_fuji.c:379-392` repeat this. Transport allocates from actual data-container length (`indigo_ptp.c:1387-1410`, with1024 padding on raw USB; ICA allocates exact input length1150-1156). If metadata exceeds received length/padding, native publication or local save reads beyond that allocation. The sink trusts data_size for file write and blob size (`indigo_libs/indigo_ccd_driver.c:2610,2629-2632`). Minimal fix: request/retain actual downloaded length, validate metadata independently, and pass only verified actual bytes to all consumers. Add fake transport mismatched metadata/object tests. Distinct from active019 DeviceInfo parsing. No reproduction executed.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-12. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-028

Severity medium; confidence high; length/termination. `indigo_drivers/ccd_ptp/indigo_ptp_canon.c:1534-1540` copies256 characters with strncpy into a256-byte payload without forcing NUL, then strlen derives the transfer length. The caller1545-1547 passes the framework text item's value, whose capacity is larger than256; a valid long text property is sufficient to lose termination. This can overread stack bytes and use that inflated length for the USB/ICA outgoing copy. Minimal fix: reject an overlong PTP text value or reserve and write a terminator with a checked explicit encoded length; test longest accepted property and one-too-long. No matching active finding identified. Source-only, no payload/reproduction.

Evidence: [driver-ac-report.md](driver-ac-report.md); original alias AC-13. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-029

Category security; severity medium; confidence high. Owner coordinator; open, nonblocking audit follow-up.

Location: `indigo_drivers/focuser_primaluce/indigo_focuser_primaluce.driver:570-573`. The application firmware is copied with INDIGO_COPY_VALUE into INFO_DEVICE_FW_REVISION_ITEM->text.value (512 bytes); the code then appends ` / ` and the web firmware string using unchecked strcat. Even a first string truncated to511 leaves no room for the separator. The second string comes directly from an8192-byte device reply (`shared/primalucelib/primalucelib.h:77,502`; `primalucelib.c:416-421` returns the decoded token pointer without a per-value cap). This is a malformed/oversized device-reply memory overwrite, distinct from FND-000052 concerning client WiFi JSON escaping. Fix by formatting the combined version into a bounded destination, preserving values before any shared-buffer reuse. Verify large individual/combined firmware fields via local fake transport; no oversized reply was generated here.

Evidence: [driver-fm-report.md](driver-fm-report.md); original alias FM-1. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-030

Category functional; severity medium; confidence high. Owner coordinator; open, nonblocking audit follow-up.

Location: `indigo_optional_drivers/ccd_rpi/indigo_ccd_rpi.cpp:1693-1694` and`:1766`; type`:932-941` contains `std::string eid`. indigo_safe_malloc provides zeroed raw storage, not C++ construction, but attach immediately assigns to eid. This invokes a member of a non-live std::string, with implementation-dependent startup crash/invalid storage behavior. free at shutdown also skips the string destructor and can leak its heap allocation. Existing FND-000019–000027 cover different optional-driver failures, not this C++ lifetime mismatch. Use value-initializing new rpi_private_data{} and matching delete, or a deliberate placement-construction/destruction contract. Verify attach/detach with short and non-SSO camera IDs under a fake libcamera backend; Raspberry Pi/real libcamera unavailable in this read-only review.

Evidence: [driver-fm-report.md](driver-fm-report.md); original alias FM-2. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-031

Category security; severity medium; confidence high. Owner coordinator; open, nonblocking audit follow-up.

Location: `indigo_drivers/agent_alpaca/indigo_agent_alpaca.c:1210-1217`. Camera Bayer property is allocated for ALPACA_MAX_ITEMS128 at`:1000`. When all slots are occupied, cam_number equals count128 and the code still forms items+128, writes label/name and increments count. The earlier main device mapping check at`:1186` rejects more than128 devices but does not leave the enclosing CCD branch, so it does not guard this independent array. Distinct camera names can also accumulate through discovery/session churn. This causes out-of-bounds heap writes and subsequent over-count property publication. Unlike existing FND-...-035 (core native registry >= bug), this is a separate Alpaca property mapping contract. Reject/omit a new Bayer entry at capacity before forming the item pointer; preserve existing camera mappings and ensure an unmapped camera has no invalid Bayer pointer. Verify127/128/full+one mappings and remove/re-add behavior with synthetic properties. No capacity stress was executed.

Evidence: [driver-fm-report.md](driver-fm-report.md); original alias FM-3. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-032

Category functional; severity medium; confidence high. Owner coordinator; open, nonblocking audit follow-up.

Location: `indigo_drivers/mount_ioptron/indigo_mount_ioptron.driver:2264-2279`. For HC8406, both guide finalizers call ioptron_no_reply_command(:Qn#/:Qe#) and discard its boolean result, zero pulse values and publish unconditional OK. The pulse start path`:1087-1115` uses indefinite directional commands for this protocol rather than device-timed pulses, so the final stop is the event that ends physical guiding. A failed transport stop leaves movement unconfirmed while reporting success. The helper`:212` already returns a failure signal; use it to publish ALERT and retain/recover uncertain motion status. Disconnect/abort should not erase a failed-stop state without reconciliation. Compare other mount finalizers (lx2004498 etc.) that already use their stop result. Existing GPIO finalizer finding ...repository-audit-011 is a separate driver/sink. Verify both axes, stop write failure and healthy subsequent recovery with simulator transport. No hardware pulse accuracy was measured.

Evidence: [driver-fm-report.md](driver-fm-report.md); original alias FM-4. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-033

Category functional; severity medium; confidence high. Owner coordinator; open, nonblocking audit follow-up.

Location: `indigo_drivers/agent_imager/indigo_agent_imager.c:1944-1958`; caller`:2203-2218`. With at least four equal valid per-star minima, standard deviation is zero and strict `focus_diff < 3 * best_focus_stddev` excludes every value, leaving used_values0. `best_focus /= used_values` returns NaN; caller computes steps from it and sends the resulting focus movement without checking finiteness. This is a derived-data defect, not generic numeric change_property input validation. Distinct from existing masked-contrast FND-...-032. Handle zero variance as a valid common minimum and reject/fallback when no values survive; validate final computed focus before motion. Add numerical unit cases for equal values, ordinary outliers and no usable minima. No movement was commanded.

Evidence: [driver-fm-report.md](driver-fm-report.md); original alias FM-5. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-034

Category functional; severity low; confidence high. Owner coordinator; open, nonblocking audit follow-up.

Location: `indigo_drivers/agent_guider/indigo_agent_guider.c:2782-2786`,`:3267-3305`. AGENT_GUIDER_RESET_MKGP_PROPERTY is allocated and exposed, but agent_device_detach releases RESET_PPEC and all the other custom property allocations while omitting RESET_MKGP. The macro points to the independent `agent_reset_mkgp_property` field, not an alias of RESET_PPEC. Repeated agent instances/load/unload leak one property per instance. Whole-agent source allocation/release pairing found this sole custom uppercase-property mismatch. Release this property alongside RESET_PPEC after stopping callbacks. Verify repeated attach/detach allocation accounting.

Evidence: [driver-fm-report.md](driver-fm-report.md); original alias FM-6. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-035

Category security; severity medium; confidence high from source lifetime contract;

Location: `indigo_drivers/agent_scripting/indigo_agent_scripting.c:455-463`,`:477-483`,`:499`,`:725`. Define/redefine/update callbacks receive the live cached property pointer as queued data. A later definition of the same named property resizes/reallocates that same cached object when more items are required; `indigo_resize_property` (`indigo_libs/indigo_bus.c:1605-1607`) calls realloc for non-BLOB growth. Pending handlers still hold the previous pointer. `execute_script:1046-1058` evaluates ordinary script calls synchronously under the scripting mutex, while `indigo_execute_handler_with_data` enqueues work (`indigo_driver.c:2266-2268`, queue contract2221-2230). There is no property copy, reference-counted stable indirection or queue drain before mutation. A normal define/update followed by larger redefine can therefore leave a queued callback dereferencing freed storage; even when realloc stays in place, earlier notifications observe the later mutated property instead of their requested snapshot. The same pattern exists for text/number/switch/light cache definitions. Serialize cache changes and publication under the same queue or give each queued operation owned stable data, preserving the final cache/bus ownership contract. Add local valid-script lifecycle/notification regressions; no reproduction or offensive payload was created. Independently confirmed by the separate security reviewer against queue and realloc ownership.

Evidence: [driver-fm-report.md](driver-fm-report.md); original alias FM-7. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-036

Category functional; severity medium; confidence high. Owner coordinator; open, nonblocking audit follow-up.

Location: `indigo_mac_drivers/focuser_mjkzz_bt/indigo_focuser_mjkzz_bt.m:208-217`. The notification callback checks only that at least one complete message exists and address is0x81, then immediately applies decoded position/speed/stop data; it never checks ucSUM or the response command high bit. The authored portable driver for this same message protocol explicitly validates address, response command and checksum (`indigo_drivers/focuser_mjkzz/indigo_focuser_mjkzz.driver:82-98`; protocol rationale at89-90). A complete malformed/stale notification can change measured position or publish successful motion completion even when the protocol rejects its integrity. Validate the complete response frame with the shared message helpers before applying it, preserving only documented protocol flexibility; characterize invalid checksum/address/command and healthy notification cases. This audit did not connect Bluetooth or inject a notification. CoreBluetooth transport integrity is not a substitute for checking the device's application framing contract.

Evidence: [driver-fm-report.md](driver-fm-report.md); original alias FM-8. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-037

Category security; severity medium; confidence high; owner coordinator; open, nonblocking follow-up.

`indigo_libs/indigo_raw_utils.c:1237-1248` rejects only nonpositive include widths/heights. It then allocates projection vectors with length next_power_2(include_width/height) at1360ff and runs calibrate_re1206-1230. BG_RADIUS is5 at1072; calibrate_re unconditionally writes vector[0..5] and starts another edge-zeroing loop at size-5, independent of allocation length. Region width/height1–4 therefore produces a vector allocation of1/2/4 elements but out-of-bounds writes before FFT. For a length1 transform `_fft:318-348` has only n==2 as the recursion base, so1 recurses through0 indefinitely. RGB projection filtering also indexes include_width-2/include_height-2 for a one-column/row region. These are valid positive region dimensions accepted by the public function and by agent_guider validate_include_region878-933; the DONUTS include-region path at998ff passes these values. A bright varied narrow region can pass the max/threshold gate. Validate the algorithm's actual minimum window and frame-bounded include rectangle before any reads/allocation; define a clean INDIGO_FAILED/GUIDE_ERROR result for too-small regions or implement small-window algorithms safely. Verify minimum-1/minimum and valid narrowframe/region behavior with synthetic local images under sanitizers. No crash/memory-overflow test was executed in this audit.

Additional small-image star-search boundary to assess in the same algorithm-minimum repair or a separate scoped record: indigo_find_stars_precise_threshold2594-2599 uses clip_edge=height/4 below threshold. At height1–3 this is0, and local-max checks at2804ff read off-1/off-width/diagonals for border candidates. Width is not independently guaranteed to have an interior. Caller RAW shape validation permits positive dimensions, so formal shape validity alone does not establish a safe neighbor stencil. Distinct from prior Bayer012 and PSF007 records.

Evidence: [math-closure-report.md](math-closure-report.md); original alias MATH-1. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-038

Category functional; severity medium; confidence high; owner coordinator; open, nonblocking follow-up.

`indigo_libs/indigo_align.c:702-710` solves the two equations using v=(d3*k1-d2*k3)/(k4*k1-k2*k3) then u=(d2-v*k2)/k1. When the first two hour angles are symmetric around the meridian (e.g -15°, +15°, followed by +45°), k1=cos(a2)-cos(a1)=0 although the determinant can be nonzero and the system remains solvable. The final division yields nonfinite u while returning true; near-symmetry is also poorly conditioned. This is a legitimate three-distinct-point polar alignment geometry. The actual platesolver reference3 path685 ignores the boolean and694-698 publishes the resulting state OK, passing error values into target coordinate generation692-693. Solve both outputs directly using the determinant (or choose a stable pivot), reject genuinely singular/nonfinite results, and require the caller to honor failure before publishing a target/OK. Add numerical symmetric-meridian and truly degenerate geometry cases. No mount move or solver was run.

Evidence: [math-closure-report.md](math-closure-report.md); original alias MATH-2. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-039

Category functional; severity medium; confidence high; owner coordinator; open, nonblocking follow-up.

`indigo_libs/indigo_platesolver.c:275-329` waits up to60s for Imager capture BUSY/ALERT or1s for Guider process BUSY/ALERT. After either loop it rejects only ALERT; a missing/ignored request whose state remains IDLE/OK falls through to `Capture started` and true. `start_process:477-515` therefore leaves a solve/polar operation waiting for an image even though no fresh capture was accepted. Require observed request acceptance/completion, and return failure/ALERT on bounded acceptance timeout, preserving abort behavior. Characterize absent BUSY, immediate ALERT, accepted capture and fast completed capture without stale state confusion. No long wait or fake protocol reproduction was performed.

Evidence: [math-closure-report.md](math-closure-report.md); original alias MATH-3. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-040

tools/run_driver_test.py:109 promises no file changes, while dry-run suppresses only final README/summary records. Build, temporary files and requested simulator/hardware tests still execute. Correct the epilog to describe record preview; keep current execution semantics. Source-confirmed; no driver run performed.

Evidence: [root-closure-report.md](root-closure-report.md); original alias ROOT-1. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-041

tools/alpaca_omnisim_conformu.py:111-114 starts a child then waits for readiness before returning it. A readiness exception prevents caller assignment at290, so finally stops None and leaves the child/session alive. Establish ownership before waiting or clean up inside start_omnisim on exception. A pure fake-Popen/readiness test confirms ownership loss; no actual child/network operation.

Evidence: [root-closure-report.md](root-closure-report.md); original alias ROOT-2. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-042

`indigo_server/resource/mount.html:96,101,106,111` bind only element-local mousedown/up; moveWest562-566 sendsWESTtrue andstopWest568-571 sendsfalse, analogous East573-582/North584-593/South595-604. No mouseleave, windowmouseup, pointercapture, pointercancel, lostpointercapture, blur orvisibilitychange stop lifecycle exists in mount.html/components.js/indigo.js (targeted grep confirmed onlyfour local mouseupbindings). An ordinary press thenrelease outside button leaves movement asserted whileWebSocket stays connected. Framework client-detach abort consequently doesnot cover this trigger; no actual mount operation was run. Add pointer capture and unified release/cancel/blur/lostcapture stop, with synthetic UI event regression forrelease outside andcancel; preserve explicit Stop button andnetwork detach behavior. Distinct from prior network-client-detach finding;  Canonical disposition: distinct from disconnect handling; no active duplicate found. Root independently inspected element-local release handlers and sending functions.

Evidence: [core-report.md](core-report.md); original alias CORE-BROWSER. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-043

The test-designer role points to nonexistent tests/AGENTS.md instead of indigo_test/AGENTS.md; documenter points to nonexistent docs/AGENTS.md instead of indigo_docs/AGENTS.md. The task-archive routine unconditionally promises idle/coordinator reset although archive_task.py:121-124 applies it only to legacy v1; modern task state remains completed. These mismatches can skip scoped instructions or misdiagnose correct completion state. Correct paths and qualify legacy behavior without changing helper semantics. Source-confirmed, no lifecycle mutation for this diagnostic.

Evidence: [root-documentation-closure.md](root-documentation-closure.md); original alias ROOT-FACTORY. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.

## FND-20261024121920-3bd2-repository-audit-044

indigo_docs/INDIGO_IMAGER_MULTICAMERA_SYNCHRONIZATION.md:126,160 claims controlled followers skip inter-frame delay/post-delay. indigo_agent_imager.c:1370-1394 gates only dithering; PRE_DELAY, delay and POST_DELAY1395-1425 remain unconditional. Users can configure timing/breakpoints expecting an absent bypass. Document current dithering-only bypass and actual delay behavior; specify any desired behavior change separately. Source-only comparison; no camera or driver run.

Evidence: [root-documentation-closure.md](root-documentation-closure.md); original alias ROOT-FOLLOWER. Confidence: high source-contract evidence, with execution limits as stated. Next accountable action: coordinator assigns minimal repair and the listed regression checks. Status open; audit-artifact gate nonblocking, product remediation required.
