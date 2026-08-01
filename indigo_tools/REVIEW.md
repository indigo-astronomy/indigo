# indigo_tools Review

## Status

| Field | Value |
| --- | --- |
| Last reviewed commit | `017ba602857378e4aed489c065c76eacae15924c` |
| Review state | Baseline review complete; open findings recorded. |

## Scope

Command-line tools under `indigo_tools/`.

Reviewed source files:

- `fits_to_raw.c`
- `fix_fits.c`
- `fix_xisf.c`
- `indigo_driver_metadata.c`
- `indigo_drivers.c`
- `indigo_generator.c`
- `indigo_list_usbserial.c`
- `indigo_metadata_extractor.c`
- `indigo_prop_tool.c`
- `indigo_raw_crop.c`
- `indigo_raw_to_fits.c`
- `indigo_scan_drivers.c`
- `Makefile`

Generated build output under `indigo_tools/Debug*/` was excluded.

## Current Findings

| ID | Severity | File | Summary | Status |
| --- | --- | --- | --- | --- |
| TOOLS-001 | High | `indigo_prop_tool.c:181`, `indigo_prop_tool.c:202`, `indigo_prop_tool.c:214`, `indigo_prop_tool.c:224`, `indigo_prop_tool.c:251`, `indigo_prop_tool.c:262`, `indigo_prop_tool.c:937` | The property parsers build `sscanf()` formats with field widths equal to destination buffer sizes, which still leaves room for a terminating NUL beyond the array. The remote-server parser also uses `%[^:]:%s` with no widths for `hostname[255]` and `port_str[100]`. Exact-size or long command-line arguments can overflow stack/global buffers before validation. Use bounded widths of `sizeof(buffer) - 1`, check `item_count < MAX_ITEMS` before appending parsed items, and reject overlong fields. | Open |
| TOOLS-002 | High | `indigo_raw_crop.c:41`, `indigo_raw_crop.c:44`, `indigo_raw_crop.c:45`, `indigo_raw_crop.c:68`, `indigo_raw_crop.c:76` | `indigo_raw_crop` trusts the RAW header and ROI dimensions. It does not validate the header read, RAW signature, positive crop width/height, multiplication overflow, allocation success, or payload read size. A truncated or crafted RAW file, or a crop like `0,0,-1,10`, can drive undersized allocation and out-of-bounds reads/writes. Validate the header and type, compute byte counts with overflow-checked `size_t`, require `width > 0 && height > 0`, and fail if reads or allocations are incomplete. | Open |
| TOOLS-003 | Medium | `fits_to_raw.c:49`, `fits_to_raw.c:65`, `fits_to_raw.c:69`, `fits_to_raw.c:70`, `fits_to_raw.c:77`, `fits_to_raw.c:80` | `fits_to_raw` does not check CFITSIO status after reading image parameters or pixels, accepts any non-8-bit FITS image as 16-bit output, and allocates image buffers from unchecked axis multiplication. Bad or unsupported FITS inputs can produce corrupt RAW output or memory faults. Check `status` after every FITS call, explicitly accept only supported `bitpix` values, validate `naxis/naxes`, and verify allocation/write results. | Open |
| TOOLS-004 | Medium | `indigo_raw_to_fits.c:46`, `indigo_raw_to_fits.c:64`, `indigo_raw_to_fits.c:68`, `indigo_raw_to_fits.c:70`, `indigo_raw_to_fits.c:72`, `indigo_raw_to_fits.c:160`, `indigo_raw_to_fits.c:161`, `indigo_raw_to_fits.c:165` | The RAW-to-FITS utility has fragile file handling. `write()` is treated as a one-shot full write, input allocation/reallocation is unchecked, `fseek()`/`ftell()` and `fread()` failures are ignored, and each glob pattern is evaluated repeatedly in separate `glob()` calls before `globfree()`. This can mis-handle partial I/O, leave uninitialized data in conversions, or leak glob state. Use checked `size_t` file sizes, loop until all output bytes are written, call `glob()` once per pattern, and check all I/O results. | Open |
| TOOLS-005 | Medium | `indigo_driver_metadata.c:15`, `indigo_driver_metadata.c:19`, `indigo_metadata_extractor.c:83`, `indigo_metadata_extractor.c:84`, `indigo_metadata_extractor.c:85` | Metadata tools copy paths into fixed buffers or pass paths through a shell command unsafely. `indigo_driver_metadata` uses `strcpy()` from arbitrary CLI arguments into `name[128]`; `indigo_metadata_extractor` builds `clang -E %s` for `popen()` without quoting and then dereferences `pipe` without checking for failure. Long paths can overflow, and specially named driver paths can change shell behavior. Use `snprintf()`/length checks and fork/exec-style invocation or strict shell quoting plus `popen()` failure checks. | Open |
| TOOLS-006 | Medium | `indigo_generator.c:123`, `indigo_generator.c:1035`, `indigo_generator.c:2152`, `indigo_generator.c:2162`, `indigo_generator.c:2724`, `indigo_generator.c:2737`, `indigo_generator.c:2741`, `indigo_generator.c:2745`, `indigo_generator.c:2751`, `indigo_generator.c:2765`, `indigo_generator.c:2766` | The generator assumes allocations, reallocations, file redirects, and path copies always succeed. `allocate()` immediately `memset()`s a possibly NULL allocation, `realloc()` results overwrite the only pointer, `freopen()` results are ignored, and `strcpy(source_file, definition_file)` can overflow `PATH_MAX`. Failures can crash the generator, read from the wrong stream, or emit partial files. Add checked allocation helpers, temporary pointers for `realloc()`, bounded path copies, and explicit `freopen()` error handling before generating output. | Open |
| TOOLS-007 | Medium | `fix_fits.c:58`, `fix_fits.c:63`, `fix_fits.c:74`, `fix_xisf.c:67`, `fix_xisf.c:75`, `fix_xisf.c:90`, `fix_xisf.c:97` | The repair tools read whole files and then inspect or rewrite fixed offsets without first proving the file is large enough, and they do not check write results. A small input can make `fix_fits` read past the loaded buffer at offset 5760, while `fix_xisf` can rewrite from `data + 2880` even when the file is shorter. Validate minimum sizes before fixed-offset access and check every `fwrite()`/`fputc()` result before reporting success. | Open |
| TOOLS-008 | Low | `indigo_tools/Makefile:33`, `indigo_tools/Makefile:35`, `indigo_tools/Makefile:42`, `indigo_tools/Makefile:48` | The tools build target produces `indigo_drivers`, `indigo_driver_metadata`, and `indigo_scan_drivers`, but `install`, `uninstall`, and `clean` omit some of those binaries. This can leave stale build artifacts and makes packaging behavior differ from `all`. Decide which helper tools are intentionally private; then either exclude them from `all` or handle them consistently in install/uninstall/clean rules. | Open |

## Review Focus

- Argument parsing, exit codes, and user-facing error messages.
- File, network, and process resource cleanup.
- Use of public INDIGO APIs instead of duplicated protocol logic.
- Portability of command-line behavior.
- Tests or documented manual validation for changed behavior.

## Reviewed Ranges

| From | To | Date | Notes |
| --- | --- | --- | --- |
| Repository start | `017ba602857378e4aed489c065c76eacae15924c` | 2026-08-01 | Baseline review of all checked-in `indigo_tools` sources, excluding generated build output. |
