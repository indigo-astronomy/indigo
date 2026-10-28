# Verification

Verdict: pass
Reviewed source: working-tree change of Makefile over `750174b3f`; source identity in process.json. Same agent as the author.

## Results
- AC-001: `git diff --word-diff Makefile` shows only: `ccd_qhy2` appended to STABLE and removed from UNSTABLE; `ccd_pentax agent_solver` appended to UNTESTED; `ccd_pentax` and `system_alpaca` removed from DEVELOPED (now empty); `ccd_pentax` removed from EXCLUDED (now `agent_test`).
- AC-002: `make Makefile.inc` regenerated lines 34-39 with the same lists. `make -n -f ../Makefile.drvs folders` in indigo_drivers prints `Folders excluded: agent_test/` and an empty `Folders excluded from install`.
- AC-003: `make -f ../Makefile.drvs ccd_pentax/ system_alpaca/ agent_solver/ OP=all` in indigo_drivers (macOS, clang, universal x86_64/arm64) succeeded; build/drivers contains indigo_ccd_pentax, indigo_system_alpaca and indigo_agent_solver executables/libraries.
- AC-004: the `## Testing` sections of all 33 UNTESTED_DRIVERS READMEs were listed. Passing real-device records: agent_alpaca (2026-10-01/04 ConformU FlipFlat, Ultimate Powerbox 3, Pocket Powerbox, Mount LX200), ccd_libcamera (2026-10-25 linux arm64 ov5647 10/10 OK), ccd_pentax (2026-10-27 linux arm64 PENTAX K-m bulb, DNG, PEF single frame OK). These three were moved; the remaining 30 have only simulator, fake SDK, Arduino simulator or loopback records, except polaralign_mlastro whose one hardware record failed (18/14). Final lists: UNTESTED ends with `polaralign_mlastro agent_solver`, STABLE ends with `ccd_qhy2 agent_alpaca ccd_libcamera ccd_pentax`.

## Limitations
Linux and Windows builds of `ccd_pentax` were not run here; `make install` was not run. No driver tests apply to a build-list change.
