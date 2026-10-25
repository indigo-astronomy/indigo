# Test and regression plan

Status: final

Thirteen new fake USB cases in `test_ccd_ptp_sdk` (also built with ASan/UBSan by `test-ccd-ptp-sdk-sanitize`):

| Case | Acceptance | Fault |
| --- | --- | --- |
| `device_info_truncated` | AC-001 | DeviceInfo ends after the format arrays |
| `device_info_exact_buffer` | AC-001 | decoder on an exact-size heap buffer (ICA delivery), truncated after formats and inside an array |
| `device_info_short_array` | AC-001 | property array count beyond the reply |
| `device_info_oversized_array` | AC-001 | 1100 event codes |
| `descriptor_oversized_enum` | AC-002 | 1100 numeric enum values, 20 string enum values |
| `descriptor_truncated_enum` | AC-002 | enum count 10 with 4 values |
| `sony_oversized_extension_info` | AC-003 | 1100 operation codes in Sony extension info |
| `canon_oversized_device_info_ex` | AC-003 | 1100 event codes in GetDeviceInfoEx |
| `canon_custom_func_ex_bounds` | AC-004 | valid record, group id 0, group id 17, group past record, item count past group, value size past group, group count 17 |
| `object_size_mismatch_nikon`, `_sony`, `_fuji` | AC-005 | ObjectInfo size +4096 and -100 bytes, 20-byte ObjectInfo |
| `canon_long_text_property` | AC-006 | 300-byte and 6-byte text values |

AC-007: full recorded run and sanitizer run. Pre-fix evidence is in verification.md.
