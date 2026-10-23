# Parser repair developer handoff

Task: `20261023121227-a609-fix-server-security-high`. Isolated working directory: `/private/tmp/indigo-security-parsers`. Base: `989ec8db122f65ea3813af9db13d510d18bee90e`. Uncommitted patch: `/private/tmp/indigo-parsers.patch`. No task/registry/shared build configuration, generator, vendor or prohibited driver files changed. Assigned developer and test-designer roles loaded; this is developer verification, not independent security approval. Requested model settings were strong gpt-6-astra/high; inference metadata/cost not independently observable.

## Changed paths and contracts

- `indigo_libs/indigo_xml.c`: checked attribute capacity (524288 content bytes plus terminator), names (127 content bytes), named entities (reserved terminator and rejection of unknown/malformed entities); direction guard for request-only versus response paths; strict decimal BLOB sizes, validated streaming base64 quartets, exact decoded-length consistency, recoverable per-item allocation and 256 MiB inline limit. Both legacy and modern inline bodies accept whitespace. Each temporary set-BLOB property owns distinct item buffers via the existing WO cleanup contract; dispatch/error/EOF cleanup releases them. URL metadata is not subject to the inline allocation cap. Cached update vector types must match before accessing item unions. Empty inline updates clear cached content without reallocating to size zero.
- XML cache consumer uses the transport sibling's `indigo_acquire_blob_by_name` / `indigo_release_blob`; it copies size/content under the entry mutex, releases the retained entry after unlocking, and does not dereference borrowed property/item pointers.
- Five upstream XML message forwards now use literal `%s` with NULL format preserved for absent messages. This directly coupled format issue was explicitly added by coordinator authorization; regression observes definition, update, explicit message and deletion notifications containing `%s %n %%` literally.
- `indigo_libs/indigo_json.c`: input and token allocations reserve terminators, field-name bound is 127, and oversized number/logical tokens reject before dispatch. WebSocket header/extension/mask/payload reads require exact counts; all length comparisons happen in uint64 before narrowing. Masked text and continuation messages up to 262144 assembled bytes are accepted. RSV, binary/unsupported opcodes, unmasked client frames, bad continuation/control framing and noncanonical length encodings reject. Ping emits pong and close emits close; complete control writes hold the existing JSON serializer mutex and use the adapter output handle. Fragmented text permits interleaved controls.
- `indigo_test/unit/test_protocol_security.c`: 13 public-entrypoint cases using isolated temporary files, no sockets or external services. Tests link a built archive and never include implementation `.c`. Eligible for ordinary UNIT_TESTS registration if desired; coordinator owns Makefile/Xcode registration. Fixture content is synthetic.

## Retained evidence reused

Read specifically assigned archived `parsers-security.md` and active canonical findings 011–017 from `20261023112118-8e5e-server-security-audit`. No task/findings archive bulk discovery or rg occurred. Read own root/source/test instructions and task brief/specification/plan/test-plan/handoff. Existing tests and protocol entrypoints were inspected before changes.

## Baseline reproduction

Generated baseline XML/JSON sources with `git show HEAD:indigo_libs/indigo_xml.c` and corresponding JSON path into `/private/tmp/parser-baseline/`. Built those fresh with `clang -g -O1 -Wno-deprecated-declarations -DNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer -DINDIGO_MACOS -Iindigo_libs -c SOURCE -o OBJECT`. The unchanged source-current `indigo_base64.c` was built with identical instrumentation for the decoded-size reproduction. Thinned the coordinator's baseline-built archive with `lipo /Users/polakop/Development/indigo/build/lib/libindigo.a -thin arm64 -output /private/tmp/parser-baseline/libindigo.a`, replaced parser/base64 object members using `ar r`, then linked the new regression suite with the same command shown below, using baseline archive/output paths and `-DNDEBUG`.

Ran each baseline case separately via `INDIGO_TEST_CASE_FILTER=CASE /private/tmp/parser-baseline/test_protocol_security` using Python subprocess, preserving each combined stdout/stderr in `/private/tmp/parser-baseline/CASE.log`:

| Case | Exit | Observed old-source failure |
| --- | --- | --- |
| xml_attribute_boundaries | SIGABRT (-6) | ASan heap-buffer-overflow |
| xml_name_boundaries | SIGABRT (-6) | ASan heap-buffer-overflow |
| xml_entity_boundaries | SIGABRT (-6) | ASan stack-buffer-overflow |
| xml_wrong_direction | SIGABRT (-6) | UBSan NULL device member access then ASan SEGV |
| xml_blob_owned_items_and_padding | SIGABRT (-6) | ASan heap-use-after-free in set_property |
| xml_blob_rejects_mismatch_and_malformed | SIGABRT (-6) | ASan heap-buffer-overflow in base64_decode_fast |
| json_token_boundaries | SIGABRT (-6) | ASan heap-buffer-overflow |
| websocket_frames | 1 | Positive short frame incorrectly dispatched a change |
| xml_messages_are_literal | 1 | Expected literal `%s %n %%`, observed `(null)  %` |

The initial archive replacement attempt targeted a fat archive and did not replace baseline members; its assertion failure was discarded as repair evidence. All reported final builds first thin the archive and replace the checked objects, as shown below.

## Final developer verification

Host: macOS arm64, Xcode clang. Ran the exact script below from the isolated working directory via `sh /private/tmp/run-parser-check.sh`. It compiles changed XML/JSON and sibling bus source afresh, replaces those archive members, and links public tests against that built archive. Unchanged core/dependency archive members are the coordinator's source-current baseline build. Tests are instrumented plus the changed XML/JSON/bus sources; this is not an all-library sanitizer build. Coordinator must verify the integrated final source candidate separately.

Results for **both assertions enabled and -DNDEBUG**: protocol security **13/13**, existing XML protocol **8/8**, existing JSON protocol **6/6**, all exit 0, no ASan/UBSan findings. Logs: `/private/tmp/parser-check-debug/test_protocol_{security,xml,json}.log` and `/private/tmp/parser-check-release/test_protocol_{security,xml,json}.log`.

Normal non-sanitized build/run used `sed -e 's/for configuration in debug release/for configuration in normal/' -e 's/-fsanitize=address,undefined //g' /private/tmp/run-parser-check.sh > /private/tmp/run-parser-check-normal.sh`, then `sh /private/tmp/run-parser-check-normal.sh`: the same **13/13 + 8/8 + 6/6** passed. Logs under `/private/tmp/parser-check-normal/`.

`make -C indigo_test test-clean` completed exit 0 after final execution; generated test outputs remain only in isolated /private/tmp evidence directories. `git diff --check` passed. No source commit, integration or publication performed.

```sh
#!/bin/sh
set -eu
cd /private/tmp/indigo-security-parsers
for configuration in debug release; do
 output=/private/tmp/parser-check-$configuration
 mkdir -p "$output"
 extra=
 if [ "$configuration" = release ]; then extra=-DNDEBUG; fi
 for source in indigo_xml indigo_json; do
  clang -g -O1 -Wno-deprecated-declarations $extra -fsanitize=address,undefined -fno-omit-frame-pointer -DINDIGO_MACOS -I/private/tmp/indigo-security-transport/indigo_libs -Iindigo_libs -c indigo_libs/$source.c -o "$output/$source.o"
 done
 clang -g -O1 -Wno-deprecated-declarations $extra -fsanitize=address,undefined -fno-omit-frame-pointer -DINDIGO_MACOS -I/private/tmp/indigo-security-transport/indigo_libs -I/Users/polakop/Development/indigo/build/include -c /private/tmp/indigo-security-transport/indigo_libs/indigo_bus.c -o "$output/indigo_bus.o"
 lipo /Users/polakop/Development/indigo/build/lib/libindigo.a -thin arm64 -output "$output/libindigo.a"
 ar r "$output/libindigo.a" "$output/indigo_xml.o" "$output/indigo_json.o" "$output/indigo_bus.o"
 for test in test_protocol_security test_protocol_xml test_protocol_json; do
  clang -g -O1 -Wno-deprecated-declarations $extra -fsanitize=address,undefined -DINDIGO_MACOS -Iindigo_libs indigo_test/unit/$test.c "$output/libindigo.a" -L/Users/polakop/Development/indigo/build/lib -lusb-1.0 -lhidapi -lz -framework IOKit -framework CoreFoundation -lm -lpthread -o "$output/$test"
  mkdir -p indigo_test/build/unit
  (cd indigo_test && "$output/$test") > "$output/$test.log" 2>&1
  tail -1 "$output/$test.log"
 done
 done
```

## Coverage and residuals

Boundary inputs span multiple library reads; base64 regression spans a 600000-byte encoded payload and verifies decoded bytes. Ownership tests cover larger/smaller and smaller/larger items, both protocol versions, malformed and truncated second items, strict padding, empty content, invalid declared lengths and oversized declared size without large allocation. WebSocket cases cover all positive truncated payload/header/mask lengths for a small frame, truncated extended lengths, 64-bit lengths with small low bits/high bit, exact full capacity, continuation with interleaved pong, unmasked/unsupported frames and exact ping/close responses. Access tokens and ordinary valid XML/JSON examples remain covered by existing suites.

Windows/32-bit runtime and Linux runtime are not available in this assignment. UINT64-before-LONG_MAX/capacity checks are source-reviewed but not a Windows execution claim. Tests do not exhaustively certify RFC6455 UTF-8 grammar, performance, arbitrary stream syntax, cache concurrency or OOM behavior. Broader aggregate property-item/request budgets remain the previously scoped non-goal finding 018.

Pre-existing follow-up: XML set_property allocates cached RO BLOB `property_item->blob.value`; parser deletion/EOF calls indigo_release_property, but release_dependencies frees BLOB item values only for WO properties (RO caches hold a distinct snapshot). Thus cached parser-owned RO item storage can leak on deletion/disconnect, independently of newly owned temporary inline storage. Coordinator explicitly kept this outside the current repair unless introduced/amplified. Do not change generic RO freeing: ordinary drivers own their RO buffers. A future fix should be restricted to the XML parser-owned cached property lifecycle. No risk acceptance or closure of this follow-up is claimed.

## Next assignment

Coordinator imports the three-file patch, wires the test target/Xcode source entry while preserving unrelated existing project changes, retains this evidence, runs integrated source-current tests/process gates, and obtains final independent quality/security review. Reviewers already identified and rechecked the control-frame serialization and empty-BLOB fixes; final integrated verdict remains coordinator-owned.
