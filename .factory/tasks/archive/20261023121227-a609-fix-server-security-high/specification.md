# Specification

Complexity: high (concurrent BLOB lifetime/publication and interacting streaming parser state). Risk: high (remote memory corruption/disclosure, process availability, shell/format injection). No product-policy ambiguity blocks defensive repairs; preserve documented token semantics and normal protocol interoperability.

- AC-001 (001): HTTP upload framing rejects malformed/duplicate/conflicting/unsupported lengths before allocation, enforces practical per-upload and aggregate memory budgets, handles allocation failure recoverably, and defines empty uploads.
- AC-002 (002,005): receive complete uploads privately with bounded progress/absolute deadlines outside cache locks; publish atomically only on exact completion; preserve prior bytes on failure and keep unrelated publication responsive.
- AC-003 (004): every cache reader/writer retains a live entry until done; property release retires all permission classes before item release; no unprotected borrowed item/entry fields remain in HTTP/XML consumers.
- AC-004 (008,009): management INSTALL uses only validated canonical version choices safely passed as one shell argument; helper diagnostics containing percent directives publish literally. Tests must stub execution, never install or alter the clock.
- AC-005 (011,012): XML attribute, name and entity boundaries reserve terminators and reject overflow without memory access outside buffers; valid boundary input still parses.
- AC-006 (013,017): JSON token/frame boundaries reserve terminators; WebSocket length validation occurs in uint64_t before narrowing, all frame components require exact reads, supported masking/opcode/control/fragmentation behavior is enforced safely.
- AC-007 (014): inbound XML client direction accepts only request handlers and never executes response-only handlers with a NULL adapter.
- AC-008 (015,016): inline XML BLOB decoding validates declared/decoded lengths and owns independent item storage until dispatch/cleanup, including malformed/truncated multi-item requests.
- AC-009: preserve valid XML/JSON/BLOB/WebSocket and documented access-control behavior; no edits to prohibited drivers, vendor code, generator or unrelated Xcode changes. No properties are added/removed by this scope.

Non-goals: remaining five lower-priority audit findings and unrelated findings, broad fuzzing or security certification. A directly coupled contract fix may be included with explicit evidence; never claim unrelated findings closed.

Coupled review repair: FND-20261023121227-a609-fix-server-security-high-001 requires XML upstream message text to publish literally, preserving NULL for absent messages; percent-containing define/update/delete/message regressions verify the same boundary class as audit 009. See preliminary-review.md.
