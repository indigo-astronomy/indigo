# Security review

Verdict: pass

Date: 2026-10-23. Reviewed source base/unchanged main target: 989ec8db122f65ea3813af9db13d510d18bee90e. Candidate: final staged product/test/documentation diff, exact identities in final-candidate-identities.json and process.json. This is scoped repair sign-off; no whole-server certification or accepted risk.

## Independence and method

The delegated management developer changed roles to independent verifier/security reviewer for transport, XML/JSON/WebSocket, Alpaca and their tests. It did not implement those changes and separately reran their regressions. Its own management implementation and tests are explicitly excluded from that independent verdict. Coordinator independently reviewed that management diff, surrounding canonical INSTALL/copy/token/helper/shell-escape contracts, source-extracted tests and full RPI source syntax, and repeated the normal/sanitized management suite. Coordinator authored the small Alpaca adaptation and test/build/documentation integration; their independent review is delegated. Model request for substantive child implementation/review was approved-policy strong OpenAI gpt-6-astra/high; actual inference settings/cost are not independently observable. There is no cross-provider review.

See [independent security review](independent-security-review.md) for source coverage, exact independent commands/results, identity addendum and initial corrected invocation failures. Coordinator test/build evidence belongs to [verification](verification.md); developer baseline reproductions and limitations are retained in the three developer reports. Reviews inspected actual diff and relevant contracts, rather than equating passing Factory metadata checks with memory safety.

## Trust boundaries and decisions

Untrusted HTTP lengths/body/progress now reach bounded recoverable private upload storage before exact atomic publication. Aggregate pending/published/retired charge is reclaimed at physical buffer retirement. Registry acquisitions retain entry lifetime across release/clear/resize/shrink; metadata/content snapshots avoid borrowed original item access and cache locks across HTTP I/O. XML and Alpaca maintained consumers acquire/release explicitly. Public entry layout and borrowed legacy APIs remain compatible; external legacy users still require documented lifetime serialization.

XML attribute/name/entity and JSON token/frame bounds reserve terminators; malformed/incomplete messages stop parsing. Upstream inline BLOB ownership/decoded length/padding is explicit per item, including empty/failing multi-item requests. XML dispatch enforces input direction before response-only handlers. WebSocket reads validate uint64 lengths before narrowing, exact components and supported masked text/control/fragmentation; control writes serialize with normal JSON output. Percent-containing upstream XML messages publish literally with NULL absence preserved.

Management INSTALL rejects all unknown selected raw names before canonical mutation, uses the canonical selected item and existing shell_escape with worst-case expansion capacity that fits the final command buffer. The validated version is one shell argument. Helper ALERT and strerror output use literal %s. Synthetic tests intercept popen and never execute a system helper, installation or clock change. Full-source RPI_MANAGEMENT syntax check passed. Existing documented device/master-token coordination semantics are preserved; no new confidentiality/authentication claim is introduced.

No dependency, cryptographic, deployment or third-party integration change. New caps/deadlines and mandatory safety snapshot behavior are documented in existing server/protocol references. MIGRATION_STATUS only changes the Alpaca automated-test count; its manual Comment is preserved. Xcode index adds test references only; owner's pre-existing project reorder remains unstaged and excluded.

## Findings and retest disposition

Audit findings FND-20261023112118-8e5e-server-security-audit-001, -002, -004, -005, -008, -009 and -011 through -017 are fixed and reverified. Canonical records retain origin evidence, link this repair verification and are archived only after this review. Coupled FND-20261023121227-a609-fix-server-security-high-001 (upstream XML diagnostic format) is likewise fixed and reverified. Two preliminary repair defects (control reply serialization and empty cached BLOB realloc) returned to implementation and were independently rechecked; no unresolved duplicate finding remains.

New medium FND-20261023121227-a609-fix-server-security-high-002 records the pre-existing parser-owned cached RO BLOB leak and remains open for a separate bounded follow-up. Five lower-priority original audit findings remain active, as do unrelated existing findings, including compression/SHA-1 follow-ups. No closure or risk acceptance of those issues is claimed.

## Limits and verdict

Passing scope is the 13 selected high findings plus the coupled literal-message fix. ASan/UBSan instrumentation covers repaired core units/harnesses, not every dependency. No Linux/Windows/32-bit execution, TSan, exhaustive fuzzing, hardware, deployed RPI wrapper, broad ConformU or whole-process leak certification. Broader unit-suite pre-existing /dev/null diagnostic mismatch remains explicitly failing in verification.md; it is not an affected acceptance gate. No blocking finding remains in this repaired scope. Accepted risks: none. Integration/publication remain pending separate owner authorization.


Delivery addendum: the owner-authored c35c056edd5d6cf3ff7fa387199cd49f8fcf16e5 changes only Xcode navigation references for five obsolete findings. Parsed object comparison confirms no build-phase/settings, source, test, dependency, transport or authorization change. Coordinator independently reviewed this low-risk delta and refreshed the project identity; previous independent substantive security review remains bound to unchanged repaired code. Scoped pass retained, with all prior limitations unchanged. Merge/push/branch cleanup are now explicitly owner-authorized.
