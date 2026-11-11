# Specification

Status: approved (owner approved the plan on 2026-11-11)

## Problem and outcome
Provide a standalone Debian package with the command line plate solver, mutually exclusive with the full package.

## In scope
- `make package-solver` on Linux builds `$(INDIGO_PACKAGE)-solver-<version>-<build>-<arch>.deb` (`indigo3-solver` on 3.x).
- The package installs only `/usr/bin/indigo_solver`.
- Main package control gains `indigo3-solver` in `Provides` (versioned) and `Replaces`, so installing it over the solver
  package takes `/usr/bin/indigo_solver` over and dpkg drops the emptied solver package.
- Neither package declares `Conflicts` on the other: apt resolves a conflict by offering to remove `indigo3` (and
  `ain-imager-indigo3`). Instead the solver `preinst` refuses the install, with a prominent message, while `indigo3` is
  installed, unpacked or half-configured.
- Docker deb builds produce and copy out both packages; `clean-all` removes the solver package output.

## Out of scope
`indigo_solver_indexer`, repository publishing, non-Debian platforms.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | `make package-solver` builds `indigo3-solver` with version equal to `indigo3` (`$(INDIGO_VERSION)-$(INDIGO_PACKAGE_BUILD)`), containing only `/usr/bin/indigo_solver` | build, `dpkg-deb -I/-c` |
| AC-002 | `indigo3-solver` installs alone and the binary runs | scratch dpkg root install, run binary |
| AC-003 | `indigo3-solver` refuses to install while `indigo3` is installed, and apt does not offer to remove `indigo3` | preinst run, owner apt run on the host |
| AC-004 | installing `indigo3` over `indigo3-solver` removes it and takes over `/usr/bin/indigo_solver` | scratch dpkg root install |
| AC-005 | `indigo3` still builds and declares `Provides: indigo, indigo3-solver (= version)` and `Replaces` incl. `indigo3-solver` | build, `dpkg-deb -f` |
| AC-006 | Docker targets build and copy both packages | `make -n deb-arm64-pkg`, `bash -n tools/build_debs.sh` |

## Failure behavior and edge cases
Prerelease builds use the same `INDIGO_PACKAGE_BUILD` (with `~marker`) for both packages. On 2.x the name becomes
`indigo-solver` with the same relationships to `indigo`.

## Permissions, data, and security requirements
Only a `preinst` that queries the `indigo3` status; no services or users in the solver package. No secrets involved.

## Compatibility and nonfunctional requirements
Runtime dependencies follow `ldd build/bin/indigo_solver`: libc6, libstdc++6, libgcc-s1, zlib1g (bookworm names).

## Assumptions and open questions
Assumption: the stable 2.x `indigo` package does not ship `/usr/bin/indigo_solver`, so no relationship is declared.

## Owner decisions and requirement changes
2026-11-11, Rumen G. Bogdanovski: plan approved ("sounds good goahead"); indexer stays out.
2026-11-11, Rumen G. Bogdanovski: the first `Conflicts` design made apt offer to remove `indigo3` and
`ain-imager-indigo3` on the host; replaced by the refusing `preinst`, with the message made prominent on request.
Affects AC-003, AC-005.
