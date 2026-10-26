# Specification

- AC-001: A concurrent bus enumeration cannot enter a newly registered device before its attach callback has returned and initialized its context/properties.
- AC-002: Attach callbacks can publish properties through the existing recursive bus mutex without self-deadlock.
- AC-003: Release the registration mutex immediately after the optional attach callback and before automatic CONFIG.LOAD. A CONFIG callback may wait for a different thread to access the bus.
- AC-004: Preserve existing device limits, optional callback behavior, callback result handling, tokens and lifecycle behavior.
- AC-005: The new regression exposes the original defect; native unit checks and relevant complete simulator driver suites pass after the fix, with exact results retained.

Complexity: high because this changes synchronization shared by all devices and requires controlled concurrent tests. Risk: medium (bus-wide hangs or premature access if incorrect); change is local and reversible. The owner selected the lock-through-attach approach after a static driver audit found no concrete driver deadlock. The known RPI management attach can hold the bus during shell queries; no runtime/platform certification is claimed. No unresolved product questions.
