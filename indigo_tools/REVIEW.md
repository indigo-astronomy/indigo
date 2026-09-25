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
| TOOLS-011 | Medium | `Makefile.drv:111-113` (repository root) | Generated driver sources are regenerated during the build by a rule with a list of targets and a single recipe:
`$(SOURCES): $(DEFINITION) ../../indigo_tools/indigo_generator.c` with `$(BUILD_BIN)/indigo_generator $(DEFINITION)`, where `SOURCES = $(wildcard *.c) ...` and `DEFINITION = $(wildcard *.driver)`. A rule with several targets and an ungrouped recipe runs that recipe once per out-of-date target, so under `make -j` two generator invocations for the same driver can run at once, and a compile can read one of the `.c` files while a generator run is rewriting it. Observed three times during this session as transient parse errors in files that are clean in git and compile fine on a rerun: `indigo_ccd_uvc.c:22: unterminated conditional directive` and `indigo_ccd_dsi.c:532: use of undeclared identifier 'i'` (a truncated `i` from a half-written file). It also sweeps in hand-written `.c` files that sit in the same directory, since `SOURCES` is a wildcard. Use a grouped target (`&:`), a stamp file, or a pattern rule so one generator run satisfies all of its outputs. | Closed (fixed 2026-09-19). Only the driver source carries the recipe now; the header and the main stub are declared to depend on it, so make orders them behind a single generator run. Grouped targets (`&:`) would say this directly but need GNU make 4.3 and this tree builds with 3.81. `GENERATED_SOURCE` also picks `.cpp` where the generator emits C++ (`ccd_qhy`, `ccd_qhy2`), and hand-written sources sharing the directory are no longer pulled into the rule. Measured by touching all 99 `.driver` files and running `make -j8 all`: the old rule invoked the generator 207 times for 99 drivers, most of them twice; the new rule invokes it 99 times, once each, with no duplicates and a clean build.) |
| TOOLS-009 | High | `indigo_generator.c:2363` | SDK and HID SHUTDOWN templates detached devices before queued hot-plug work finished; the first drain fix covered only direct libusb. | Closed (fixed) |
| TOOLS-010 | High | `indigo_generator.c` HID INIT/arrival emission | HID templates ignored hot-plug registration and attach failures and retained INIT after queue creation failed. | Closed (fixed) |
| TOOLS-012 | Medium | `indigo_generator.c` (`write_c_connection_change_handler()`) | The generated connection handler queued the `on_timer` one-shot call inside the connect branch, before it assigned `CONNECTION_PROPERTY->state = INDIGO_OK_STATE` and before the closing `indigo_<class>_change_property(device, NULL, CONNECTION_PROPERTY)`. The generated `<device>_timer_callback()` prologue is `if (!IS_CONNECTED) return;`, which requires that OK state. In a generated hot-plug driver the connection handler runs on the per-driver queue while the callback runs on the device queue, so the callback frequently observed the still-BUSY connection and returned; because nothing else queues it, a self-rescheduling poll was then lost for the whole session. Found by inspection, not reproduced: an earlier claim of a reproduction on a physical ASI294MC Pro is withdrawn, because those failures were the vendor SDK returning a zero temperature for the first ~400 ms after init plus a test asserting on a legitimately suppressed property update. The old placement also queued the callback before the closing `indigo_<class>_change_property()` call that defines the class properties, so it could publish to a property the client did not have yet, which needs no thread race. The call is now emitted once at the end of the handler, after the connection is published and the class properties are defined, guarded by `if (IS_CONNECTED)`. Full impact, the 14 affected drivers and their revalidation are recorded as `DRV-211` in `indigo_drivers/REVIEW.md`. | Closed (fixed 2026-09-21 with the user's explicit approval; all 115 generator inputs regenerated, all portable and macOS drivers rebuilt, 397 narrow fake-SDK cases across the 12 affected drivers that have a suite pass) |
| TOOLS-013 | Low | `indigo_generator.c` (`write_c_change_property()`, `write_c_connection_change_handler()`) | The generator wrote the refusal block and the CONNECTION admission block out statement by statement, eight and five lines respectively, at every site: 150 refusals across 49 drivers and 164 connection blocks. It now emits `INDIGO_REJECT_CHANGE_IF()` and `INDIGO_PROCESS_CONNECT()` / `INDIGO_PROCESS_QUEUED_CONNECT()`, the macros added in `LIB-016`. The third connection shape, which picks the driver queue or the device queue per request, keeps its explicit form because the choice sits inside the admission guard. Changed with the user's explicit approval. All 115 inputs regenerated: 134 files, -2428/+586 lines, no behaviour change — for four sampled drivers the preprocessed translation unit is byte-identical apart from the new `indigo_reject_change()` declaration and shifted `assert()` line numbers, so no driver version was bumped for this. `make all` is clean and twelve complete driver suites pass. | Closed (fixed 2026-09-21) |
| TOOLS-014 | Medium | `indigo_generator.c` (`write_c_connection_change_handler()`, new `write_c_disconnect_state_reset()`) | The generated disconnect branch calls `indigo_cancel_pending_handlers(device)`, which also drops change handlers that were queued but had not run yet. Their properties stayed BUSY, and the BUSY guard of `INDIGO_COPY_*_PROCESS_CHANGE` then silently refused every later change to them: after reconnect for connected-only properties, and at once for `always_defined` ones. Reproduced with the `ccd_asi` fake SDK. | Closed (fixed 2026-09-25). After `on_disconnect`, the generated handler returns every declared property still BUSY to OK, except CONNECTION and CONFIG, whose state the framework owns. `always_defined` properties are republished at once. All 114 generated drivers with declared properties were regenerated and their `version` bumped by one; `gps_simulator` and `gps_gpsd` declare no properties, so their output is unchanged. Regression test `Cancelled change does not survive disconnect` in `indigo_test/integration/test_ccd_asi_sdk.c` fails against the previous output (`CCD_GAIN` BUSY after reconnect) and passes now. Tracked for the drivers as DRV-217. |

## Finding Summaries

### TOOLS-009 (Closed — fixed)

SDK and HID SHUTDOWN templates detached devices before queued hot-plug work finished; the first drain fix covered only direct libusb. All three transports now share serialized disconnected-device verification and deregister/drain/detach/delete emission; SDK retries stop and cancel before drain.

### TOOLS-010 (Closed — fixed)

HID templates ignored hot-plug registration and attach failures and retained INIT after queue creation failed. Fake USB tests for SX/Atik reproduced the failures. Added retryable queue/registration rollback and failed-attach allocation cleanup; generated outputs rebuilt.

Validation: all four scenario groups pass for both SX and Atik.

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
| HEAD | working tree | 2026-09-08 | Focused review of the approved generator delta: INDIGO free helpers, direct-libusb duplicate arrival rejection, master attach failure cleanup, retryable INIT rollback, and SHUTDOWN queue drain through the shared timer API. Full root `make all` passed for macOS x86_64/arm64. No additional regression found in this delta; existing unrelated findings and folder baseline remain unchanged. |
| HEAD | working tree | 2026-09-08 | Follow-up across every generated hot-plug transport after identifying the omitted SDK/HID branches (TOOLS-009). Unified SHUTDOWN emission; generator regression covers libusb, SDK, SDK retries and HID. Fake SDK checks drain-before-detach, discovery retry lifetime and rejected shutdown. Folder baseline unchanged. |
| `84298256404b3aee1028d29ce152233ffd8afe2e` | working tree | 2026-09-09 | Focused HID generator INIT/arrival rollback review exposed by the SX/Atik fake USB tests; recorded and closed `TOOLS-010`. Queue/registration retry and failed-attach cleanup were verified by all four scenario groups for each wheel, with regenerated output and production build checks. Folder baseline unchanged. |
