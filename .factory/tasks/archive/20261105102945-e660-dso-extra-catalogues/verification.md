# Verification report

Verdict: pass
Reviewer: Claude Code, Opus 5.5
Date: 2026-11-05
Revision and working-tree state: base 64aaf73f2 with uncommitted task changes
Review independence: same agent as the implementer; no fresh-context verifier was used.

## Acceptance evidence
| Criterion | Result | Evidence and rationale |
| --- | --- | --- |
| AC1 | pass | 13013 entries, 0 out of range. Spot checks: B33 5.68h -2.45, Abell39 16.46h +27.91, Sh2-101 20.00h +35.28, Sh2-155 22.95h +62.64. B343 lies on a dark patch of a solved Cygnus frame. |
| AC2 | pass | 1150 read, 928 added, see generation-report.txt. Sh2-105 merges with NGC6888 0.8 arcmin apart. Merges by position are heuristic. |
| AC3 | pass | The only removed line in the diff is the description terminator line; 0 duplicate ids. |
| AC4 | pass | Header diff appends `DARK_NEBULA`; "Dark nebula" lists 345 entries. |
| AC5 | pass | Sky map candidates are 519 on both tables. |
| AC6 | pass | Generator writes r1 = r2 and angle 0; 121 added objects have size 0. |
| AC7 | pass | See Size. |

## Size
Built with `-g -fPIC -O3 -std=gnu11 -pthread`; the other archive members are unchanged.

| File | Baseline bytes | New bytes | Change |
| --- | --- | --- | --- |
| indigocat_dso.o | 1570224 | 1689144 | +118920 (+7.6%) |
| libindigocat.a | 17240664 | 17359584 | +118920 (+0.7%) |
| libindigocat.so | 17091776 | 17224288 | +132512 (+0.8%) |
| libindigocat.so, stripped | 16944592 | 17076952 | +132360 (+0.8%) |
| ain_platesolver (static link) | 39035168 | 39102256 | +67088 (+0.2%) |

## Source comparison
- Sharpless, SIMBAD against VII/20: median 1.3 arcmin, 10 objects over 30 arcmin, up to 897 arcmin (Sh2-62). VII/20 positions are used because SIMBAD has Sh2-105 about 2 degrees from NGC 6888 while VII/20 matches it.
- van den Bergh, SIMBAD against VII/21: 149 objects, median 2.4 arcmin, largest 8 arcmin.
- Abell, Collinder and Melotte positions come from SIMBAD alone and have no second source check.

## Checks
| Command | Result |
| --- | --- |
| `./.factory/scripts/check` | pass |
| `./.factory/scripts/security` | pass |
| `./.factory/scripts/test` | 197 run, 12 failures; the same 12 tests fail on a clean detached checkout of base 64aaf73f2 |
| `indigo_test/unit/test_indigocat_corrections.c` linked with the new archive | all passed |
| Ain Platesolver linked with the new archive, solved Cygnus frame | Deep sky 11, was 4 |

## Owner decisions
The owner was told about the 12 pre-existing Factory test failures and asked for an exception, then replied "please commit". This is recorded as the exception for this task only. Licence status is in [licence-review.md](licence-review.md).

## Limitations
- `make -C indigo_test test-unit` was not run, because the task worktree has no library build; only the indigocat test was run.
- Linux arm64 only. Windows and macOS were not built; the change adds only C data and an enum value.
- Collinder has 170 of about 471 and Melotte 74 of about 245 numbered objects in SIMBAD; those totals are from memory.
- Large catalogue diameters such as vdB36 (820 arcmin) and Sh2-109 (about 18 degrees) are published values and draw very large circles.
