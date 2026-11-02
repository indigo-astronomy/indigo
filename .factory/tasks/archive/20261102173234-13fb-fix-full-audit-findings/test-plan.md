# Test plan

Use native solver image-input/unit cases with tiny generated RAW buffers and arithmetic boundary headers; no real image recognition/index downloads. Check valid mono/RGB conversion, huge/truncated geometry, zero post-bin, detector errors/counts, Astrometry parity source contract. Run affected driver-specific fake/simulator suites in child tasks. No hardware or unavailable platform claims.

| Acceptance | Evidence strategy |
| --- | --- |
| AC-1 | Huge/zero/truncated RAW headers across four signatures, valid four-format controls and bounded native allocations. |
| AC-2 | Tiny RAW/FITS downsample2, extreme Bayer bin, unsupported detector signature and nonpositive capacity; sanitizer execution. |
| AC-3 | Existing independent synthetic CD-matrix parity cases plus Astrometry parser/hint source comparison; inspect cone-selection and corrected radius prose. |
| AC-4 | Child serial checksum/numeric/foreignACK/AP/refusal tests, SDK deadline/resource tests and test-verdict/generator failure probes; final mixed ALTO assertion/helper composition run and central lifecycle validation. |
