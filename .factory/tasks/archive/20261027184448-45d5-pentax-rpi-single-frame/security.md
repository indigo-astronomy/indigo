# Pentax capture security review

Verdict: pass

## Scope and independence

Independent in-host security reviewer; no implementation writes, no camera, SSH, or other hardware access. Reviewed the complete uncommitted Pentax production, hardware harness, fake USB, recorder, and fixture-test changes over synchronized task HEAD `8251b1aab0eba4604fe7e12f31a133f3f7144949` (includes latest master target `a5b41ff892ce937576685db11b5ceef14f02d9e2`). Root owns implementation and final integration. This review covers task changes, not the entire INDIGO server or all Pentax models.

## Trust boundaries and findings

No new blocking security findings were identified in the reviewed diff.

- Camera protocol arguments are encoded into a fixed16-byte stack buffer only after validating nonnull input and count1..4. Nonlegacy packed writes are bounded; legacy offset writes retain4-byte framing. Existing response/segment bounds, CSW validation, allocation limits, readout deadlines, and transport failure propagation remain intact.
- Settings and exposure transitions remain under the per-device mutex. ISO/settings handlers reject changes during an active capture; periodic status polling skips active captures. The trigger timestamp is captured after parameter staging and before command dispatch. Readiness polling cannot restart the absolute deadline. Existing abort, shutter-close retry, saved-state restoration, and disconnect ownership paths remain present. Timer-origin repair adds no privileged operation, entitlement, shell command, dependency, or public API.
- A physical B state can contain a nonnumeric shutter; it is accepted only for known B program values and restoration does not issue an invalid timed-shutter setter. Timed programs retain numeric validation.
- Optional native image persistence is local operator-controlled test behavior. Relative artifact paths reject before USB initialization; final files use exclusive `wbx` creation, so existing files and final-component symlinks are not overwritten. Write/close failures fail validation. The absolute parent directory is an operator trust boundary; test launchers use unique task-owned temporary directories. Native image bytes remain outside version control.
- The single-frame exposure override rejects nonfinite, out-of-range, and trailing-junk values before USB initialization. Physical tests remain explicit opt-in profiles with watchdogs and cleanup.
- Recorded partial profiles require literal known Make profile values and distinct scope labels before build/execution, preserving full-camera records. Alternate Make assignment syntax and expansions are rejected by the existing guard. The guard protects evidence semantics; it is not a general sandbox for operator-supplied Make arguments.
- Added debug status values contain camera settings, not credentials or native image data. Existing verbose USB logging can include native image prefixes, so retained logs must be screened/redacted before publication. Current task-tree inspection found no binary artifacts or data-read lines larger than412bytes. The final expanded evidence tree was rechecked after baseline/final Linux/macOS logs and findings were added; no binary payload,64-byte hex dump, or credential-like assignment was found. The coordinator retains responsibility for any later evidence additions.

## Executed checks

Working directory: `/private/tmp/indigo-pentax-rpi`, macOS arm64; working tree above the synchronized task HEAD, no camera usage.

- `./.factory/scripts/security`: exit0; configured `python3 .factory/scripts/validate_project.py --security` reports local security boundaries valid.
- `python3 -B -m unittest discover -s indigo_test/unit -p test_driver_test_recording.py`:25tests,25passed, exit0.
- Standard-library recursive inspection of this task's files: no NUL-containing binary files and no USB `bytes read` entries over412bytes at review time. Actual RAW/JPEG files were not copied into the task tree.

## Final synchronized-tree recheck

The five source identities below are unchanged at synchronized HEAD `8251b1aab0eba4604fe7e12f31a133f3f7144949`; the additional master merge contains unrelated solver process evidence. Reinspected final generated README/summary records,44/1 inventory count, task reports/logs and the five closed functional findings. These changes add no sensitive payload or security-relevant runtime behavior.

`MIGRATION_STATUS.md` changes only the Pentax test inventory from36/1 to44/1. A conservative data-model path signal does not represent a schema or stored application-data migration here; no data migration, permission, authentication, credential or deployment boundary changed. The report covers this signal explicitly without claiming database validation.

Reran `./.factory/scripts/security`: exit0, local security boundaries valid. A standard-library scan of every final task file found no NUL bytes, no64-byte hexadecimal runs, and no credential-like `token/password/secret/private-key` assignments. Short camera status/protocol excerpts remain; large native-image dumps were redacted. Images are retained outside Git.

## Limits

Physical shutter/EXIF accuracy of the final offline timing correction is not verified and must not be inferred from transfer success or fake USB timing. No new hardware access is needed for this security review. No Windows runtime, all-model certification, whole-repository audit, or resolution of pre-existing out-of-scope security findings is claimed. The role's referenced `indigo_docs/security.md` does not exist in this checkout; the actual diff and configured checks were reviewed directly. No owner risk acceptance is claimed.

## Reviewed source identities

- `indigo_drivers/ccd_pentax/indigo_ccd_pentax.c`: SHA-256 `b7b78630bfeb2855891ec1450885e5dd9d53736e9132bdf000ee3c4584651830`
- `indigo_test/hardware/test_ccd_pentax_hw.c`: SHA-256 `0532e0c8142db31a1536d4f346c9ff32592fb5ace1ba882024a62dc9232daf55`
- `indigo_test/integration/test_ccd_pentax_usb.c`: SHA-256 `07c8505e1e55f002e29742b056467e96a86e5494225ff2cef0158a7f99ca40c6`
- `indigo_test/unit/test_driver_test_recording.py`: SHA-256 `5de9c573c95ad326239df6f20bba7045b27d6daeab35082c44efd602a8728250`
- `tools/run_driver_test.py`: SHA-256 `f76053b17275f2d422462bb982f8fcf57aac44a552addd22290fd92a9486248b`
