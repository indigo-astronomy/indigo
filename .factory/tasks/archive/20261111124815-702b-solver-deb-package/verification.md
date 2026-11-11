# Verification report

Verdict: pass
Reviewer: Claude Code (claude-opus-5-5), also the implementer
Date: 2026-11-11
Revision and working-tree state: branch task/20261111124815-702b-solver-deb-package from cd4267d46, uncommitted Makefile and tools/build_debs.sh changes
Review independence: not independent; the implementer verified its own change.

## Acceptance evidence
| Criterion | Result: pass / fail / blocked / N/A | Evidence and rationale |
| --- | --- | --- |
| AC-001 | pass | `indigo3-solver` 3.0-11 arm64, contents `./usr/bin/indigo_solver` only, Installed-Size 6096 |
| AC-002 | pass | installed alone (`ii indigo3-solver 3.0-11`); `indigo_solver` prints `INDIGO Solver v.3.0-11` |
| AC-003 | pass | first design failed on the host: `apt install ./indigo3-solver-...deb` proposed removing `indigo3` and `ain-imager-indigo3` (owner declined). Redesigned: `preinst` exits 1 with the banner on this host (indigo3 3.0-10 installed); simulated statuses installed/unpacked/half-configured refuse, config-files/not-installed/unknown allow. Owner reran apt and accepted the result ("seems ok") |
| AC-004 | pass | redesigned packages: `Replacing files in old package indigo3-solver` / `Noting disappearance of indigo3-solver, which has been completely replaced`; afterwards only `ii indigo3 3.0-11`, `dpkg -S` shows `indigo3: /usr/bin/indigo_solver` |
| AC-005 | pass | `Provides: indigo, indigo3-solver (= 3.0-11)`, `Replaces: ..., indigo, indigo3-solver`, `Conflicts: indigo` |
| AC-006 | pass (dry run only) | `make -n deb-arm64-pkg` passes the solver deb name as the 4th argument; `bash -n` clean; Docker not run |

## Executed checks
| Command | Working directory / environment | Exit code | Result and evidence |
| --- | --- | --- | --- |
| `make package-solver` | repo root, Debian 13 arm64 | 0 | package built |
| `make package` | repo root | 0 | built after removing three empty leftover driver directories (see limitations) |
| `dpkg-deb -c / -f` on both packages | repo root | 0 | as in acceptance table |
| `fakeroot dpkg --root=<scratch> --force-not-root --force-depends -i ...` | scratchpad root, main package repacked without maintainer scripts | 0 / 1 for refused conflict | install, takeover and refusal scenarios as above, also reinstall of the solver package |
| `sh <solver>/DEBIAN/preinst install` and with a `dpkg-query` shim per status | repo root, host | 1 / 0 | as in AC-003 |
| owner: `sudo apt install ./indigo3-solver-3.0-11-arm64.deb` | owner terminal, indigo3 3.0-10 installed | — | first design: apt proposed removal (declined); final design accepted by the owner |
| `dpkg-query -W libc6 libstdc++6 libgcc-s1 zlib1g` | host | 0 | dependency package names exist |
| `make -n deb-arm64-pkg`, `bash -n tools/build_debs.sh` | repo root | 0 | arguments and syntax correct |
| `./.factory/scripts/findings check` | repo root | 0 | 25 active, 292 archived, valid |

## Baseline comparison and regressions
The first `make package` failed on the empty, untracked directories `indigo_drivers/agent_astap`, `agent_snoop`
and `agent_solver`, which remained in the local checkout after those drivers were retired or renamed. The failure is
unrelated to this change; the empty directories were removed with `rmdir`, after which the main package built.

## Findings
None.

## Manual checks and limitations
- The Docker build (`make deb-*`) was not run because Docker is not installed on this host.
- The takeover test used `dpkg --root` with `--force-depends` and with the maintainer scripts of both packages
  removed to avoid running them on the host; the solver `preinst` was tested separately. The full apt cycle
  (remove indigo3, install solver, install indigo3 over it) was not reported back by the owner.
- Built only on arm64.

## Verification verdict rationale
All acceptance criteria are covered, AC-006 by dry run only.

## Register reconciliation
No findings created or affected.
