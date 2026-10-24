# Test plan

Use indigo_test/unit/test_bus_property.c linked to the built library. Check actual destination capacity before any subsequent growth/release; old-code regression fails safely on metadata rather than deliberately exercising out-of-bounds access. Cover repeated long text replacement, source independence, self-copy, reserved inactive text cleanup, empty source and type changes. Run native test-unit and scoped sanitizer/leak checks if supported. Record exact commands, results and limits in verification.md.

## Extended scenarios

Resource lifetime: owned metadata, concurrent GET/PUT callback removal and remove-all draining, unrelated registry progress, cleanup on failures. Prefix/suffix: ordinary and exact-fit capacity behavior at audited sites. Text format: ordinary percent-bearing data preserved, no dangerous conversion payloads. CCD filenames: tiny synthetic frames with isolated output/HOME, token semantics, maximum valid FILTER/FOCUSPOS, literals containing %, sequence collision/scanning and overcapacity failure. Browser motion: all directions, outside release/cancel/capture loss/focus/visibility/page loss, duplicates/unrelated pointers, failed capture and explicit stop. Run opt-in local transport target separately, with loopback access as required. No hardware, exploit workflows or third-party requests.
