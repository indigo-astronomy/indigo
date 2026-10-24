# Test plan

AC-001: One stopped-short idle regression per affected driver, with successful target arrival and applicable reverse/backlash/abort/external-motion cases retained. Run serial simulator or fake SDK suites; record exact counts and unavailable platforms.
AC-002: Shared RAW shape cases exercise all supported formats, truncated headers/pixels, unsupported signature, zero and huge dimensions, and trailing data. Consumer checks exercise real image conversion and preview where practicable. Run sanitizer-supported tests.
AC-003: Synthetic camera discovery fills all 128 mappings, attempts another camera, and rediscovers an existing camera; inspect count, item identities and mapping behavior. No external hardware/network service is required.
AC-004: Version diffs, generation reproducibility, whitespace, findings/decision/project validation, configured Factory tests/security, final review and committed-range process gate.
