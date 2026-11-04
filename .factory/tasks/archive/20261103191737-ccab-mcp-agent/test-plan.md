# Revised first-increment tests

Acceptance for this owner-revised step:
- POST uses the existing callback and passes the exact POST method; PUT and GET remain compatible.
- Headers and Content-Length body bytes, including JSON whitespace and binary/newline content, remain available to the callback exactly as with PUT. Fragmented bodies are readable by the callback.
- Existing prefix routing remains; endpoint exactness is enforced by a test handler, including raw query behavior.
- A callback that fully consumes input can return true to serve a second request on the same connection. Returning false still closes it.
- Removal drains an in-flight POST callback using existing retirement synchronization.
- POST cannot publish a built-in BLOB upload and unsupported static/missing POST routes terminate rather than reparse a body as another request.
- Existing HTTP security/BLOB/GET/PUT retirement and WebSocket cases remain passing.

The previous AC-002/003 dedicated-parser promises are superseded by explicit owner direction. Transport header preflight remains in force; callback body validation, deadlines, confidentiality and authorization are responsibilities of the future MCP handler, not asserted implemented by this dispatch change. Use synthetic local sockets only. No full MCP/client/hardware or unavailable-platform certification.

## Skeleton coverage

S-001/002: info/init failure without started bus, idempotent lifecycle, default-disabled/enable/disable, exact path and unsupported methods, unload/reload and in-flight body/drain behavior. S-003: lengths/duplicates/TE/Expect/media/Accept/Origin errors, fragmented/partial/oversized and stalled bodies, bounded nesting/token count/concurrency, no pipeline replay. S-004: initialize and server capability/version output, initialized notification202, ping, empty tools/list, string/numeric IDs, escaped keys, unknown methods, invalid envelopes, strict JSON grammar/UTF-8 and stale/trailing tokens. No real device commands or private configs; no hardware. S-005: focused build plus platform project membership review, unavailable platforms explicitly unverified.

## Read-tools acceptance
Test tool catalog/schema, exact target/argument validation and error codes; full device names, RO values and separate number targets, partial/reordered updates and state-only updates; dynamic text/BLOB ownership; redefinition/property/device deletion and reappearance; WO/hidden omission and readable PASSWORD/token inclusion; BLOB URL metadata, original-item local URL identity and no payload copy/fetch/base64; long text/output beyond64KiB with exact Content-Length; concurrent requests and unload/reload. Reuse existing transport/lifecycle cases and recorded driver runner; no hardware or real-client validation claim.

## Write acceptance
WO schema discovery must include item metadata while get/serialized values remain unavailable; update previous whole-WO discovery omission accordingly. Test typed text/number/switch partial requests including explicit0/false/emptytext, exactUnicode names, rejected unknown/duplicate/type/RO/hidden/light/BLOB/empty targets beforebus callbacks, optional token syntax/no masterfallback, BUSY then observed completion, same-thread unload BUSY and own CONNECTION disable. Use synthetic devices only. Result proves dispatch attempt only; lock rejection can return busOK with zero callback effects.
