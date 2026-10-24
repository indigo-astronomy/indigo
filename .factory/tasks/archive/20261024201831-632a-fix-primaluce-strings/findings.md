# Finding repair evidence

This task repairs known defects in the owner's INDIGO software using local synthetic fixtures.

- FND-20261024121920-3bd2-repository-audit-029: replacing unchecked firmware concatenation with remaining-capacity formatting prevents writing beyond INFO. Three original crashing simulator profiles now pass under ASan/UBSan, including a 700-byte application field, a 700-byte web field and two 300-byte fields. Ordinary formatting remains covered by metadata and old_firmware cases.
- FND-000052: AP and STA credential setters share bounded JSON encoding of quotes, backslashes and control bytes. Exact wire-frame, UTF-8/empty, maximum expansion and no-partial-request tests pass in the library suite. Existing request templates are preserved. Public-property AP/STA confirmation passed in the full 76/76 recorded run.

Both findings are closed after the completed acceptance runs; see verification.md and independent quality-review.md/security.md. No other finding is closed or risk accepted by this repair.
