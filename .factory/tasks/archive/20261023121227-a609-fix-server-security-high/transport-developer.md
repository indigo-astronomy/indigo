# Transport security repair handoff

Task: 20261023121227-a609-fix-server-security-high. Assignment: transport developer/test designer. Single writer in `/private/tmp/indigo-security-transport`, detached base `989ec8db122f65ea3813af9db13d510d18bee90e`. No commits, branches, registry edits, driver edits, generator edits, or shared Makefile edits. Strong tier was requested by the coordinator; actual inference settings are unobservable. This is developer verification, not independent review.

## Changed paths

- `indigo_libs/indigo/indigo_bus.h`
- `indigo_libs/indigo_bus.c`
- `indigo_libs/indigo_server_tcp.c`
- `indigo_test/unit/test_transport_cache_security.c` (new)
- `indigo_test/integration/test_transport_http_security.c` (new, opt-in sockets)

Import the three tracked diffs and copy both new tests. The public `indigo_blob_entry` layout is unchanged. New acquire/release/snapshot and upload allocation/publication APIs are additive. Legacy `indigo_find_blob` / `indigo_validate_blob` preserve borrowed semantics and now document that external lifetime serialization is required; all maintained consumers must migrate. Parser owner owns XML migration, coordinator owns Alpaca migration. Those two files were not changed here.

## Behavior and findings

001: HTTP BLOB PUT requires exactly one complete decimal Content-Length, accepts surrounding spaces/tabs, rejects missing, signed, malformed, overflow/over-limit, duplicate/conflicting lengths and any Transfer-Encoding. Complete bounded header lines are required; maximum 64 header lines including the terminating blank line, 1023 bytes per line. The 64 MiB individual limit and 256 MiB aggregate limit include pending upload buffers and published upload buffers, including retired cache entries retained by existing readers. Allocation is ordinary recoverable malloc. Admission or allocation failure returns 503. Invalid framing/over-limit returns 400. Zero-length uploads are valid and publish an empty BLOB.

002/005: Bodies are received into private storage outside all registry/entry/bus locks. EOF/error/timeout preserves prior published content. Successful exact completion swaps the buffer under the entry mutex. Progress deadline defaults to 5 seconds and absolute deadline defaults to 60 seconds from beginning of headers, with each wait capped by both. Failed body progress returns 408; EOF/read error returns 400; retirement before publication returns 404. PUT responses close the connection to avoid unread-body framing ambiguity. Compile-time overrides: INDIGO_BLOB_UPLOAD_LIMIT, INDIGO_BLOB_UPLOAD_BUDGET, INDIGO_BLOB_UPLOAD_PROGRESS_TIMEOUT, INDIGO_BLOB_UPLOAD_TOTAL_TIMEOUT.

004: Registry acquisition retains a private wrapper under blob_mutex. Readers protect cache fields using entry->mutext, or call indigo_snapshot_blob. Release must occur AFTER unlocking entry->mutext. Property clear/release retires every permission class before item storage is freed; retained cache data lives until the final reference. No original property/item pointers are dereferenced by the HTTP consumer. GET copies bytes/metadata privately before network output and fetches an external URL using the copied item. Coordinator-approved refinement: GET snapshots even with indigo_use_blob_buffering=false; that flag continues gating compression. External lazy bytes are fetched privately per request rather than published through a borrowed original item.

Coordinator-authorized coupled 004 repair: BLOB resize holds the registry lock, retires removed items even without allocation, and updates surviving registry item/property pointers transactionally when the property allocation moves. Producer-owned item payloads are not freed by retirement/resize. Existing retained readers use copied metadata/cache data.

GET compression now reserves expansion space and falls back to original bytes if allocation/compression reports no output; the existing separate gzip finding is not claimed closed by this task. The change does not add dependencies or change token policy. The separately tracked RO upload/staging ownership finding 003 remains outside this high-priority assignment; successful RO uploads are deliberately not claimed prevented.

## Executed evidence

All commands below ran in `/private/tmp/indigo-security-transport` on macOS arm64. Baseline library dependencies were built by the coordinator from the unchanged base. Changed bus/server sources were compiled directly for focused checks; unchanged dependency objects are not sanitizer-instrumented. No hardware, device drivers, install/time helpers, external hosts or real data were used.

Baseline build (exit 0):

```sh
cc -std=gnu11 -DINDIGO_MACOS -DINDIGO_TRANSPORT_BASELINE -Iindigo_libs indigo_test/unit/test_transport_cache_security.c -L/Users/polakop/Development/indigo/build/lib -Wl,-rpath,/Users/polakop/Development/indigo/build/lib -lindigo -lpthread -lm -o /private/tmp/transport-baseline
/private/tmp/transport-baseline
```

Baseline run exited 1, deterministic failure: `retired upload is unregistered`, assertion `indigo_validate_blob(address) == NULL`. The baseline does not exercise new symbols or dereference freed memory. No large allocations or server crashes were needed to establish this failure.

Final cache sanitizer build/run (exit 0, 8/8 cases passed):

```sh
cc -std=gnu11 -DINDIGO_MACOS -DINDIGO_BLOB_UPLOAD_LIMIT=4096 -DINDIGO_BLOB_UPLOAD_BUDGET=8192 -Dmalloc=transport_test_malloc -fsanitize=address,undefined -fno-omit-frame-pointer -g -Iindigo_libs -c indigo_libs/indigo_bus.c -o /private/tmp/transport-bus-sanitize.o
cc -std=gnu11 -DINDIGO_MACOS -DINDIGO_BLOB_UPLOAD_LIMIT=4096 -DINDIGO_BLOB_UPLOAD_BUDGET=8192 -DINDIGO_TRANSPORT_ALLOC_FAILURE -fsanitize=address,undefined -fno-omit-frame-pointer -g -Iindigo_libs indigo_test/unit/test_transport_cache_security.c /private/tmp/transport-bus-sanitize.o /Users/polakop/Development/indigo/build/lib/libindigo.a -L/Users/polakop/Development/indigo/build/lib -Wl,-rpath,/Users/polakop/Development/indigo/build/lib -lusb-1.0 -lhidapi -lz -lpthread -lm -framework CoreFoundation -framework IOKit -o /private/tmp/transport-cache-sanitize
/private/tmp/transport-cache-sanitize
```

Cases: unregister released WO item; retained content across release and refused retired publication; retirement after permission change; moved/shrunk property registration; budget pending/published/retired content and reclaim; empty/replacement publication; 50 concurrent lookup/retirement cycles with reader-start synchronization; injected malloc failure and budget restoration. The failure shim is compiled only into the changed bus test object; no production test hook is added. Small limits execute budget exhaustion with at most 8192 bytes. When linked normally with production limits, the normal unit test omits large budget allocations; the small-limit sanitizer target is required to exercise that contract.

Final HTTP sanitizer build/run (exit 0, 7/7 cases passed):

```sh
cc -std=gnu11 -DINDIGO_MACOS -DINDIGO_BLOB_UPLOAD_LIMIT=4096 -DINDIGO_BLOB_UPLOAD_BUDGET=8192 -DINDIGO_BLOB_UPLOAD_PROGRESS_TIMEOUT=0.25 -DINDIGO_BLOB_UPLOAD_TOTAL_TIMEOUT=1.0 -fsanitize=address,undefined -fno-omit-frame-pointer -g -Iindigo_libs indigo_test/integration/test_transport_http_security.c indigo_libs/indigo_bus.c indigo_libs/indigo_server_tcp.c /Users/polakop/Development/indigo/build/lib/libindigo.a -L/Users/polakop/Development/indigo/build/lib -Wl,-rpath,/Users/polakop/Development/indigo/build/lib -lusb-1.0 -lhidapi -lz -lpthread -lm -framework CoreFoundation -framework IOKit -o /private/tmp/transport-http-sanitize
/private/tmp/transport-http-sanitize
```

Cases: malformed/missing/duplicate/conflicting/unsupported/oversized framing with old bytes preserved; short positive EOF and empty EOF; valid lower-case/OWS length and empty body; partial receipt invisible to concurrent GET while unrelated and same-entry bus publication remain responsive; progress timeout; slowly progressing body bounded by absolute deadline; retirement during receive. Socket tests use an ephemeral listener and clients connect only to loopback; the production listener itself binds all IPv4 interfaces. The initial sandbox run exited 1 because bind was denied; scoped automatic approval permitted subsequent bounded synthetic runs. No denied action was bypassed. The final successful HTTP run followed the resize repair and final source changes.

Initial development compile errors (incorrect copy helper name, void compressor return, wrong test initializer arity) were corrected. Initial test harness failures were corrected: a NULL synthetic device violated indigo_update_property's existing precondition, and the response reader waited for EOF beyond the declared zero response body on rejected requests. These are not production regressions or waived gates. Static linking initially lacked CoreFoundation/IOKit and was corrected using the normal macOS dependencies. Final compiler output contains the pre-existing deprecated sprintf warning in indigo_dtoa. No final sanitizer diagnostics occurred.

`git diff --check` exited 0. Test binaries/objects are under `/private/tmp`, not the worktree. No indigo_test/build artifacts were created; there is no generated Makefile.inc in this isolated worktree, so normal Makefile targets remain coordinator verification.

## Final source identities and next assignment

SHA-256 at handoff:

```
6e9dc2fe175afa77d89c10f5620add5667554b8ef2439be68df3e55597f856b6  indigo_libs/indigo/indigo_bus.h
6a8e7b3fef4cea0bb79d1a9c7209d991b1de9f6864986cd87fd876697a61ae3a  indigo_libs/indigo_bus.c
eda58fa6a40f8d43578cbca06a39f95a68bab873fac1345792febe464fac9125  indigo_libs/indigo_server_tcp.c
0e4802e194d6c82a88420cd903fff8a7debae7f96ecce779b3c7cd650dde41b6  indigo_test/unit/test_transport_cache_security.c
609c97e772d268a4f6f4217362c7be71e04ea3d7dedf13b05d9383b207a8bc96  indigo_test/integration/test_transport_http_security.c
```

Coordinator: import these exact files, integrate XML and Alpaca retained consumers, register ordinary cache unit tests and dedicated opt-in transport HTTP/sanitizer targets, update behavior documentation, rebuild combined source-current library, run combined regression and independent quality/security review, then reconcile canonical findings. Linux/Windows execution, ThreadSanitizer, full-library sanitizer instrumentation, hardware and external URL retrieval were not run. No independent approval or finding closure is asserted here.
