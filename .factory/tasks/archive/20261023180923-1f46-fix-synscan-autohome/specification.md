# Specification

AC-001: AUTOHOME preserves encoder disable, RA/DEC initial stop and counter setup, per-axis index preparation (including extra offset/reverse/dwell when required), dual-axis forward search, ten-degree backoff, index return and final counter/DEC offset setup in original order.

AC-002: Every motion/index wait and three-second dwell yields the serialized queue through a named finalizer. Abort and disconnect execute during every stage within a bounded transport transaction budget; they cancel future home work and send actual emergency stops to both axes. No later start/counter command may escape cancellation.

AC-003: Device/protocol errors, malformed replies, missing responses and motion/index deadlines end AUTOHOME ALERT with its switch cleared, homed false and both axes stopped best effort. Success reports OK and homed state after readback. Reconnect/restart cannot resume cancelled work.

AC-004: Preserve existing mount/guider/aux, tracking and queued property request contracts. No public properties added/removed; no generator implementation change.

High complexity: dual-axis asynchronous ordering and cancellation. High operational risk: physical motor movement. Simulator validation only; hardware validation requires future explicit authorization and cannot be claimed here.
