# Pentax K-m hardware repair quality review

Verdict: pass

Reviewer: pentax_final_quality, independent reviewer context; no production or test implementation. Reviewed state: synchronized task HEAD8251b1aab0eba4604fe7e12f31a133f3f7144949 plus its final working-tree diff in /private/tmp/indigo-pentax-rpi. Scope includes the production control/capture changes, fake USB regressions, hardware profiles/artifacts, canonical recording guard/tests, REFACTOR, inventory, generated Testing records/summary and final task documents. This is a task diff review, not a full repository audit or physical timing certification.

## Method and findings

Read the quality reviewer role/routine and applicable source/test instructions. Inspected complete changed functions and neighboring capture cleanup, status polling, setter, property publication and recorder execution paths. Compared aperture/shutter/ISO command argument contracts with primary pkTriggerCord source (`pslr.c`, setters and command helper). Aperture uses three arguments without WU wrapping; shutter and ISO retain their established wrapping paths. Reviewed the fake's packed nonlegacy and ordered legacy framing checks, initial ISO publication observation, delayed aperture readback, initial Bulb nonnumeric shutter case, and refusal/error paths.

No additional blocking quality defect was found. The argument helper bounds its stack buffer. Setting polling reads until the requested value appears or the two-second settling deadline expires, with a possible final bounded protocol transaction; it does not repeatedly write. Rebuilding ISO choices preserves the observed selection. Bulb cleanup avoids invalid numeric shutter restoration while retaining program restoration, shutter closure and buffer cleanup.

The earlier after-acknowledgement timing assertion tested the wrong endpoint and had no upper bound. Final reinspection confirms that argument staging precedes the monotonic timestamp, then the existing trigger command/status order is preserved. The fixed deadline includes readiness latency. The replacement regression measures trigger command to close with both bounds, using100ms staging and200ms acknowledgement delays; an over-budget1.4s acknowledgement case asserts prompt closure rather than a restarted hold. The final logs show1.205s for1.2s requested and0.000s after over-budget acknowledgement. This supports software command timing, not physical curtain/EXIF accuracy. The independent timing analysis and [verification.md](verification.md) retain the physical limitation.

## Harness and recording integrity

Opt-in single-frame and bulb-only profiles retain payload decode/frame-count assertions and common restoration/disconnection. Full-profile control failures remain test failures while allowing useful subsequent diagnostics. Artifact output uses exclusive creation and checks writes/close. Duration/path preflight rejects invalid values before USB initialization. Profile labels prevent limited runs from replacing full-camera acceptance records. The60s watchdog can terminate before cleanup on a hang; no universal restoration guarantee is claimed.

Independently ran `python3 -B -m unittest discover -s indigo_test/unit -p test_driver_test_recording.py`:25/25 passed. Repeated `git diff --check` on the final source/docs: passed. These checks use isolated synthetic fixtures, not a camera. Inspected final normal/sanitized fake logs and the verification report's44-case results. Full acceptance commands/results belong in [verification.md](verification.md), rather than being duplicated here.

## Documentation and verdict limits

Final specification, plan, test plan, handoff and REFACTOR distinguish historical preparation/failures from final results. The inventory counts44 simulated cases and one conditional hardware case; profile variants do not inflate physical case count. Generated Testing records retain the full mixed-mode failure separately from bulb/DNG/PEF passes. JPEG/RAW images remain outside Git. The final offline timing fix has zero subsequent physical runs, explicitly disclosed under the owner's instruction. Other models, macOS claim recovery, Windows, multiple devices and physical hotplug are not certified.

No unrelated source refactoring, new dependency, generated driver or Xcode change appears in the final diff. Canonical repaired findings are owned by the coordinator in findings.md/registry. Quality review is complete for this candidate; security sign-off, archival, committed-range validation and authorized integration/publication remain coordinator delivery steps.

Reinspection after the latest master advance: HEAD8251b1aab0eba4604fe7e12f31a133f3f7144949 merges solver evidence and maintenance metadata only, with no application/tool source change or conflict. Reviewed updated verification/handoff and the five closed findings. The quality verdict is unchanged; the coordinator reruns configured checks against the advanced target.
