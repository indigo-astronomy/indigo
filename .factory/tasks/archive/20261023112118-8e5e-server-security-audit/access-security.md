# Access, configuration, and browser security review

Reviewed revision: `cb3c392e9f8833ddb6d29744a34cfae618b04fc8`, task branch `task/20261023112118-8e5e-server-security-audit`, 2026-10-23. Reviewer: delegated security-reviewer, independently reviewing existing product sources. Write scope: this report and `access-coverage.json`. Coordinator owns canonical findings. Verdict: **changes required**. No implementation change, fix, retest or owner risk acceptance is claimed.

## Contracts and boundaries

Read root AGENTS.md, indigo_libs/AGENTS.md, assigned role, project/state, active brief/specification/plan/handoff, active finding summaries, server guide and the complete access-control guide. `docs/security.md` is absent; applicable product contracts come from maintained documentation. Configured Factory security checks validate workflow metadata and are run by the coordinator; they are not application vulnerability tests.

`indigo_docs/INDIGO_DEVICE_ACCESS_CONTROL_AND_LOCKING.md:12` explicitly says tokens/locking are not security features and all clients can monitor values. Lines 21, 27 and 43 require a nonzero master token for protection/locking. Public default control (:24), readable properties and absence of transport confidentiality are not reported as authorization defects. **Reject the earlier zero-master bypass proposal**: allowing tokenless changes with a zero master token is documented behavior. No unapplied proposal is treated as current code or verified remediation.

Reviewed boundaries: client request to bus dispatch, device/master token matching, ACL-file parsing/startup, management properties to OS helper commands, driver loading, and server metadata to maintained browser components. Device control does not imply permission to reinterpret arbitrary data as shell commands or variadic format strings.

## Findings for canonical registration

### A-01 — High, security: INSTALL item names reach a shell without validation or quoting

Location: `indigo_server/indigo_server.c:1414-1419`, command execution at `:860-868`.

INSTALL copies matching values into the canonical property, then iterates the original incoming `property->items` and interpolates its first selected item's **name** into `s_rpi_ctrl.sh --install-version %s`. `execute_command()` formats that string and calls `popen()`. JSON copies client-provided names without restricting shell syntax (`indigo_libs/indigo_json.c:265-278`). Bus dispatch checks device tokens, not a version allowlist (`indigo_libs/indigo_bus.c:835-856`); `indigo_property_match()` checks type/device/property name only (:1824-1829). `indigo_property_copy_values()` copies known names to the destination but never filters the incoming request (:1924-1980), so unknown names still reach the shell.

Prerequisites: `RPI_MANAGEMENT` build, runtime `-f` / `--enable-rpi-management` with helper found (:1770-1782), version discovery creating INSTALL (:925-974), and a client permitted to change Server properties (public by default, otherwise satisfying its token). Ordinary non-RPI builds are not asserted to expose this path.

Impact: caller-controlled shell syntax executes with server-process privileges, outside the intended choice of an available package version. Root privileges depend on deployment and were not verified. No shell payload or installation was executed; this is a complete source-to-sink finding.

Remediation: reject names absent from canonical INSTALL items and invoke the helper through an argument-vector API, or consistently quote the validated selected version when retaining the shell API. Use canonical validated state, not raw incoming names. Regression checks should intercept invocation and show unknown names rejected and allowed versions passed as one argument, without installing packages.

### A-02 — High, security: management error text is interpreted as a printf format

Location: `indigo_server/indigo_server.c:877-881`; formatting sink `indigo_libs/indigo_bus.c:1078-1082`.

The helper's first `ALERT:` line is advanced past its colon and supplied directly as `indigo_update_property(device, property, message)` with no variadic arguments. That API calls `vsnprintf()` using the supplied format. Percent directives in the diagnostic therefore consume nonexistent arguments instead of remaining literal data.

Concrete current-source path: HOST_TIME copies client text and shell-quotes it into `--set-date` (`indigo_server/indigo_server.c:1541-1545`). The maintained helper stores the argument (`tools/rpi_ctrl.sh:633-635`), dispatches it (:703), and on failed date operation calls `__ALERT "cannot set date ${1}"` (:574-578). `__ALERT` echoes the text (:187-189). `tools/rpi_ctrl_v2.sh:565-569` has the same error contract. Shell quoting preserves percent characters and does not neutralize formatting in the subsequent output path.

Prerequisites: enabled RPI management, deployed `s_rpi_ctrl.sh` wrapper using this maintained helper/error contract, a client able to change Server.HOST_TIME, and a failing date operation. The wrapper under that installed name is not present in this checkout, so exact installed packaging was not exercised. The formatting bug also applies to any other helper error containing percent directives.

Impact: undefined variadic reads, possible process crash or disclosure in published error messages; write directives can cause memory corruption. Specific exploitability or crashes are not asserted or tested. This remains reachable through the already shell-quoted HOST_TIME path after fixing A-01.

Remediation: publish helper output using literal `"%s"` (or the existing raw-update helper) and handle absent diagnostic text explicitly. A stubbed-helper test should prove percent-containing diagnostics remain unchanged; no system clock operation is needed.

### A-03 — Low, functional/local availability: blank ACL lines read before the stack buffer

Location: `indigo_libs/indigo_token.c:169-172`; startup caller `indigo_server/indigo_server.c:1717`.

The whitespace loop tests `len >= 0` only before entering. A line made of newline/spaces/tabs decrements `len` to -1 and reads `buffer[-1]` on its next condition. Blank lines are explicitly allowed by the access-control guide's Device Access Control File Format section. Ordinary valid local configuration can therefore trigger undefined behavior. This is not a remote authorization bypass or confirmed secret disclosure.

An isolated fixture extracted the unchanged current `indigo_load_device_tokens_from_file()` function, stubbed logging/ACL setters and loaded a file containing one newline. Apple clang 21.0.0 (clang-2100.3.34.2), `-std=c11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`: build exit 0; fixture terminated with signal 6; UBSan `index -1 out of bounds for type 'char[562]'`; ASan `stack-buffer-underflow`, READ of size 1, buffer at frame offset 32/access offset 31. Symbolizer startup warnings limit stack symbolization, not the reported bounds violation. No installed library or stale build artifact was used.

Remediation: test the nonnegative index in the loop condition and retain the empty-line skip. Regress newline-only, whitespace-only, comments and ordinary records under sanitizers. Register as functional/local availability, not a network security bypass.

## Deduplication and dispositions

- `FND-000013` already tracks publishing devices before attach completes (`indigo_bus.c:718-731`). This also precedes applying the configured token; preserve as impact on that lifecycle finding, not a new record. No timing stress was run.
- `FND-20261023101753-bc8d-complete-repository-audit-035` already covers the driver registry capacity boundary (`indigo_client.c:209`). Do not duplicate it as a loading finding.
- Existing browser/server findings for UNLOAD (-040), fractional timezone (-038), image error-response deletion (-039) and sexagesimal signs (-037) are not new security findings.
- Dynamic loading via LOAD is documented management behavior (`INDIGO_SERVER_AND_DRIVERS_GUIDE.md:136-144`). Reviewed forwarding to `indigo_load_driver()` and the library loader; no independent malicious-library upload route was established. Restricting administrative paths requires a separate policy decision.
- Authored browser assets use Vue text interpolation for messages/labels. A scan found no `v-html`, `innerHTML`, `document.write` or eval sink. BLOB links accept absolute http/https, prefixing other strings with the page origin (`components.js:1457-1465,1521-1528`); the direct javascript-URL lead is not supported. External HTTP(S) links/images alone do not establish XSS.
- Browser registry object keys (`indigo.js:389-415`) and hostile upstream metadata were not exhaustively simulated; no prototype-pollution exploit is claimed.
- The transport reviewer reports absent WebSocket Origin checks. Cross-origin interoperability policy and browser/deployment restrictions require clarification/testing; this assignment does not assert a demonstrated cross-site compromise.
- HTTP BLOB mutation permissions belong to the transport review, avoiding a duplicate finding.
- Startup ignores ACL-load failure and token parsing is permissive. Failure policy merits clarification, but no extra security bypass is claimed under the documented token model.

## Evidence and limits

All commands used `/Users/polakop/Development/indigo` at the reviewed HEAD; only task evidence was changed. Discovery used Git inventories, grep, sed, nl and Python, never ripgrep. Active summaries came from `./.factory/scripts/findings list`, with targeted active-record inspection for deduplication. No task/findings archive was bulk-loaded. No protected driver files, vendor source, legacy `indigo_tests`, live service, hardware, credential or deployment was accessed.

Reproduce the ACL fixture from the repository root (temporary files automatically removed):

```python
from pathlib import Path
import subprocess, tempfile
source = Path('indigo_libs/indigo_token.c').read_text()
body = source[source.index('bool indigo_load_device_tokens_from_file('):source.index('\nbool indigo_save_device_tokens_to_file(')]
prefix = '''#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#define INDIGO_NAME_SIZE 128
#define INDIGO_VALUE_SIZE 512
#define INDIGO_ERROR(...) ((void)0)
#define INDIGO_DEBUG(...) ((void)0)
typedef unsigned long long indigo_token;
void indigo_set_master_token(indigo_token t) { (void)t; }
bool indigo_add_device_token(const char *d, indigo_token t) { (void)d; (void)t; return true; }
'''
with tempfile.TemporaryDirectory(prefix='indigo-acl-audit-') as temp:
    p = Path(temp)
    (p/'fixture.c').write_text(prefix + body + '\nint main(int argc, char **argv) { return indigo_load_device_tokens_from_file(argv[1]) ? 0 : 1; }\n')
    (p/'blank.idac').write_text('\n')
    subprocess.run(['clang', '-std=c11', '-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', str(p/'fixture.c'), '-o', str(p/'fixture')], check=True)
    result = subprocess.run([str(p/'fixture'), str(p/'blank.idac')], capture_output=True, text=True)
    print(result.returncode, result.stderr)
```

This is source-contract review plus one isolated sanitizer case, not a full penetration test. Unperformed: RPI deployment/wrapper integration, actual helper invocation, native server runtime, browser cross-origin tests, hostile-upstream simulation, library provenance audit, Linux/Windows execution, concurrency stress, dependency CVE audit, exhaustive driver/property review. The coordinator runs configured final checks and owns canonical registration and repair assignments. High security findings remain open and block product signoff; audit delivery is not a safe-release certification.
