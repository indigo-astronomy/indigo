# Verification

Verdict: pass

## Scope and environment

Working directory `/private/tmp/indigo-http-acl`, base9557ca85e, product181b4f6df. macOSarm64 runtime; universal x86_64/arm64 compilation only, no Linux/Windows runtime/hardware/externalnetwork claim. Source changedserver/token and tests compiled directly; support archives copied and not fullyinstrumented. Developer executed tests; root separately reviewed the actual4-file source/testdiff and reported PASS before sourcecommit. No later product change.

## Baseline and found defects

FND-20261023112118-8e5e-server-security-audit-007: generic read_line has unlimited interbyte waits after initial G; request/headerdrips can retain admission indefinitely. Baseline actual instrumentedserver, new `partial HTTP` and `progressing HTTP` filters each exit1, retainingworker beyond finiteprogress/totaltestbudget. Client sockets are closed by probe cleanup, no indefinite hung server or externalprobe.
FND-20261023112118-8e5e-server-security-audit-010: newline/space-only ACL input decrementslen past0. Direct baseline token.c/test build under ASan/UBSan exits134 with stack-buffer-underflow/index-1. Three legacy baseline tokencontrols individually pass3/3. All fixturevalues synthetic; no real tokens/configuration used.

## Contracts and repair

Server-local peekpreflight waits for the entire requestline+headerblock before any GET/PUTdispatch. It leaves bytes queued: GETparser and customPUTcallback semantics remain unchanged, including coalescedbinarybody. Request/headerblock capped4096bytes, at most64 nonemptyheaderlines after requestline, linepayload at most1022bytes (trailingCR beforeLF excluded). NUL/missingterminator/oversize rejected beforedispatch. Budget intentionally small relative to ordinary socketreceivebuffers; no claim for custom-reduced socketbuffers or unavailable platforms.

Default progress timeout5seconds and aggregate framingdeadline30seconds, monotonically measured perrequest. Testbuild uses0.2/1seconds. CompleteHTTPheaders cannot be extended indefinitely by dripprogress; unchangedpeekprefix sleeps10ms rather than busyspinning. GenericPUTcallbacks retain their own body lifecycle; existingBLOB header/body5s/60s limits unchanged. HTTPframing deadlines do not apply after XML/JSON/WebSocket protocolswitch, and HTTPkeepalive gets fresh framingbudget perrequest. No publicAPI/handle-layout/I/O-layer change.

ACLtrim now checkslen>=0 before unsignedchar isspace; blank/space/tab/CRLF lines are safelyskipped, ordinarycomments/master/device records retain behavior and malformedrecord still fails. No ACLtransactionality/authpolicy redesign.

## Exact commands and results

- Local ignored Makefile.inc absolute paths remapped, build/lib+include prerequisites copied. No primarybuild artifacts written.
- `make -s -C indigo_test print-variable VAR=SECURITY_CFLAGS`, append `-DINDIGO_HTTP_HEADER_PROGRESS_TIMEOUT=0.2 -DINDIGO_HTTP_HEADER_TOTAL_TIMEOUT=1.0`; exact flags retained security-flags.txt.
- `make -C indigo_test SECURITY_CFLAGS="<security-flags.txt content>" build/security/test_transport_http_security`: exit0, compiler commands in http-final-build.txt. The same target before sourcefix built baseline. Changedserver, common bus/XML/JSON/I/O and test sources instrumented; remaining supportarchive unchanged.
- Baseline `INDIGO_TEST_CASE_FILTER='partial HTTP' ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/security/test_transport_http_security`: exit1 expected. Same with `'progressing HTTP'`: exit1 expected. Retained baseline traces.
- Final `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/security/test_transport_http_security`: exit0,23/23. Tests partialG/GET/PUT, driprequest/dripheaders aggregate deadline, exact4096/overlimit,64/65headers, overlong/NULline, partialpeerclose, genericPUTheaders+coalescedNULbody, HTTPkeepalive, establishedXML/JSON/WS idle beyondtestHTTPtotaldeadline, plus originalBLOBupload/cache/admission/file/resource-retirement controls. Deliberate injected worker-spawnfailure diagnostics are expected controls, not unexpected errors.
- `make -C indigo_test build/integration/test_server_websocket_sanitize`: exit0; `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 indigo_test/build/integration/test_server_websocket_sanitize`: exit0,5/5 actualnewservercode. RFC6455accept, maximumfittingkey, overlongkeys and subsequenthealthyupgrade preserved. This target uses productionHTTPtimeoutdefaults; the HTTPsuite separately tests idleprotocolswitch with shorteneddeadline.
- For normalHTTP compilation, use sameSECURITY_CFLAGS with sanitizer/framepointer flags removed and O1→O2, `SECURITY_BUILD=build/http-normal`, and SERVER_WEBSOCKET_SANITIZE_LDFLAGS with fsanitize removed; `make -C indigo_test <these overrides> build/http-normal/test_transport_http_security`: exit0. Exact compiler/link commands in http-normal-build.txt. `indigo_test/build/http-normal/test_transport_http_security`: exit0,23/23, loopbackcontext.
- `clang -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -Iindigo_libs -Ibuild/include indigo_test/unit/test_token.c indigo_libs/indigo_token.c -Lbuild/lib -Wl,-rpath,/private/tmp/indigo-http-acl/build/lib -lindigo -lpthread -lm -o /tmp/token-fixed`: exit0; `/tmp/token-fixed`: exit0,4/4. Actualtoken source/test instrumented; supporting library uninstrumented. Same compilation against originaltoken source produced retained baselineunderflow. Same command withoutsanitizers and O2, output/tmp/token-fixed-normal: exit0,4/4.
- For each of indigo_libs/indigo_server_tcp.c and indigo_libs/indigo_token.c: `clang -g -arch x86_64 -arch arm64 -mmacosx-version-min=10.10 -fPIC -std=gnu11 -DINDIGO_MACOS -Duint=unsigned -Iindigo_libs -Ibuild/include -Werror=return-type -Werror=implicit-function-declaration -c <source> -o /tmp/http-acl-<server-or-token>-universal.o`: exit0, no diagnostics.
- `./.factory/scripts/check` and `./.factory/scripts/security`: exit0; scoped Factory structural/security boundaries, not productcertification. Root owns combined197-case Factoryscope, not duplicated here.

Loopbacktests require outside-sandbox bindingpermission and run only isolated ephemeralports/privateHOME/syntheticBLOBs. No externalservice/hardware/image/index activity. Compiler stderr in retained builds includes inherited deprecatedsprintf warnings from unchanged helpercode, no new diagnostic in productionuniversalcompile. Leaksanitizer is not enabled on this macOS host. Transcripts normalize trailingwhitespace only.

## Final test summary

Baseline newHTTP reproductions0/2 expected, oldtoken3/3 controls, ACLASan expectedabort. FixedHTTP23/23 normal and23/23 ASan/UBSan; actualserverWebSocket5/5 sanitizer controls; token4/4 normal and4/4 sanitizer. Hardware0run/0passed. All ACs pass within statedscope. Root owns sharedMakefile additions for2 shortHTTPtestdefines, canonicalclosure and integration/publication; no newpersistent testfiles or Xcode references.

`make -C indigo_test test-clean`: exit0; localtestbuildartifacts removed after retaining reports. Root final artifactreview PASS of source181b/23normal+23sanitizedHTTP/5sanitizedWS/4bothToken/universalcompile and negativebaselines; runtimechecks remain developer-owned. No additionalproductchanges or tests required. Final immutableprocesscheck follows separateFactorycommit and is recorded externally.
