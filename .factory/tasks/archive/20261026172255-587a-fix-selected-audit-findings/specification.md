# Specification

- AC-001: Additional master/slave clones preserve original base identities, slave-to-cloned-master linkage and per-instance shared private-data ownership through create/enumerate/remove/recreate; connected clones cannot be removed.
- AC-002: Small-frame 8/16-bit Bayer equalization samples fixed parities with even steps, including repeated tiles and saturation handling.
- AC-003: Guider detach releases its independently allocated MKGP reset property after callbacks stop.
- AC-004: Nutation at a new date is independent of prior cached date results.
- AC-005: LibRaw initialization failure returns a defined allocation error without dereference or close of NULL.
- AC-006: Solar aberration converts 20.4898 arcseconds to degrees correctly; epoch checks preserve unit conventions.
- AC-007: Passive discovery closes its socket after bind failure on POSIX and Windows; successful discovery behavior is preserved.
- AC-008: Editor mapping recognizes explicit logical-device identities, including inherited CCD properties, without generator or generated-source changes.
- AC-009: SHA-1 byte assembly/padding uses defined unsigned shifts and preserves known WebSocket handshake output under UBSan.

Complexity: high for lifecycle ownership and cross-component verification; medium for remaining bounded numerical/mapping fixes. Risk: medium functional/resource correctness, reversible source edits. No public ABI change or new dependency. Tests are non-interactive and hardware-free; the guider check is a scoped allocation lifecycle regression, not a full hardware/driver certification. No unresolved product questions.
