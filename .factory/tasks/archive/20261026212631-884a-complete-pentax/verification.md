# Verification of Pentax software delivery

Base/local master64e965d7f21a257f073e95c74f0d253de57d8361, isolated task branch. Root implemented source; independent test designer wrote regressions and separate reviewers inspected security/quality. No claim of full hardware acceptance. The owner explicitly authorizes software delivery with physical verification deferred to Linux; see delivery-decision.md.

## Software results

- Original native universal arm64/x86_64 build and17 existing fake USB cases passed before changes. Hardware baseline discovery passed, claim failed-99 before exposures/settings.
- Latest production native build passes. Final fake USB31/31 and ASan/UBSan31/31 pass, including public timed/bulb exposure, abort/recovery and abort during gated multi-block download; retained native BLOB re-enumeration; refused control and capture-mode readback; invalid/zero shutter readback; active upload-mode recovery; repeated countdown; exact segment/data boundaries; explicit detach/claim/release/reattach rollback and errors. Independent security reviewer also ran sanitizer checks on the preceding candidate and reinspected final pairing.
- Commands: `make -C indigo_drivers/ccd_pentax -f ../../Makefile.drv all`; `python3 tools/run_driver_test.py ccd_pentax`; `make -C indigo_test build/integration/test_ccd_pentax_usb_sanitize`; `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/integration/test_ccd_pentax_usb_sanitize`. Logs retained with this task; task-root paths and trailing whitespace are normalized, command outcomes are preserved. macOS arm64 runtime, universal compilation; no Linux driver execution. LeakSanitizer unavailable on macOS, explicit ownership assertions used; not a leak certification.
- Configured check/security groups pass; Factory regression group190/190 passes. `git diff --check` and Xcode plutil syntax pass. Source research/licenses in research.md; primary tethering/transport sources in tethering-research.md.

## Physical results and limits

K-m discovery succeeds but interface acquisition fails-99 in ordinary execution, standardUSB-entitled copy, older libusb comparison, elevatedUID0 and UID0+standardUSB. Confirmed root UID means no claim that privilege alone explains remaining failure. No physical exposure, settings/readback/restoration, JPEG/PEF/DNG compatibility, bulb timing or abort acceptance passed. These hardware attempts preceded the final explicit USB ownership change; that final source has no physical acceptance evidence. Recorded elevated runner: one planned case, zero passed, README records1/0 Failed. No physical unplug, SD deletion or firmware operation. System Mass Storage binding restored after diagnostics. Persistent ChatGPT USB userclient/exclusive-access warning is a candidate conflict; not proven sole cause.

Docker Linux was started and inspected: only virtual root hubs, no K-m, no imported USB/IP device. This is a missing hardware prerequisite, not a passing Linux test. See docker.md and hardware-access.md for exact evidence/continuation.

AC-1 through AC-5 have software evidence only. AC-6 physical acceptance remains unverified and is explicitly deferred by the owner for this delivery. Other camera models, nativeRAW payloads and platforms remain unverified. No VM entitlement added. The owner now authorizes commit/merge/push of this software result and defers hardware acceptance to a later Linux task. The earlier checkpoint remains in history; closure includes archive/process evidence. Physical failures remain failures, not waived passing tests.

Verdict: pass

This passing verdict is for the owner-approved software delivery scope and its documented hardware exception, not for physical camera acceptance. Required committed-range checks remain delivery gates and will be reported externally after the final commit.
