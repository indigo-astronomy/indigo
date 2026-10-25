# Plan

1. Compare the prose against parse_state and the light handlers in indigo_xml.c, plus CCD_LENS and CCD_LENS_FOV state assignments.
2. Correct the protocol paragraph and enum comment only.
3. Inspect the final diff, perform separate verification/quality/security passes, and record the explicit no-tests instruction.
4. Archive evidence and prepare one task commit; integration and publication require separate owner authorization.

Risk: low, text-only. No dependencies, interface, state transition or trust-boundary changes.
