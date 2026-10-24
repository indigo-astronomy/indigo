# Specification

AC-001: Property list/get/set parsing preserves ordinary names, whitespace and quote handling while rejecting overlong fields, malformed requests and more than MAX_ITEMS entries before writing outside buffers. Maximum fitting fields and MAX_ITEMS entries remain accepted.

AC-002: Remote hostname and optional port parsing uses bounded copies and rejects overlong/invalid input without connecting. Host-only requests retain the default port.

AC-003: RAW crop accepts the four documented RAW signatures, complete positive-sized images and an exactly parsed positive ROI within the image. Header/payload truncation, unsupported signature, unrepresentable byte counts and malformed/out-of-range ROI fail with nonzero exit.

AC-004: Successful crops preserve exact pixel bytes for every supported format; allocation/read/write/close failures are errors with resources released.

Complexity: medium, two independent input boundaries and size arithmetic, no concurrency. Risk: high memory safety impact, bounded local changes. No material product questions. Optional RAW metadata after the pixel payload remains permitted.
