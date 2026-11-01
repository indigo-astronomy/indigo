# Developer verification

Task: 20261101213045-9647-scale-repository-audits. Worktree: /private/tmp/indigo-20261101213045-9647-scale-repository-audits. Starting source revision: 2b040e9b6daa120b78459e3f91242b0848615716. Developer implementation/checks are not independent review. Same inherited host/provider; exact model/effort unknown. No application tests, image recognition, hardware or external service used.

## Change

Backward-compatible completion kinds distinguish partial and incremental audits. Partial closure requires exact `Verdict: partial`, retains audit:null, counts as neither work nor full/incremental coverage. Incremental completion uses existing metadata and passing report without replacing full-review checkpoint. Legacy kinds, full-audit input default, old status keys and thresholds remain compatible.

New policy supports incremental task threshold20, disabled full task threshold:null and full calendar threshold90. A frozen baseline object has exactly revision/authority/report_task. Its retained ancestor snapshot must contain the referenced partial report; legacy partial-with-reason token is accepted only for this migration proof. Historical snapshot work completions initialize incremental cadence without claiming reviewed coverage or filtering timestamps. Incremental input lists only uncovered work IDs, preserving unseen concurrent work and previous incremental coverage.

## Actual checks

Working directory above:

- `python3 -B -m unittest discover -s indigo_test/factory -p test_independent_records.py` -> exit0,25/25 tests passed in13.270s. This includes18 preserved and7 new regressions. Output retained externally at /tmp/indigo-scaled-audit-helper-tests.log.
- `git diff --check` -> exit0, no output.

Exact test result:

```text
.........................
----------------------------------------------------------------------
Ran 25 tests in 13.270s

OK
```

New regressions cover twentieth incremental/ninetieth full boundary; disabled full task threshold; partial completion not work/full coverage; chained incremental coverage/unseen older concurrent completion; strict partial archival and pinned proof; incremental input/archival not a full checkpoint; retained baseline versus timestamps/no fakecoveredtasks; missing baseline report/authority and nonancestor rejection. Existing partial-report rejection for full-audit metadata remains intact. No failure was hidden or assertion weakened.

## Scope and release

Only maintenance.py and existing test_independent_records.py changed by developer. No task_store or other helper change needed. Root owns configuration/canonical documents and original audit closure. Files now released; no running writer or test process remains. Independent final review and configured/committed-candidate gates belong to coordinator. Known current original audit state remains incomplete until coordinator archives it with explicit owner partial-closure authorization.

SHA256:

- .factory/scripts/maintenance.py: ab6b145ce19f217f211d7c6df3b7b72d26ecbd11651ae06ad00c5e4cb6d718cf
- indigo_test/factory/test_independent_records.py: e58dd588563eb410f0baf0b04b72a3f8017a403d3680789de16646151caa2ce8
