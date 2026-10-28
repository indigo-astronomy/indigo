# Repaired findings

## FND-20261027184448-45d5-pentax-rpi-single-frame-001: Packed nonlegacy control arguments

Split argument writes and missing aperture word caused K-m control rejection. Nonlegacy arguments are packed; legacy writes preserved. Strict protocol regression and real M controls pass.

Reverification: verification.md, timing-review.md and retained regression logs. Status: fixed in the final software candidate; physical timing limitation is explicit.

## FND-20261027184448-45d5-pentax-rpi-single-frame-002: Initial ISO selection survives connection

Connection rebuilt all ISO switches false, losing the original800 selection and breaking restore. Initialize selected item from configured ISO; regression and hardware restore pass.

Reverification: verification.md, timing-review.md and retained regression logs. Status: fixed in the final software candidate; physical timing limitation is explicit.

## FND-20261027184448-45d5-pentax-rpi-single-frame-003: Wait for asynchronous setting readback

Aperture acknowledgement preceded application, producing false failures. Poll bounded full-state readback. Three-read delayed application fake and real aperture exercise pass.

Reverification: verification.md, timing-review.md and retained regression logs. Status: fixed in the final software candidate; physical timing limitation is explicit.

## FND-20261027184448-45d5-pentax-rpi-single-frame-004: Restore original physical Bulb safely

B may report0/0 timed shutter, causing capture rejection or invalid restoration. Accept nonnumeric B and skip numeric shutter restore for B. Fake B0/0 and real bulb profile pass.

Reverification: verification.md, timing-review.md and retained regression logs. Status: fixed in the final software candidate; physical timing limitation is explicit.

## FND-20261027184448-45d5-pentax-rpi-single-frame-005: Bulb deadline excludes readiness latency

Deadline after full-press readiness extended1.2s to1.408s in fake and1.42s in physical RAW metadata. Timestamp command dispatch after staging; fake44/44 and sanitizer44/44 verify both bounds and overdue close. Final physical accuracy intentionally untested under owner offline instruction.

Reverification: verification.md, timing-review.md and retained regression logs. Status: fixed in the final software candidate; physical timing limitation is explicit.
