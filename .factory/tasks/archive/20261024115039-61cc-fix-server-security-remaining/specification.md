# Specification

AC-001: Release parser-owned cached read-only XML BLOB payloads on replacement, deletion and EOF without freeing driver-owned memory.
AC-002: HTTP uploads cannot alter read-only BLOB entries; writable uploads remain bounded and are consumed by their intended property operation without reusing stale staging across operations. Existing identifiers carry device/property/item identity only; this repair preserves that contract and adds no client-authentication protocol.
AC-003: Every opened file-download handle closes after successful transfer, read failure or output failure; premature EOF cannot loop forever.
AC-004: Bound accepted workers before creation, reject overload safely, reclaim reservations after failed thread creation, and close idle initial protocol connections after a bounded deadline. Established astronomy sessions preserve idle behavior.
AC-005: XML/JSON client requests have finite item/allocation budgets, reject the over-budget request without dispatch and release partial state. Ordinary and boundary-valid requests continue to work.

Complexity: high due to shared cache ownership, parser lifetimes and concurrent listener admission. Risk: high (network-facing memory/resource handling). Existing authorization tokens retain coordination semantics. No new dependencies. Default limits must accommodate ordinary INDIGO properties and image payloads.
