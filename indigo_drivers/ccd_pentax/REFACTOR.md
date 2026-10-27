# Pentax bounded transport repair

## Scope and baseline audit

This is a scoped repair of four audit findings, not a generated-driver migration. The hand-written C driver uses libusb mass-storage SCSI commands, camera-specific readiness and model-state offsets, a per-device mutex, a periodic state timer and USB hotplug allocation/detach. No generator input exists. Existing DSLR properties and valid protocol sequences must remain unchanged. The driver is under development; exposure/image capture and complete camera compatibility are outside this repair.

The model table supplies endian flags and state offsets. USB response lengths are untrusted. The current code uses unchecked open, infinite readiness loops, response-sized stack arrays, incomplete-transfer acceptance and a NULL unplug free. These are the four assigned audit findings. No manufacturer protocol documents are bundled; synthetic fixtures characterize the repository protocol, not physical camera compliance.

Baseline environment: macOS arm64 host, universal arm64/x86_64 driver build. From the isolated task root, `make -C indigo_drivers/ccd_pentax -f ../../Makefile.drv all` passes using copied local build/include and build/lib prerequisites and an ignored worktree-local Makefile.inc. The first attempt failed because the fresh worktree lacked build/drivers; after creating build directories it passed. No prior Pentax automated suite exists. Before production edits, the new fake USB suite built successfully and ran in --baseline mode: 11 cases, 6 passed and 5 expected regression failures. Valid open/close order, ready/delayed responses, camera errors, endian identity/firmware, full-state decode and retry exhaustion were preserved. The original successful open/close trace is OAKCLBDSBSBDSBDSBSBDSX; ready/delayed trace is BDSBDSBDSBDSBDSBDSBDSBDSBDS. Letters denote Open, Auto-detach, Kernel query, Claim, LUN, command Block, Data, Status, close X. Failures demonstrate open misuse, both infinite readiness modes, short/error transfer acceptance and unplug private-state leakage. Dangerous tiny/huge decode inputs are excluded from the original baseline; they will be exercised after repair. See the task baseline.log for exact output.

Hardware decision: no hardware testing; owner explicitly approved non-interactive fake USB on 2026-10-26. Hardware tests run/passed: 0/0. Linux, Windows, physical hotplug and real camera latency remain unverified.

## Atomic plan

1. Complete: capture successful original-driver transactions and safe dedicated defect reproductions with fake USB and fake time; record commands/results with task evidence.
2. Complete: transactional open, finite monotonic readiness, private bounded response storage, exact transfers and decode extents, corrected unplug ownership; increment version.
3. Complete: repeat regressions and sanitizers, build production, final recorded runner, register new test and this document in Xcode, inspect final diff.
4. Complete (execution/reviews; delivery pending): independent quality/security review, finding repair verification; coordinator retains final finding archival/process/commit evidence in the task record. No merge or push authorized.

## Found defects

- FND-20261023101753-bc8d-complete-repository-audit-020: unchecked libusb_open can pass an invalid handle to USB functions. Fixed by immediate checked return and NULL handle; regression: failed open retry (reproduced before repair).
- FND-20261023101753-bc8d-complete-repository-audit-021: readiness can occupy connection/status work indefinitely. Fixed by a five-second monotonic polling deadline in both modes; regression: both readiness deadlines (reproduced before repair). Individual synchronous USB transfers retain their bounded transport timeout/retry budget, so wall-clock return can exceed five seconds by one in-flight SCSI transaction.
- FND-20261024121920-3bd2-repository-audit-024: camera length drives allocation/narrowing and fixed-offset decode; partial transfers are accepted. Fixed by a reusable 65,535-byte private buffer (the 16-bit read protocol maximum), minimum identity/firmware lengths, all-model state extents, exact CBW/data/CSW transfers and CSW signature/tag/residue checks. High-byte shifts use unsigned arithmetic. Regressions: short transfers and transfer errors (reproduced before repair), response boundaries, general state boundaries, all model state bounds/layouts and CSW integrity.
- FND-20261024121920-3bd2-repository-audit-025: unplug frees a NULL local and leaks private state. Fixed by capturing private state before detach and freeing it after detach/unref; regression: unplug private ownership (reproduced before repair; repeated removal and detach-clears-pointer fixture).

- FND-20261026210137-00fb-fix-pentax-findings-001: independent security review found uninitialized or unterminated manufacturer/product strings used during hotplug. Source-confirmed, not reproduced on the original driver. Fixed with initialized PENTAX/Camera fallbacks, reserved NUL and validated string counts; hotplug descriptor names regression passes for absent, failed and maximum non-NUL replies with balanced attach/removal.

## Coverage and limitations

Tests target open/rollback, readiness/error/timeout, response and transfer boundaries, model endian/state decode and unplug ownership. Full CCD class exposure, streaming, image data, public bus concurrency, real timers, complete hotplug enumeration and all physical models are outside this narrow helper/lifecycle regression scope; this repair makes no full-driver certification claim. No guider interface is exposed, so pulse timing is inapplicable.

## Final test summary

Simulated tests run/passed: 17/17 in the repaired normal suite; 17/17 under AddressSanitizer and UndefinedBehaviorSanitizer. The final recorded `python3 tools/run_driver_test.py ccd_pentax` run passed 17/17 and wrote the 3.0.0.2 mac arm64 fake SDK record plus TEST_SUMMARY.md. Production universal build passed. Source/header dependencies are registered in the Makefile; test and REFACTOR are in Xcode navigator groups. LeakSanitizer is unavailable on this macOS host; explicit allocation/free ownership assertions cover the repaired leak. Seven compiler warnings concern existing sprintf deprecations; no broad formatting/API cleanup is included.
Hardware tests run/passed: 0/0 (not requested).

## Completion task 20261026212631-884a-complete-pentax

The original source exposed ISO writes and monitoring but no public exposure, image download or abort path. Native and original17 fake regressions passed before changes. The available K-m was discovered; physical connection failed at USB claim before camera settings or exposures.

Implemented for K-m/K-2000: checked connection rollback, program/aperture/shutter/ISO readback, timed and bulb exposure, shared monotonic countdown, queued bounded readout, new-buffer selection, JPEG/PEF/DNG transfer, abort/recovery, temporary-setting restoration, retained native BLOB ownership and paired USB detach/claim/release/reattach. Other models retain monitoring and existing ISO support; unverified capture is hidden. No generator migration or copied copyleft code.

### Steps and evidence

1. Complete: protocol/license research, original native/fake baseline and short physical baseline attempt. Task research.md and tethering-research.md identify sources and limitations.
2. Complete in software: lifecycle, controls and asynchronous capture/abort/transfer. Findings001–007 in the task findings.md were repaired and verified. USB cleanup intentionally adds explicit release/reattach compared with the original successful trace, preserving vendor command ordering while fixing failed-claim ownership.
3. Complete: production fake USB and public-bus fixtures, native hardware harness, Xcode registration and property reference. Fake31/31 and ASan/UBSan31/31 pass; configured Factory190/190, check/security and Xcode syntax pass. No VM entitlement added.
4. Software delivery authorized by the owner; physical acceptance deferred to Linux. Ordinary, standard-USB-entitled, older-libusb and UID0 variants fail claim(-99). No physical image, timing, settings restoration or native RAW compatibility passed. The hardware runner records1/0 Failed. The system Mass Storage binding was restored after privileged diagnostics. A persistent ChatGPT USB user client/exclusive-access warning is a candidate conflict, not proven sole cause. Docker Linux runs but sees only virtual hubs, no K-m or imported USB/IP device. See task verification.md, hardware-access.md and docker.md.

The owner requested non-interactive tests with short grouped camera use because batteries are limited, and no physical USB unplug. No SD-card deletion, firmware change or unfiltered USB-sharing service was performed. Preserve settings/card/old RAM data. A standalone terminal test after releasing competing USB ownership, or a real Linux USB host, remains necessary before completion. The owner subsequently authorized commit/merge/push of this software result on 2026-10-26 and deferred hardware acceptance to a later Linux task. The checkpoint remains in history, with physical failures preserved.

### Current acceptance coverage

Fake tests exercise production helpers and public connection/exposure/upload/abort requests with real queues/timers. They cover retained BLOB data after completion/abort, busy timing and repeated countdown, refused/malformed readback, bounded segments/transfers, active upload-mode recovery and explicit USB cleanup failures. Hardware class acceptance, real JPEG/PEF/DNG data, physical pulse timing, other models, multiple devices, Linux/Windows runtime and physical hotplug remain unverified. This is not full driver certification.

### Completion test summary

Latest simulated run:31 tests run,31 passed. Sanitized run:31 run,31 passed (ASan/UBSan; LeakSanitizer unavailable on macOS). Final requested hardware group:1 planned,0 passed, blocked before exposure. Docker physical driver tests:0 run/0 passed because camera is not exposed to the VM. Software delivery closes under the explicit owner-approved physical-validation deferral; physical acceptance remains unfinished.

## USB initialization follow-up 20261026223438-4f82-pentax-terminal-validation

Baseline: the owner supplied a successful 2.0.0.0 server log (identity 012d73, firmware 1.11.1.8, 412-byte status). Current 3.0.0.3 failed USB claim with -99 both in the hardware harness and the exact sudo server command, with UID 0 verified. The current fake USB suite passed31/31. No physical capture acceptance passed.

The original source enables libusb automatic detach before claim; the completion change disables it and explicitly detaches before claim. Darwin uses the same capture helper for both, but automatic detach retains the handle lock through claim and rechecks the kernel binding. The source difference is confirmed; its causal connection to physical -99 remains unproven.

Owner instruction: compare and repair USB initialization while leaving the camera asleep. No further camera access is authorized during this offline repair. Keep all previous failure records.

Plan (before production changes):
1. In progress: reproduce automatic-detach claim-failure ownership in the fake backend using the actual Darwin wrapper, not a duplicate implementation; preserve original successful open ordering.
2. Pending: restore the driver's original automatic-detach acquisition, keep checked open/LUN and bounded protocol protections, increment version. Since libusb owns auto-capture, fix its Darwin auto-claim rollback in the owning backend rather than guessing ownership from IORegistry. This narrowly necessary library change supports the requested USB initialization repair; no ABI changes or other vendor code are in scope.
3. Pending: verify no double detach/attach, failed-detach ownership, failed-claim rollback, normal release, and camera-free capture regressions; run sanitizers and rebuild actual dylibs. Compare the normalized original open/claim order.
4. Pending: independent quality/security review and evidence. Physical verification remains pending owner wake confirmation. Do not claim -99 fixed from fake tests.

### Offline repair outcome

The automatic-detach acquisition order OAKCL is restored in3.0.0.4. libusb owns automatic successful release; the driver does not detach/reattach a second time. Normal release verifies a previously present system binding, and claim/cleanup failures block immediate retry. The narrowly scoped Darwin backend fix uses successful capture ownership to undo a failed claim exactly once. Its missing rollback was reproduced before the change using the actual production wrapper and is tracked as FND-20261026223438-4f82-pentax-terminal-validation-001. No changes to public API/ABI, other driver sources or USB entitlements.

Plan steps1–3 complete in software: baseline regression captured, original ordering restored, production driver/library built;36/36 normal and36/36 ASan/UBSan tests passed. Failed-detach/no-binding paths never attach an unowned capture; failed raw claim restores owned capture; release and suppressed-reattach failures block retry. Step4 quality and security reviews passed for the software diff. Quality review also found and verified the corrected libusb source dependency (FND-20261026223438-4f82-pentax-terminal-validation-002). Physical -99 root cause remains unconfirmed.

### Latest repair test summary

Simulated tests:36 run/36 passed; sanitized:36 run/36 passed. Physical tests during this offline repair:0 run/0 passed. Earlier3.0.0.3 hardware acceptance remains1 planned/0 passed, plus the failed direct-server diagnostic. The owner asked to keep the camera asleep. Linux/Windows runtime, real device binding restoration, and current physical capture remain unverified.

## RPi connection validation 20261027181603-4710-pentax-rpi-validation

Owner provided SSH access to Linux arm64 RPi with K-m, retaining non-interactive short groups and no physical unplug. Existing prebuilt2.0.0.0 passed connection/status/disconnect. An isolated3.0.0.4 build initially lacked three current framework symbols against the old installed build; rebuilt matching current framework/server in a remote temporary directory instead of modifying the installed checkout. The matching current server/driver then passed connection and disconnection with INFO version3.0.0.4, identity012d73 and firmware1.11.1.8. No exposures requested. This verifies connection on that Linux host, not complete capture or macOS -99 resolution.

A focused Linux fake USB baseline reproduced a test portability defect: output recovery hard-coded /private/tmp, absent on Linux, causing its fixture setup/recovery to fail. Plan: use portable /tmp fixture path, expose the existing opt-in --baseline hardware harness through a Makefile argument (default remains full --run), label the baseline case explicitly, run36 fake cases, and record the owner-authorized connection-only profile with the canonical runner and a distinct model label. No production behavior change or extra physical case added; coverage inventory remains36/1. Final full-class acceptance and exposures remain deferred.

The Linux-only test fixture failure is fixed without weakening coverage (FND-20261027181603-4710-pentax-rpi-validation-001). Review also identified that an unlabelled baseline could replace a full-camera record; a pre-execution recorder guard now requires the explicit connection-baseline type or --no-record (FND-...-002). An actual-recorder pre-fix test reproduced execution under an unlabeled partial profile; all11 recorder tests pass on macOS and Linux after repair. No production driver change.

### RPi final test summary

Simulated:36 run/36 passed on Linux arm64 via canonical runner. Limited hardware connection/status baseline:1 run/1 passed with3.0.0.4, explicitly recorded as PENTAX K-m (connection baseline). Full capture/bulb/abort hardware cases during this task:0 run/0 passed. Physical unplug:0/0. The old full K-m failure record remains distinct. Compatible current framework/server and driver built only in a temporary directory; existing remote checkout and packages unchanged. Owner camera use ended after the short baseline.

Final guard regression counts:13/13 macOS andLinux; alternate Make assignment and expansion bypasses rejected. Linux fake36/36, connection-only baseline1/1. No further camera requests or full capture claim.
