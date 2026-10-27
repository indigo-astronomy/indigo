# Specification

1. Compare original automatic-detach/claim initialization with current explicit detach and restore original acquisition ordering while preserving checked open, exact LUN validation and protocol safety.
2. Preserve successful-capture ownership when a Darwin raw interface claim fails: restore only the capture acquired by that call, once, without public API/ABI change.
3. Release only a claimed interface, avoid duplicate attach and block unsafe immediate retries after claim/cleanup uncertainty.
4. Normal local builds must rebuild the patched Darwin backend when its source changes.
5. Verify software contracts with actual-production-wrapper regressions and camera-free tests/sanitizers. Build version3.0.0.4 and the existing hardware harness without using the sleeping camera.
6. Retain failed hardware evidence and report current physical connection/capture as unverified. Complete the owner-authorized software commit/merge/push; hardware acceptance is a subsequent owner-notified follow-up.

Owner authority: prior explicit hardware deferral; offline-repair instruction to leave the camera asleep; 2026-10-27 instruction `urob commit, merge, push` after the 3.0.0.4 software result and explicit physical-validation limitation were reported.
