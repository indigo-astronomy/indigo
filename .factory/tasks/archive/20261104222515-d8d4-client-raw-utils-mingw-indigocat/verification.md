# Verification report

Verdict: pass
Reviewer: coordinator
Date: 2026-11-05
Revision and working-tree state: product commit `91f304911` (by the owner, same content as the verified working tree) on base `67a7e06fd`
Review independence: same agent as the author.

The verdict covers what was executed on Linux. The owner accepted the MinGW and macOS builds below as open when asking for this record to be committed on 2026-11-05, and tests them after integration.

Linux aarch64, working tree on the task branch, base 67a7e06fd.

| Command / check | Result |
| --- | --- |
| make -C indigo_libs libindigo_client | passed; indigo_raw_utils.o in the .a, indigo_find_stars_fast_threshold exported by the .so, no undefined indigo symbols |
| nm build/lib/libindigo.a | still defines indigo_find_stars_fast_threshold |
| make -n VERSION=x ARCH=x bundle in indigo_windows/gcc | rule graph complete, 19 of 19 catalogue objects in the DLL link, bundle copies the new files |
| native gcc compile of the new MinGW object rules with the Windows-only flags overridden | passed for indigo_raw_utils, indigocat_dso, indigocat_precession |
| INDIGO_EXTERN on every public function of indigo_raw_utils.c and on the indigocat symbols Ain Platesolver uses | present, so they are exported from the DLLs |
| ./.factory/scripts/check | passed |
| make -C indigo_test test-unit | rc=0, 427 PASS lines, 0 FAIL lines |
| Ain Platesolver linked without its copy of indigo_raw_utils.c, solving a real FITS frame | same solution as the agent |
| Ain Imager, Viewer, Guidelog Viewer relinked against the new libindigo_client.a | all link |

## Not performed

- MinGW build: no MinGW compiler on this host, so the DLLs and the SDK bundle have never been produced.
- macOS build: no macOS host. The Makefile change is shared with Linux.
- indigo_test integration suite.
