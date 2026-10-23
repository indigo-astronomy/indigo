# Management security repair developer evidence

Task: 20261023121227-a609-fix-server-security-high. Finding scope: FND-20261023112118-8e5e-server-security-audit-008 and -009; acceptance AC-004. Assigned isolated worktree: `/private/tmp/indigo-security-management`, detached at `989ec8db122f65ea3813af9db13d510d18bee90e`. Developer/test designer is the same agent; these are developer checks, not independent verification. Requested strong gpt-6-astra/high; actual inference settings not independently observable.

## Changes

- Validate every selected incoming INSTALL name against canonical items before mutating canonical values. Reject the complete request with ALERT if any selected item is unknown, including a valid first item followed by an unknown selected item. Retain first-selected behavior for valid requests and no execution for unselected requests.
- Pass the selected canonical item's name through existing `shell_escape()`, using `INDIGO_NAME_SIZE * 4 + 3` capacity. Maximum expansion plus the fixed command fits execute_command's 1024-byte buffer.
- Publish helper diagnostic data through literal `%s`; preserve the entire ALERT line if no colon exists. Also use literal `%s` for the same function's strerror diagnostic path.
- Add an opt-in, dependency-free source-extracted regression runner and C harness in `indigo_test/security/`. No source/property additions or removals, token changes, helper script changes, driver changes, or documentation modifications.

## Inputs and scope

Read assigned task brief/specification/plan/test-plan/handoff/state from the main checkout; isolated root AGENTS.md, project/state, developer and test-designer roles, complete indigo_test/AGENTS.md; root README, DEVELOPMENT, relevant server guide and token-contract documentation. Explicitly reused archived audit `access-security.md` and active canonical 008/009 records. Active finding summaries read through coordinator checkout. No scoped server AGENTS.md exists in Git inventory. Temporary worktree contains only assigned modifications. No independent finding closure is claimed.

## Exact verification commands and results

All commands below ran from `/private/tmp/indigo-security-management` on the source working tree unless noted.

1. Before production edits, `python3 indigo_test/unit/run_server_management_security.py --sanitize`: initial harness compilation failed because extraction matched the change_property forward declaration. Corrected extraction to require the full definition signature and opening brace; this was a harness defect, not a product regression.
2. Before production edits, repeated `python3 indigo_test/unit/run_server_management_security.py --sanitize`: compiled, exit 1; 10 tests total, 7 expected failing regression assertions, 3 passed. Failures: exact argument quoting, unknown selected name accepted, mixed known/unknown selection accepted, canonical shell syntax not quoted, maximum apostrophe token not quoted, helper diagnostic passed as format, missing-colon diagnostic absent. No hostile command ran; popen was intercepted. The publication stub recorded unsafe format values without evaluating missing variadic arguments.
3. After production edits, `python3 indigo_test/unit/run_server_management_security.py --sanitize`: exit 0, 10/10 passed, no sanitizer diagnostics.
4. `python3 indigo_test/unit/run_server_management_security.py`: exit 0, 10/10 passed.
5. Coordinator requested relocation because unit tests require a linked library; moved tests into dedicated security directory, added canonical second-index and unselected-unknown-name coverage, and asserted canonical selection is preserved on unknown rejection.
6. Final `python3 indigo_test/security/run_server_management_security.py --sanitize`: exit 0, 12/12 passed, no ASan/UBSan diagnostics.
7. Final `python3 indigo_test/security/run_server_management_security.py`: exit 0, 12/12 passed.
8. `git diff --check`: exit 0, no whitespace errors.
9. `cc --version`: Apple clang 21.0.0 (clang-2100.3.34.2), arm64-apple-darwin27.0.0. `uname -sm`: Darwin arm64.

The runner invokes `cc -std=c11 -D_GNU_SOURCE -D_DARWIN_C_SOURCE -DRPI_MANAGEMENT -O1 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -I<root>/indigo_libs -I<temporary> <root>/indigo_test/security/test_server_management_security.c -pthread -o <temporary>/test_server_management_security`; sanitizer mode appends `-fsanitize=address,undefined -fno-omit-frame-pointer`. Each compile has a 60-second timeout and each test process a 30-second timeout. Extracted source and binary are confined to an automatically removed TemporaryDirectory. No shared build artifacts or servers remain.

## Coverage and limitations

The runner compiles source-current, unchanged text of shell_escape, execute_command, and the complete RPI INSTALL prefix of change_property, and calls that entry with real public INDIGO property types. Remainder of change_property is excluded. Property matching/copy/publication and popen/pclose are inert stubs; shell invocation strings are inspected, never executed. Assertions cover single-argument quoting including apostrophes, semicolons, substitution-looking text, backticks, wildcards, quotes, backslashes and percent characters, and the maximum canonical name expansion. Stubbed helper replies cover OK, percent-bearing ALERT, missing-colon ALERT, empty reply, and popen failure.

Not exercised: complete linked server, live bus/token gate, deployed RPI wrapper/package manager, operating-system clock/helper actions, Linux/Windows runtime, or real shell parsing. Existing token semantics are preserved by leaving those paths unchanged. These tests establish the bounded management selection/formatting contract only. Full candidate checks and independent security/quality review remain coordinator responsibilities. Coordinator owns opt-in Makefile registration (`test-server-management-security`) and any Xcode group decision; pre-existing owner Xcode diff is untouched. No actual management operations, network, package installs, hardware, commits, merges, publication or registry edits occurred.

## Handoff

Uncommitted patch: `/private/tmp/indigo-management.patch`; apply from repository root. Changed-file identities:

- `indigo_server/indigo_server.c`: SHA-256 `e19035e33121e3665ca35c90e5a22879072504d6540de4bcfc4a593fc140be46`
- `indigo_test/security/run_server_management_security.py`: SHA-256 `5c58a8524afe1722f2469b303192c96fad91c3a0fe3eccacff2144376184101d`
- `indigo_test/security/test_server_management_security.c`: SHA-256 `18a3f8d0335053c8719d7183e9ef05bc647932056a70093fbb9314297ef4523c`

Next: coordinator integrates the patch and opt-in target, reruns focused regressions on the combined final source, and obtains independent review of management changes. Developer is available for independent transport/parser review (different ownership), with explicit role load and candidate identity required before that review.
