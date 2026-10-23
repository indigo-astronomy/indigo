# Specification

The INDIGO TCP server accepts a WebSocket upgrade on `GET /` when the request carries a `Sec-WebSocket-Key` header. The server appends the RFC 6455 GUID (36 bytes) to the key and hashes the result in a fixed 256-byte buffer, so the longest key it can hold is 219 bytes.

- AC-001: A `GET /` whose `Sec-WebSocket-Key` value is 220 bytes or longer is refused with `HTTP/1.1 400 Bad Request`. The server does not answer `101 Switching Protocols`, writes nothing past the key buffer, closes that connection, and keeps accepting new connections.
- AC-002: The RFC 6455 sample key `dGhlIHNhbXBsZSBub25jZQ==` still upgrades with `101 Switching Protocols` and `Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=`.
- AC-003: A 219-byte key, the longest that fits, still upgrades with `101` and the accept value derived from the whole key. The fix adds no restriction beyond the buffer limit.
- AC-004: The real server source, built with AddressSanitizer, serves the AC-001 request without a memory error. Before the fix the same build reports a stack-buffer-overflow in the handshake.

Out of scope: strict RFC validation of key length or base64 alphabet, other HTTP handlers, and the inherited findings.
