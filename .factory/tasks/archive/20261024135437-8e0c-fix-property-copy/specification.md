# Specification

Copy assignment into an existing property must retain its actual destination allocation capacity, allocate enough storage for active source items, release previous destination text ownership (including inactive reserved items), and preserve independent long-text values. NULL destination allocation remains supported. Self-assignment must preserve the property and its text ownership. Cleared unused destination slots must be safe for later reuse/release. Source count, metadata and active items remain unchanged. Existing BLOB payload-copy semantics remain unchanged; replaced text destination dependencies must be released through existing lifecycle helpers; BLOB copies keep their existing borrowed payload semantics.

Acceptance: exercise NULL/reused destinations, source spare capacity, larger destination capacity, shrink/grow, long-to-long and long-to-short text, inactive text slots, type replacement and self-copy; all relevant native and Factory checks pass. No product property definitions change, so PROPERTIES.md is unaffected.

## Extended acceptance criteria

- AC-001: property reuse capacity/text/self-copy and borrowed BLOB semantics as above.
- AC-002: resource metadata remains valid while requests execute; removal drains active callbacks before owner teardown/unload, without holding registry lock across callback/I/O. Caller synchronization restrictions are documented. Alpaca shuts down routes before callback state.
- AC-003: all seven audited prefix/suffix families respect actual destination capacity, preserving complete suffixes or rejecting unrepresentable identity/path inputs; no silent identifier collisions.
- AC-010: already formatted/raw data containing ordinary percent characters is preserved exactly by text wrappers, snoop properties, FITS header insertion and server service setup.
- AC-011: filename construction bounds every intermediate and destination, expands inserted values as literal data, preserves documented tokens and sequence rules, and returns failure/ALERT for unrepresentable paths. Cover date/filter/focuser/numeric/sequence paths with hardware-free public CCD tests.
- AC-042: manual mount motion stops on release outside its control, pointer cancellation/loss, focus loss or hidden/unloaded page. Stop is idempotent, ignores unrelated pointers, and retains explicit abort behavior. Test without hardware/WebSocket.

No new dependencies, generator changes or changes to off-limits drivers. Framework fixes do not change driver property definitions. Alpaca lifecycle behavior repair requires its version increment and applicable scoped documentation.
