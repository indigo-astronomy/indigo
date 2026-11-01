# Specification

AC-001: Both detectors only inspect pixels with valid eight-neighbor coordinates; tiny positive images may return zero stars.
AC-002: Validate geometry and radius before reads/allocations; radius 0 remains accepted, negative values and values above the existing centroid limit 128 fail. Scratch allocation sizes use checked size_t arithmetic. Reject geometries whose existing int pixel/channel/grid indexes would overflow.
AC-003: Direct-source ASan/UBSan matrix covers all six formats, short/narrow/tiny images, bright corners/interior, invalid and huge radii/dimensions, original reproductions, normal stars and radius 0.
AC-004: Preserve ordinary precise results and quickselect median behavior against pre-optimization baseline.
AC-005: Close both canonical findings only after verification and independent review.

Complexity medium; memory-safety risk high. Radius >128 cannot produce supported refined stars through the existing centroid routine; explicitly rejecting it bounds work and matches that existing limit. No properties, drivers or ABI changes.
