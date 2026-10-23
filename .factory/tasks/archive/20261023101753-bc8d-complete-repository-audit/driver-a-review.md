# Driver A source/contract review

Status: source/contract coverage complete for the assigned78components; changes required are nonblocking maintenance follow-ups. Reviewed revision: `f019b004769072275a3d98a87c8e9740a42c19a4`. Independent read-only reviewer `complete_driver_a`; no production changes, shared builds, hardware or external probes. Loaded repository-reviewer, repository-review routine, root/scoped driver instructions and current task specification/project map.

The per-component source identity and contract ledger is [driver-a-coverage.json](driver-a-coverage.json). All78assigned component contracts have now been reviewed. The ledger also identifies local simulator/shared helper contracts and their source identities. Review is proportional control-flow/call-site inspection of each component, not a claim that every vendor table or generated boilerplate line was examined. Vendored Duktape, SDKs, generated boilerplate and the five owner-prohibited drivers are excluded. Component review follows lifecycle ownership, asynchronous completion/abort/disconnect and device-reply/property contracts; source review is not physical-device or platform certification.

Prior partial audit source identities were compared to this base. Assigned production driver sources are unchanged since the retained repair/audit commit; prior generation/native diagnostics can be reused at their original scope, but do not count as semantic review or new runtime passes.

## Confirmed defensive maintenance observations

- `indigo_drivers/aux_fbc/indigo_aux_fbc.driver:131-148,162-179`: requested light impulse/shutter duration is completed by sleeping in a loop in the serialized handler, up to30s. Disconnect and other queued requests wait behind the loop. Medium functional, nonblocking existing behavior; use deadline+delayed finalizers and fake-serial interruption tests.
- `indigo_drivers/aux_flipflat/indigo_aux_flipflat.driver:151-171`: cover movement sleeps/polls up to30s in its handler, delaying disconnect/other requests. Medium functional, nonblocking; delayed bounded status finalizer with stop/disconnect scenarios.
- `indigo_drivers/ccd_ptp/indigo_ptp.c:453-519,542-557,1657-1668`: device-info transaction payload length is discarded before unbounded primitive/string/array decoding. Short SDK/device payloads can read outside received data; array count guard is only assert. Medium input-validation/security follow-up, nonblocking within this audit; bounded cursor parsing and malformed-payload fake transport regressions required. An isolated exact-helper sanitizer diagnostic already obtained is retained in driver-a-evidence; no full device or network exploitation claim, no further crash reproductions.

## Existing finding reconciliation

- FND-000014: old divide-by-zero path removed. Shared `rpio_sysfs_get_pwm` now checks reads and period>0 (lines418-440); callers check result before adopting values.
- FND-000015: old unterminated read removed. `rpio_sysfs_read_text:39-52` reads at mostsize-1 and terminates at returned count.
- FND-000016: early-stop cleanup removed by best-effort `rpio_sysfs_unexport_all`, but no ownership ledger distinguishes already-exported resources. Keep open/refine: rollback/disconnect can unexport resources pre-existing before this session.
- FND-000017: old relay/port mutex members removed by generated drivers; shared sysfs backend has no such mutex allocation. Historical leak shape is obsolete.
- FND-000018: current property init calls use `indigo_safe_malloc` (`indigo_bus.h:1004-1014`), which returns zeroed storage or terminates; it does not return NULL. Historical null-return partial-attach premise is not reachable under this API contract. Do not invent a recoverable allocation failure.
- FND-000019/20: Andor Initialize failure still breaks while holding driver_mutex; global mutex still destroyed inside per-device shutdown. Source-current, existing nonblocking high findings retained, no SDK run.
- FND-000021-27: RPi unbounded readiness/condition wait, start failure BUSY, external fixed-buffer copies, unchecked mmap, mode-capacity growth, advanced-property reconnect leak and duplicate focuser-name uniqueness remain source-current. Keep existing records.
- FND-000007: guide subframe restore sends change then clears saved state and relies on capture; original failure not rerun. Keep existing concrete regression finding; source inspection is not a passing retest.
- FND-000048: UPB focuser SI poll completion still owns BUSY states without distinguishing queued starts. Keep existing race finding.
- FND-20261023065605-60d4-repository-audit-011: relay-pulse finalizers still ignore failed OFF writes and clear deadlines. Keep source-confirmed follow-up.

Canonical finding closure/allocation belongs to the coordinator. No risk acceptance is implied.

## Additional confirmed maintenance observations

- `indigo_drivers/ccd_pentax/indigo_ccd_pentax.c:823-828`: `libusb_open` return is overwritten before checked and its handle is immediately used. A failed USB open must terminate the attempt before a subsequent SDK call. Medium functional, nonblocking; check result and reset handle, then fake USB open-failure regression.
- `indigo_drivers/ccd_pentax/indigo_ccd_pentax.c:457-500`: `pentax_status` polls successful-but-not-ready replies indefinitely. It is reached by connect/command/status work and holds that work until readiness arrives, preventing a bounded failure response and potentially stalling disconnect. Medium functional, nonblocking; monotonic deadline/cancellation plus permanently-not-ready fake reply case.
- `indigo_drivers/agent_imager/indigo_agent_imager.c:577-670` (`capture_frame`, especially649 onward) and `indigo_drivers/agent_alpaca/indigo_alpaca_ccd.c:1385-1490`: received RAW image content is cast/read without first checking header size and pixel span from dimensions. Imager then passes dimensions to RAW analysis; Alpaca loops over that span for imagearray conversion. `indigo_validate_blob` verifies the registered entry, not the image shape. Medium defensive input-validation/security, nonblocking; use the header/signature/positive dimension/overflow/pixel-span checks already present in `agent_guider` and propagate failure to callers. Coordinator should consolidate with framework RAW findings if they cover these caller boundaries. No new failure reproduction was attempted for these observations.

## Final assigned-area coverage and limitations

Coverage includes all71portable driver/agent directories from the assignment plus2Linux,3macOS and2optional driver components. Generated definitions own production behavior; OEM selectors share the reviewed ToupTek implementation. Associated local simulator/helper contracts were inspected for their command framing/dispatch, model state, fault hooks, bounded buffers and ready-file ownership. This is component-level source/contract audit coverage, with ordinary proportional call-path examination; it does not establish exhaustive path/model checking.

The only fresh diagnostic run by this reviewer was the already-retained isolated PTP helper diagnostic; its exact helper was extracted unchanged from the reviewed source. It establishes the helper's missing length contract, not a complete camera or network attack. The USB transport reserves zeroed1024-byte padding, so that particular tiny helper input is not a whole-transport reproducer; device-info arrays/property/vendor decoding still lack remaining-length checks and rely on decoded counts. No further crash diagnostics or exploit development was performed after the coordinator's stop instruction.

No production edits, shared builds, hardware, service probing or canonical findings changes were performed. No vendor SDK/platform compatibility, physical guider timing or optical/mechanical behavior is certified by this report. The root coordinator must reconcile nonblocking follow-ups and independently review the final task diff before a qualified whole-repository checkpoint.
