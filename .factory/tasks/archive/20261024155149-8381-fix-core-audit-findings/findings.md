# Requested finding resolutions

All fifteen requested findings are repaired. Canonical records retain original discovery evidence and add this repair report; closure follows final verified acceptance. No risk acceptance or unrelated finding closure is included.

| Acceptance | Finding | Regression evidence | Resulting behavior |
| --- | --- | --- | --- |
| AC-001 | FND-20261024121920-3bd2-repository-audit-004 | test_bus_helpers / numeric_zero_exponents_and_format_status | Successful ordinary numeric output returns OK; truncation fails and suffix checks respect format length. |
| AC-002 | FND-20261024121920-3bd2-repository-audit-005 | test_image_math / stretch_identity_and_limits | Undivided transfer coefficients preserve identity; endpoint/midpoint limits and zero dynamic range are covered. |
| AC-003 | FND-20261024121920-3bd2-repository-audit-006 | test_uni_io / standard_stream_handles_have_file_descriptors | Designated initialization binds each standard file descriptor to a FILE handle. |
| AC-004 | FND-20261024121920-3bd2-repository-audit-007 | test_protocol_json / json_parser_preserves_escaped_text | Literal and terminal backslashes, quotes and controls roundtrip through the adapter/parser. |
| AC-005 | FND-20261024121920-3bd2-repository-audit-008 | test_mount_alignment_load | Bounded, finite/domain-validated complete files commit atomically; rejected files preserve the existing model and property metadata. |
| AC-006 | FND-20261024121920-3bd2-repository-audit-009 | test_profile_selection | Profile1-only and profile0-only fixtures confirm LOAD availability follows the selected profile. |
| AC-007 | FND-20261024121920-3bd2-repository-audit-012 | test_rgb48 | Both RGB48 pixels retain all six expected 16-bit samples after full processing. |
| AC-008 | FND-20261024121920-3bd2-repository-audit-013 | test_bus_helpers / numeric_zero_exponents_and_format_status | Zero exponent forms remain zero, including overflow-sized exponent on zero; ordinary nonzero exponent parsing is retained. |
| AC-009 | FND-20261024121920-3bd2-repository-audit-014 | test_image_math / stretch_white_balance_ties_and_black | Tied maximal channels use a stable reference and zero totals avoid division; RGB24/RGB48 covered. |
| AC-010 | FND-20261024121920-3bd2-repository-audit-033 | test_agent_imager_numerics / focus minima regressions; final imager integrations | Equal minima are retained, outliers rejected and unusable derived focus cannot reach motion. |
| AC-011 | FND-20261024121920-3bd2-repository-audit-037 | test_image_math / donuts_rejects_unsupported_regions; bounds review | Unsupported small/outside regions are rejected before projection/calibration access; post-fix sanitizer test only. |
| AC-012 | FND-20261024121920-3bd2-repository-audit-038 | test_image_math / polar_alignment_symmetric_and_singular; caller review | Determinant solution handles symmetric hour angles; singular/nonfinite results fail before caller target generation/publication. |
| AC-013 | FND-20261024121920-3bd2-repository-audit-039 | test_agent_imager_numerics / exposure acceptance regressions | Fresh BUSY/OK accepts, ALERT/timeout/abort rejects; stale states cannot imply acceptance for imager or guider. |
| AC-014 | FND-20261023101753-bc8d-complete-repository-audit-031 | test_service_discovery (Darwin) | Error callbacks avoid undefined SDK fields; requested context and reference lifetimes are covered with local peers. |
| AC-015 | FND-20261023101753-bc8d-complete-repository-audit-032 | test_image_math / contrast_empty_mask_and_valid_samples; estimator review | All four pixel formats return finite zero for no accepted samples; imager rejects nonpositive/nonfinite contrast. |

Commands, baseline and platform limitations: verification.md. Defensive trust/bounds analysis: security.md. Final diff review: quality-review.md. All listed fixed-code regressions passed; whole-repository and hardware certification are outside scope.

## Additional nonblocking test-quality follow-up

FND-20261024155149-8381-fix-core-audit-findings-001: the unchanged instance-coordination suite ignores driver shutdown and `indigo_stop()` return values (`indigo_test/integration/test_agent_imager_instances.c:416-430`), yet its footer claims results include cleanup. During the final recorded run, after the POST_CAPTURE/PRE_CAPTURE barrier case, the bus logged `can't stop, 'CCD Imager Simulator' is attached`; the child and recorder still exited successfully with 57/57 asserted cases. The earlier intermediate full run had no such message. This is confirmed weak teardown validation, not evidence that any selected production repair failed. The cleanup source is unchanged from base. Retained follow-up is open, medium quality, nonblocking, owner coordinator: verify disconnection/shutdown completion and fail the case on failed teardown in a separate scoped test-maintenance task, then rerun the full imager scope. No simulator/device lifecycle fix or risk acceptance is claimed here.
