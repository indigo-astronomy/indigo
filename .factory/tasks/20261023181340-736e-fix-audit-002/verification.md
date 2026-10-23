# Verification

Verdict: pass

Reviewer: implementing Codex agent, separate verification pass; no independent review claimed.
Date: 2026-10-23. Reviewed state: task working tree based on c5a8b69ab55098f93e723f3d6ffc2a6cf5cae441. This is preliminary acceptance verification, not the post-005 integration candidate; rerun after final synchronization.
Working directory for all commands: /private/tmp/indigo-20261023181340-736e-fix-audit-002. macOS arm64, Apple clang; normal build also compiles x86_64 slice, which was not executed. Python 3 system runtime. Existing primary build/lib supplies unchanged support symbols. Generated Makefile.inc copied locally, with INDIGO_ROOT changed to the isolated worktree; no generated configuration will be committed. No writes to the primary checkout.

## Baseline

`make -C indigo_test build/unit/test_network_url BUILD_LIB=/Users/polakovic/Development/indigo/build/lib`: initial attempt could not run because isolated checkout lacked generated Makefile.inc. After copying existing local configuration, build passed.
`indigo_test/build/unit/test_network_url`: pre-fix exit 1, oversized hostname reached resolver despite expected rejection (assertion expected zero resolutions, got one).
ASan/UBSan build below against pre-fix sources: runtime error index 128 out of bounds in indigo_io.c:368, stack-buffer-overflow. To isolate the second API, `git show HEAD:indigo_libs/indigo_uni_io.c > /tmp/002-uni-baseline.c`, then the same sanitizer build substituting that baseline unified source and retaining fixed legacy source: index 128 and stack-buffer-overflow at unified source line 1586. Both baseline binaries exited nonzero. Sanitizer symbolization emitted host warnings, but fault type/source index were explicit.

## Acceptance and regressions

- AC-1 passed: 127-byte host forwarded intact; 0/128/129/2048-byte hosts rejected, both APIs, explicit and default ports. Resolver stub verifies zero resolution attempts for invalid input.
- AC-2 passed: scheme syntax, empty/zero/oversized/signed/whitespace/trailing-junk/extra-colon ports and malformed authorities rejected; decimal bounds, leading zeroes and overflow covered.
- AC-3 passed: tcp/udp override opposite hints; custom scheme preserves caller hint; default ports forwarded; explicit port overrides invalid default. Established failure values and null inputs preserved.

`make -C indigo_test build/unit/test_network_url` and `indigo_test/build/unit/test_network_url`: build passed; 3/3 named suites passed, exercising the table of cases through both production implementations. Only getaddrinfo is replaced with a deterministic failing resolver; no DNS lookup or socket attempt occurs. Unified helper logs its expected connection failure on resolver rejection.

Sanitizer command (exit 0):
```
/usr/bin/clang -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -DINDIGO_MACOS -DINDIGO_CLIENT -Dgetaddrinfo=indigo_test_getaddrinfo -Iindigo_libs -o /tmp/002-fixed-asan indigo_test/unit/test_network_url.c indigo_libs/indigo_io.c indigo_libs/indigo_uni_io.c -L/Users/polakovic/Development/indigo/build/lib -Wl,-rpath,/Users/polakovic/Development/indigo/build/lib -lindigo -lm -lpthread -lz
/tmp/002-fixed-asan
```
3/3 passed; no ASan/UBSan finding. Both changed production translation units are compiled directly; INDIGO_CLIENT excludes unrelated HID/server functionality, not the URL paths.

`./.factory/scripts/check`: passed (diff whitespace, findings 104 active/43 archived, decision index, project boundaries).
`./.factory/scripts/test`: passed, 175 tests, 112.024 seconds.
`./.factory/scripts/security`: passed, local Factory security boundaries.
`plutil -lint indigo.xcodeproj/project.pbxproj`: passed.

## Wider-suite limitation

`make -C indigo_test test-unit`: exit 2 during link of unchanged test_transport_cache_security. Existing build/lib/libindigo.dylib lacks indigo_snapshot_blob and related retained-BLOB support symbols. `nm -g /Users/polakovic/Development/indigo/build/lib/libindigo.dylib` confirms missing exports. No diff in that test or indigo_bus.c. This is an existing local binary/source mismatch, not an accepted passing full suite. Targeted URL checks compile the changed source directly and pass independently of the missing unrelated exports. Full framework rebuild and full suite are outside the bounded regression requirement; no hardware, Linux or Windows execution claimed.

## Finding and final delivery gate

FND-20261023101753-bc8d-complete-repository-audit-002 reproduced and repaired on this preliminary candidate. Keep active until final synchronized verification. Post-005 target, final content review, archival, committed process check and coordinator integration remain pending.
