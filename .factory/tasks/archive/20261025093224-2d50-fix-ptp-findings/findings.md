# Findings dispositions

| Finding | Disposition | Evidence |
| --- | --- | --- |
| FND-20261023101753-bc8d-complete-repository-audit-019 | fixed | DeviceInfo decoded within the received size, strings terminated, arrays clamped; `device_info_truncated`, `device_info_exact_buffer`, `device_info_short_array`, `device_info_oversized_array` |
| FND-20261024121920-3bd2-repository-audit-026 | fixed | descriptor enum counts clamped and length-checked, Sony/Nikon/Canon/Fuji code lists capacity-checked, Canon CustomFuncEx validated before storing, update list and ex-function writes bounded; `descriptor_oversized_enum`, `descriptor_truncated_enum`, `sony_oversized_extension_info`, `canon_oversized_device_info_ex`, `canon_custom_func_ex_bounds` |
| FND-20261024121920-3bd2-repository-audit-027 | fixed | generic (Nikon, Olympus), Sony and Fuji downloads publish the received byte count, short ObjectInfo rejected; `object_size_mismatch_nikon`, `_sony`, `_fuji` |
| FND-20261024121920-3bd2-repository-audit-028 | fixed | `set_string_property()` sends at most 255 bytes plus the terminator; `canon_long_text_property` |
| FND-20261025093224-2d50-fix-ptp-findings-001 | open, new | residual reads without a reply length: Sony GetAllDevicePropData walk (`ptp_sony_decode_property`), `ptp_decode_property_value()` (Fuji/Olympus value polls), Canon AvailListChanged image-format entries; writes there are capacity-checked by this task |

Pre-fix reproduction and retest evidence: [verification.md](verification.md). Driver record: `indigo_drivers/ccd_ptp/REFACTOR.md`, section "Audit findings — PTP reply bounds (2026-10-25)".
