# Specification

AC-001: PSF and selection digest reject zero/negative radii and inclusive ROIs crossing any edge, including centers exactly width-radius/height-radius, before reading frame data or altering outputs. Reject invalid dimensions, nonfinite/unrepresentable coordinates and missing required pointers safely. Preserve the existing minimum frame-size restriction.

AC-002: All six RAW formats still accept a complete ROI whose last pixel is width-1/height-1 and compute valid interior measurements. Preserve the existing PSF no-output shortcut.

AC-003: PSF background variance uses sufficient floating precision for high-contrast mono16 borders; threshold-based HFD/FWHM decisions remain finite and correct.

No property/API/driver behavior additions, format conversion changes, PSF-map repairs or wider RAW cleanup. Complexity medium: two shared public helpers and numerical fixture design. Risk high consequence (frame memory reads), bounded reversible implementation. No unresolved product questions.
