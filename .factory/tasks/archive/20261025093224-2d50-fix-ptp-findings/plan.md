# Development plan

Status: final

1. Baseline: fake USB suite 139/139 at `93bb23d76` (scratchpad log, not retained).
2. Shared decoders in `indigo_ptp.c`: terminated strings, length-bounded readers, arrays clamped to the destination, DeviceInfo decoded within the received size, descriptor values checked against the descriptor size, enum counts clamped, `ptp_decode_object_info()`, actual GetObject size in the generic download path.
3. Vendor paths: Sony enum clamp, extension-info caps and download size; Fuji handle count, ObjectInfo and download size, terminated hidden-property append; Olympus ObjectInfo; Nikon vendor code list within size and capacity; Canon GetDeviceInfoEx within size, CustomFuncEx validated before storing, bounded PropValueChanged string, bounded update list, ex-function writes within the request buffer, terminated text property encoding.
4. Fake USB regression cases in `indigo_test/integration/test_ccd_ptp_sdk.c`; pre-fix reproduction against `93bb23d76` in a scratch worktree.
5. ASan/UBSan run, recorded run, REFACTOR.md, MIGRATION_STATUS.md, findings closure, review.
