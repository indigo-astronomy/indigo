# Specification

The public converter accepts a byte buffer of declared length, a reusable output allocation and optional sentinel-terminated keyword array. Preserve signature support RAW1/RAW2/RAW3/RAW6 and mono/RGB planar FITS pixel values.

- AC-001: Reject NULL required arguments, short/negative input lengths, unsupported signatures, zero/oversized dimensions, multiplication overflow and incomplete full-component payload before reading pixels. Return INDIGO_FAILED with errno set.
- AC-002: Produce complete 80-byte cards and END followed by record-aligned data; grow the header in 2880-byte records when metadata crosses a record boundary. Preserve all valid metadata rather than dropping excess cards.
- AC-003: Bound embedded extension parsing by the full pixel payload and supplied input length, supporting SIMPLE=T;KEY=value; with optional final NUL. Reject malformed recognized extensions and unrepresentable metadata without buffer overrun. Ignore unrelated trailing bytes as before. Caller keyword names must fit eight characters and formatted values/cards must fit one FITS card; reject oversized cards rather than silently changing values.
- AC-004: Preserve the previous output pointer, bytes and size on validation/allocation failure. Success returns record-aligned size representable by int. Preserve mono/RGB output and unsigned-16 conversion; accept unaligned input safely.

Complexity: medium/high, because header/data placement, RGB payload offset, arithmetic and parsing jointly enforce memory bounds. Risk: high memory safety impact. No ABI changes/dependencies; no properties or driver behavior changed. Existing sentinel termination and valid C string pointers remain caller contracts. Input and reusable output are distinct allocations.
