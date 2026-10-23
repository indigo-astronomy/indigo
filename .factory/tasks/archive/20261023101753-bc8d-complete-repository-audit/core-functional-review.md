# Supplemental core, server and browser functional review

Verdict: ordinary functional/contract source coverage complete for the assigned components; eight nonblocking functional follow-ups proposed below. **SERVER SECURITY is explicitly excluded at the owner's direction.** No penetration, malicious-input, exploit, live-service or defensive security verification was performed in this supplement. This report does not qualify excluded security work or review the uncommitted defensive proposals.

## Revision, scope and method

Production was read only from committed base `f019b004769072275a3d98a87c8e9740a42c19a4` with `git show`, so concurrently present defensive source changes cannot be confused with audited production. Writes are limited to this report and `core-functional-coverage.json`. Root owns canonical findings and final disposition. Explicitly loaded the repository-reviewer role/routine in the driver assignment and scoped `indigo_libs/AGENTS.md` for this supplement.

The initial ledger identifies exact committed blobs and ordinary contract traces for bus events/properties, JSON/XML adapters and serialization, native loading/connections, session/token functional state, server management/routes/resources, all seven authored HTML applications, shared JavaScript/components and owned style/build integration. Every referenced base blob is unchanged from prior audit outcome `0cf556f8f`. Prior analyzer/native results can therefore be retained for precisely their original source/environment scope; they are not new tests by this reviewer.

Review follows actual ordinary call paths, lifetime ownership and public events rather than treating source search as proof. It is proportionate contract review, not an assertion that every line or every possible device/browser interaction was exercised. Vendor/minified JavaScript, fonts, catalog data and generated `.data` output are dependency/generated exclusions. No hardware, network service, GUI browser, Linux/Windows build or shared-library build was run.

## Contract conclusions

- Bus start/stop owns registries, background queue and session pseudo-device. Device/client attach and detach delegate callbacks with per-client generation references for detach-abort tracking. Known publish-before-attach window remains FND-000013. Property definitions, updates, deletion and messages route to client callbacks; changed-item/state suppression and per-client force flags are intentional. Write-only updates hide values; BLOB cache owns copied image content. Public change convenience functions route through the same bus, and target-only numeric copy differs deliberately from replacing actual values.
- JSON/XML output retains writable-number target/value separation, five vector types, state/messages, remote-reshare setting and per-item update flags. XML version translation, remote service suffixes, cached remote property lifetime and BLOB base64/URL modes have distinct documented compatibility behavior. JSON is the browser-facing client command/output path; its command subset is not automatically a missing XML parity feature.
- Protocol adapter constructors borrow their I/O handles and own their allocated bus/context objects. Server worker and remote-client threads detach adapters before freeing them and close the borrowed transport afterward. XML device-adapter release also frees client-specific BLOB subscription records.
- Native dynamic-driver loading records metadata and initialization outcome, preserves a driver that refuses shutdown, and unloads its dynamic library only after success. Remote-server and subprocess threads own reconnect/backoff and parser-adapter session lifetime. The capacity boundary follow-up below affects ordinary API error reporting. Process-wide library tracking is not an invitation to remove duplicate guards or retries without compatibility tests.
- Session features are intended to be per-client; the shared mutable property leaks the other client's defaults into partial updates (C-02). Token functions were examined only for ordinary add/remove/fallback and file persistence contracts; no authorization or adversarial assessment is claimed.
- Server driver requests are copied into a dedicated queue; pending-request count keeps DRIVERS BUSY through the final queued request. Management property release, subprocess reaping, listener shutdown, signal supervisor and static resource registration were traced. RPi management actions were read as existing product behavior and never executed. Static UNLOAD has a stale initialized flag (C-06).
- HTTP review is limited to ordinary XML/JSON worker selection, BLOB/resource/handler routing and object ownership. Buffered BLOB responses own a working copy; unbuffered responses hold the entry lock; resources borrow caller-owned static payload/path metadata and own list records. Existing gzip truncation finding FND-20261023065605-60d4-repository-audit-009 remains the canonical issue. No security claim is made about HTTP parsing, WebSocket framing, hostile requests, access control or server deployment.
- Browser shared state handles WebSocket opening, initial enumeration, property definitions/partial updates, deletion and reconnection. Generic controls dispatch typed property requests; component lifetime removes resize/image listeners and disconnects ResizeObserver; graph/map overlays are product behavior rather than unused scaffolding. Scripting dirty-state avoids writing untouched text, and sequence execution obtains the canonical agent scripting library resource. Imager/guider/solver preview and related-agent wiring, mount current-vs-target/map coordinates, and manager control actions were traced to their actual named properties. Conversion and failed-download follow-ups below are concrete functional issues. No browser rendering pass is claimed.

## Proposed canonical follow-ups

Root/coordinator owns final IDs, gate, severity and scoped repair assignment. All are proposed nonblocking audit follow-ups with coordinator as accountable follow-up owner; that disposition does not certify a release or accept data-loss risk. No production repair was performed in this read-only review.

### C-01 — Native registries fail to reject their exact capacity boundary

Severity: medium; confidence: high; functional/API capacity defect.

`indigo_libs/indigo_client.c:209`, `:400` and `:530` test `empty_slot > INDIGO_MAX_DRIVERS/SERVERS`. Arrays are declared with exactly those capacities (`:75`, `:313`, `:441`; public header defines256 drivers and10 servers). With no reusable slot, empty_slot equals the used count, so the257th distinct driver or11th live remote server/subprocess bypasses the intended TOO_MANY_ELEMENTS return and indexes the element after the registry. This is an ordinary full-capacity contract mismatch; no capacity stress or malicious-input run was performed.

Remediation: reject `>=` before any indexed access, preserving reuse of gaps and duplicate semantics; add bounded unit fixtures at capacity-1, capacity and after removal. Treat three registries together but do not rewrite unrelated connection behavior.

### C-02 — Partial SESSION_FEATURES update copies another client's setting

Severity: medium; confidence: high; functional session-state defect.

`indigo_libs/indigo_session.c:51-67` stores feature values in one shared property. Enumeration writes both fields from the enumerating client, but a change copies only named request items into that shared property and then copies **both** resulting values into the requesting client. For example A has FORCE_ITEM_UPDATES=true; B enumerates with both flags false; A sends only FORCE_PROPERTY_UPDATES=true. A's unmentioned FORCE_ITEM_UPDATES is reset to B's false value. Partial named-item updates are the normal bus contract, and these flags explicitly belong to the requesting client.

Remediation: seed the shared response values from the requesting client under appropriate ownership before applying a partial update, or derive each client field independently from present request items; publish only to that client. Unit regression interleaves two clients' enumeration and one-item updates. No cross-client runtime test was run here.

### C-03 — Browser sexagesimal conversion loses the sign below one degree

Severity: medium; confidence: high; reproduced functional coordinate defect.

`indigo_server/resource/indigo.js:462-490`: dtos truncates signed degrees with `d | 0`, losing the minus sign when -1 < value <0. stod determines the sign using `parseInt(degrees) <0`, which is false for negative zero. Actual Node execution of the committed functions returned `dtos(-0.5) == "0:30:00"` and `stod("-0:30:00") == +0.5`.

`components.js:1124-1184` uses these functions for sexagesimal editing/display, and generic property controls use them at1418/1426/1445. `mount.html` uses those controls and dtos for picked target coordinates. Declinations/latitudes just south of zero can therefore be displayed or sent north of zero.

Remediation: preserve the original sign separately from integer degree magnitude in formatting and parse the leading string sign; test positive/negative sub-degree values, zero, nonzero integer degrees and round-trip precision in ordinary coordinate widgets.

### C-04 — Browser HOST_TIME emits malformed fractional-hour offsets

Severity: medium; confidence: high; reproduced functional time-setting defect.

`indigo_server/resource/indigo.js:493-509` divides timezone minutes by60 and concatenates `'00'`, instead of formatting separate whole hours and remainder minutes. With getTimezoneOffset()=-330 the actual committed function produces suffix `+05.500`, whereas an ISO numeric UTC+05:30 offset is `+0530` (or `+05:30`). Every authored page calls timestamp() for initially empty Server.HOST_TIME; server setter `indigo_server.c:1540-1550` forwards the value to its existing date-management helper. Fractional-hour locales can fail first-time host clock setup.

Remediation: format signed absolute hours and minutes separately, or use a validated UTC representation supported by the helper; regress ±30/45-minute offsets and ordinary whole-hour timezones. The test replaced only Date.getTimezoneOffset in an isolated VM; no system clock was changed.

### C-05 — Imager deletes original file after an HTTP error response

Severity: high; confidence: high; reproduced functional data-preservation defect.

`indigo_server/resource/imager.html:507-519` handles xhr.onload by clicking a download link for any response and then sending AGENT_IMAGER_DELETE_FILE with the original filename. onload runs for completed HTTP404/500 responses too, and this callback does not inspect status. An ordinary expired/missing BLOB or server failure can therefore download an error body and then request deletion of the original image. This is a normal failure-handling issue; no server-security test was involved.

The exact committed callback was executed in Node24.19.0 with a synthetic XMLHttpRequest status404 and Blob('BLOB not found!'), mocked DOM/blob URL, and captured changeProperty calls. It clicked the download and emitted `["Imager Agent","AGENT_IMAGER_DELETE_FILE",{"FILE":"exposure.fits"}]`. No network request, browser download or actual file deletion occurred.

Remediation: require a successful and complete image response before advancing/deleting, preserve the original and present an error/retry path on HTTP/transport/abort failures, and avoid relying on mutable global filename during asynchronous completion. Regression tests must show404/500/abort never enqueue deletion and successful transfer maintains the established sequence.

### C-06 — Static driver UNLOAD does not clear initialized state

Severity: medium; confidence: high; functional driver-manager lifecycle defect.

`indigo_server/indigo_server.c:1378-1386`: dynamic unload uses indigo_remove_driver, but static unload directly calls the entry point's SHUTDOWN and changes the visible DRIVERS switch without setting the registry entry's initialized=false. A subsequent DRIVERS enable at1275 requires `!driver->initialized` before calling INIT. Thus a statically linked driver unloaded through UNLOAD can appear re-enabled while its devices remain absent. The normal DRIVERS-disable branch already clears initialized after successful SHUTDOWN, so the alternate path is inconsistent.

Remediation: clear static initialized only after successful shutdown, preserve it on BUSY/failure, and regress UNLOAD followed by DRIVERS enable on a bounded stub static driver. No actual driver was unloaded during the audit.

## Functional reproducer evidence

Working directory: `/Users/polakop/Development/indigo`. Source: the pinned base above. Runtime: `node --version` returned `v24.19.0`.

Conversion run used Node `vm` with the unmodified source suffix beginning at `function dtos(`, extracted with `git show f019b004:indigo_server/resource/indigo.js`; Date subclass overrides only getTimezoneOffset to return -330. Results are recorded in C-03/C-04 and the coverage ledger. No browser/global application setup was executed.

Download run used the exact source slice from `xhr.onload = function () {` to the next `xhr.send();` in committed imager.html, an isolated `vm` context with mocked DOM/URL/changeProperty/setTimeout/download and xhr.status404. The first extraction attempt used a whitespace-sensitive terminator and failed with a fixture SyntaxError; correcting the extraction to the actual next xhr.send boundary produced the captured C-05 result. That setup failure was not a product failure or a passing application test.

Other findings are source-contract proofs, not fabricated runtime results. No new app/SDK/platform pass is claimed. Root must reconcile these references with canonical findings and preserve the owner's explicit server-security exclusion in the audit qualification statement.

## Supplemental base framework contracts

The coordinator additionally assigned every path in remaining-framework.json: 61 source/header paths. All now have an individual identity and semantic-contract entry in core-functional-coverage.json. Together with the original server/UI assignment the ledger contains 74 unique paths. Source was read at the pinned base using git show; no uncommitted defensive proposal was used as audit evidence. This is proportionate component/source-contract inspection, not a claim to have proved every line or executed every public API.

Common base-driver review traced attach/enumerate/change/detach and header property/item offsets for AO, AUX, CCD, dome, focuser, GPS, guider, mount, polar alignment, rotator and wheel. Base classes allocate and publish capabilities; concrete drivers retain hardware transport and operation completion responsibility. CCD exposure countdown runs on the background handler and is stopped by cleanup; local/client/BOTH image routes and format/preview/stream resources were inspected. The exact-length write contract led to C-07. Common additional-instance ownership led to C-08. Wheel base's 16-item allocation supports the independently recorded B-01 concrete-driver count finding.

Common driver serial discovery/claim ownership was traced through hotplug event coalescing, bus-locked list/claim updates, master-group sharing, claim expiry and physical port loss. Configuration restoration records incoming file-order properties and asynchronously waits for acknowledgement with a bounded total deadline. Normal queue execution shares master ownership and task mutexes; cancellation removes pending work and detach releases the queue/background observer/timers. Timer implementation and public header confirm nonpreemptive serial callbacks: priority does not interrupt a running handler, and maximum run time is diagnostic. This supports B-04's SynScan finalizer follow-up without treating every bounded synchronous transaction as a defect.

Filter selection was traced through connection, configuration restore, enumeration and READY, including timeout rollback and cache release. Proxy names are translated back on outgoing changes; text updates deep copy item values, while cached source and agent property lifetimes follow definitions/deletions. Public deprecated cached-property lookup retains its documented limitations. Platesolver source was followed through copied/downloaded image tasks, generation abort checks, separately asynchronous abort, related-agent state updates, solve/sync/center and polar-alignment epoch conversions, plus property/preview teardown. Derived solvers still own executable shutdown concurrency; no external solver was launched.

Base64 triples, padding and pair-table indexing and MD5 block/finalization/prefix ownership were inspected as ordinary encoding/digest functionality. No cryptographic assessment or malicious parser testing was performed. Version translation preserves legacy adapter naming semantics. Public build/config/names headers and class context/property macros were reconciled with implementation roles. Generated lookup data was treated as an implementation dependency; no claim of exhaustive numeric table certification or big-endian runtime validation is made.

### C-07 — CCD local image saves accept failed or short writes

Severity: medium; confidence: high; functional persistence/error reporting follow-up; nonblocking for audit evidence completion.

indigo_libs/indigo_ccd_driver.c:2367 and2487 use `!indigo_uni_write(...)`. The callee at indigo_libs/indigo_uni_io.c:2071-2115 returns a byte count, -1 on write error, or a positive short count when a later write returns zero. Both -1 and positive short counts convert to true, so their logical negation is false. A normal disk-full/write failure therefore keeps CCD_IMAGE_FILE OK and preserves a truncated/failed output; cleanup only removes a file after ALERT. Other local DSLR save branches must be included in the same remediation.

This is a separate caller of the return contract covered by FND-20261023101753-bc8d-complete-repository-audit-004 for AVI/SER writers; reconcile or extend that record rather than losing the CCD paths or duplicating its underlying condition without distinction. Compare the return value with the exact requested byte count, publish ALERT and remove incomplete output on failure, and inject ordinary write-error/short-write results in a bounded fixture. No actual disk-full or camera test was run here.

### C-08 — Additional slave instances set the base pointer on their master

Severity: medium; confidence: high; functional multi-device lifecycle follow-up; nonblocking for audit evidence completion.

indigo_libs/indigo_driver.c:1830-1865 constructs an additional master with its correct original master base at1847, then copies each original slave. At1863 it writes `additional_device->base_device = slave_device` rather than `additional_slave_device->base_device = slave_device`. Consequently the master's original base identity is overwritten for every slave, while the cloned slave retains the original slave's copied base_device (commonly NULL). Class attach routines use base_device to suppress additional-instance creation and distinguish original/cloned device state; master/slave clone identity no longer matches the intended original pair.

Set the cloned slave's base_device, preserve the additional master's base pointer, and regress a master with at least one slave through creation, enumeration, removal and re-creation, asserting both originals and shared private-data ownership. No production instance or hardware lifecycle was mutated in this review. No matching active finding was found in the targeted reconciliation read.

## Candidate classification and qualification

The mount base contains unfinished MULTI_POINT transform branches that return OK without assigning output values, but the alignment property is hidden by default. This is an exposure-dependent source candidate; no new confirmed active-use finding is claimed without tracing a maintained driver that exposes this option. Existing old commented encoder-distance code is inactive; nearest-point execution uses spherical angular separation. MD5 file-prefix helper has no read-error return channel in its existing void API; that API limitation alone is not classified as a new confirmed defect. Platesolver bounded waiting and separate async abort differ from the SynScan same-queue starvation condition; no duplicate starvation finding is proposed.

Ordinary functional source/contract coverage is complete for this assigned manifest. Server security is explicitly excluded by owner instruction and has not been assessed or declared passing. The report cannot independently qualify a full repository audit including that excluded area. Platform/SDK/hardware readiness remains subject to the coordinator's retained evidence and documented limitations; these source findings remain open follow-ups until fixed and reverified.
