# Specification

Status: accepted (owner instruction in conversation, 2026-10-27)

## Problem and outcome
Port the C solver of the external tester into `indigo_agent_solver` so the agent solves INDIGO RAW frames natively,
manages its Tycho-2 quad indexes outside the repository, and the maintainer can build those indexes with a tool.

## In scope
- Solver core in the driver directory: geometry, quad codes with ambiguity variants, index file format (versioned,
  packed little-endian), HEALPix ring helpers, index selection/loading, image star detection, image quads, hash
  matching with tolerance, star voting, robust tangent plane centre fit with parity.
- `indigo_solver_indexer` tool building `tyc2-<fov>deg.isix` from the Tycho-2 catalogue for the given FOVs.
- Agent: `X_AGENT_SOLVER_INDEX` any-of-many property (items `TYC2_1`, `TYC2_2`, `TYC2_5`, `TYC2_10`, `TYC2_20`);
  selecting downloads the file with curl into `$INDIGO_CACHE_BASE/solver` or `~/.indigo/solver`, validates its
  header and publishes it in `AGENT_PLATESOLVER_USE_INDEX`; clearing deletes it. Native `solve` with WCS output.
- Build registration (driver Makefile, tools Makefile, test Makefile, Xcode), `*.isix` ignored, PROPERTIES.md,
  README.md, REFACTOR.md, MIGRATION_STATUS.md row, driver version bump.

## Out of scope
Swift tester, index hosting, FITS/JPEG uploads, Windows build, integration into master.

## Acceptance criteria
| ID | Observable requirement | Verification method |
| --- | --- | --- |
| AC-001 | The driver builds with the ported core and the tool builds from the shared core | `make -C indigo_drivers/agent_solver -f ../../Makefile.drv all`, `make -C indigo_tools .../indigo_solver_indexer` |
| AC-002 | The index builder reproduces the tester's indexes from the Tycho-2 catalogue | tool run on the catalogue, star/quad counts compared with the tester build |
| AC-003 | Quad codes are invariant under similarity transforms and the index file round trips with validation | unit test `test_agent_solver_core` |
| AC-004 | A synthetic field is solved (centre, scale, rotation, parity) and an unrelated field is rejected | unit test `test_agent_solver_core` |
| AC-005 | The agent reports installed indexes, solves a RAW upload against a selected index, refuses solving without an index or with a non-RAW upload, and deletes a cleared index | integration test `test_agent_solver` |
| AC-006 | The tester frame `image_0.raw` solves with the tool-built 5° index to the known solution and the 20° index yields no false solution | tester harness run recorded in verification.md |
| AC-007 | No index file is tracked by git | `.gitignore` rule and `git status` |
| AC-008 | Property documentation and status tables are updated and the driver version is incremented | review of PROPERTIES.md, MIGRATION_STATUS.md, `DRIVER_VERSION` |

## Failure behavior and edge cases
Missing or truncated index files are reported and skipped; a download that fails validation is removed and the item
cleared; an abort stops the download and the solve; a solve without a selected index, with an unsupported image or
without a consistent set of at least six matched stars fails with an ALERT and a message.

## Permissions, data, and security requirements
Index files are written only under the cache base directory; the download URL is a compile time constant and the
file name is derived from the FOV, never from user input; curl is executed without a shell on POSIX. No credentials.

## Compatibility and nonfunctional requirements
Public platesolver properties unchanged; one new `X_` property. Loading the 5° index takes about one second and
the index stays loaded between solves. The agent is not part of the Windows solution.

## Assumptions and open questions
The hosting URL of the index files is a placeholder (`https://indigo-astronomy.org/solver`) to be confirmed by the
owner. README.md changes need the owner's approval per repository rules; the proposed text is in the working tree.

## Owner decisions and requirement changes
2026-10-27, Peter Polakovic: port the C part only; index files must not go into the project; no commit or push
until reviewed.
