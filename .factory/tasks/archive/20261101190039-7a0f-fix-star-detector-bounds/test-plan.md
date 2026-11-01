# Test plan

AC-001/003: all six raw formats x precise/fast; dimensions 1x1, 1x144, 144x1, 2x144, 144x2, 3x3, 3x144, 144x3, 4x4, 8x8, 64x64, bright each corner and interior plus constant images. Radii 0/1/3/16/128.
AC-002/003: radius -1/INT_MIN/129/32768/INT_MAX; zero/negative dimensions and signed-product/grid-overflow geometry; all return failure before image access. Original edge reproduction returns success zero; original radius reproduction fails cleanly.
AC-004: existing 8192 median checks and 288 six-format precise comparisons with pre-optimization baseline, including radius 0. Ordinary star fields assert nonzero detection for supported refinement radii.
AC-005: independent reviewer inspects code/tests/evidence; registry and process checks validate closure. Baseline reproduction evidence is retained in task 20261101185459-950f-verify-star-detectors.
