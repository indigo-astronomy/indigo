# Specification

Status: final

## Problem and outcome
Malformed or hostile PTP replies can no longer make the driver read past a received buffer or write past a fixed-size destination, and downloaded images are published with the number of bytes actually received.

## In scope
`indigo_drivers/ccd_ptp/indigo_ptp.[ch]`, `indigo_ptp_canon.c`, `indigo_ptp_sony.c`, `indigo_ptp_fuji.c`, `indigo_ptp_olympus.c`, `indigo_ptp_nikon.c`; driver version 3.0.0.55 → 3.0.0.56.

## Out of scope
Bounded-cursor rewrite of every vendor decoder whose input has no transport size (Sony GetAllDevicePropData property walk beyond enum capacities); physical cameras.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | DeviceInfo is decoded only within the received length. A truncated reply keeps the fields decoded so far and leaves the rest empty; strings are always NUL terminated; an array longer than the destination keeps the first PTP_MAX_ELEMENTS - 1 codes. | `device_info_truncated`, `device_info_oversized_array` (fake USB, also ASan/UBSan) |
| AC-002 | A descriptor enum count larger than the destination (1024 numeric, 16 strings) keeps only the values that fit, and every value read stays within the received descriptor. | `descriptor_oversized_enum`, `descriptor_oversized_string_enum` |
| AC-003 | Sony extension info and Canon/Nikon vendor code lists never write past the supported-code arrays. | `sony_oversized_extension_info`, `canon_oversized_device_info_ex` |
| AC-004 | A Canon CustomFuncEx record with an invalid group id, group size, item count or value size outside the record is rejected without writing; valid groups still work. | `canon_custom_func_ex_bounds` |
| AC-005 | GetObject results are published and saved with the number of bytes received; a metadata size larger or smaller than the payload does not change the BLOB size. Generic, Sony and Fuji download paths. A short ObjectInfo is rejected. | `object_size_mismatch_<vendor>` |
| AC-006 | A Canon text property value longer than 255 bytes is sent truncated to 255 bytes plus a terminator. | `canon_long_text_property` |
| AC-007 | All existing fake USB and fake ICA cases still pass. | recorded run `python3 tools/run_driver_test.py ccd_ptp`, sanitizer run |

## Failure behavior and edge cases
A truncated DeviceInfo no longer fails the connection; the missing fields are empty, as an absent field would be. A rejected CustomFuncEx record sets the GeneralError last error like other malformed Canon events.

## Permissions, data, and security requirements
Camera replies are untrusted input.

## Compatibility and nonfunctional requirements
Well-formed replies decode exactly as before. macOS (ICA and USB) and Linux builds.

## Assumptions and open questions
None blocking.

## Owner decisions and requirement changes
2026-10-25: owner asked to fix all PTP findings.
