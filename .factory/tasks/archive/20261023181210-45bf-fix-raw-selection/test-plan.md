# Test plan

AC-001: table-driven positive-radius edge rejection in every format, invalid radii/dimensions/coordinates and null required pointers; output sentinels preserved. New tests must fail on baseline.
AC-002: complete edge-touching ROI returns success (or guide error for a constant frame), interior synthetic star retains centroid/PSF.
AC-003: radius-two 5x5 border with four 65535 and twelve zero samples has mean 16383.75 and standard deviation about 28377; thresholds exceed max, so HFD/FWHM must both be diameter 5, peak 49151.25. Also exercise fractional variance with an 8-bit border.

Hardware-free native unit tests using test_runner.h and linked production library; sanitizer object compilation does not include production C directly. Existing full unit suite may hit known FND-000047 ENODEV/ENOTTY mismatch, which is not silently waived.
