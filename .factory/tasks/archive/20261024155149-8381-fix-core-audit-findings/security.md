# Scoped security review

Verdict: pass
Reviewer: core_io (independent image/math review; separate self-review of its alignment parser), image_math (independent persisted-input/caller quality review), root final defensive review. Security-reviewer role explicitly loaded. Date: 2026-10-24.
Revision/state: base `21efe303fe63575c3574f1579285828d9ba568cc` plus final task source content bound in process.json. Review is scoped local defensive data/bounds handling. Actual host model/effort not exposed; current authorized OpenAI host retained, no cross-provider invocation or additional charge.

## Trust boundaries and risk

Saved alignment files are untrusted persisted input. Counts are checked against fixed capacity before any indexing; complete rows are parsed into a temporary model, validated for finite/domain-correct coordinates and known flags, and committed only after all rows succeed. Every opened handle is closed. Rejected files leave model and property counts unchanged.

DONUTS requires at least 12 pixels per included dimension and a fully contained region before projection reads/allocations. This matches background radius 5 and ensures safe calibration/FFT sizes; subtraction checks avoid overflowing rectangle-corner sums. Caller failure remains represented by INDIGO_FAILED. Existing broader image-size/buffer-size contracts were not redesigned or certified.

JSON escaped strings preserve literal backslashes without changing protocol authorization. DNS-SD errors are checked before undefined SDK result fields are consumed; retained resolve context supplies the requested name/interface and is freed with its reference, including immediate/async startup failures. Safe fixtures cover callback failure and lifecycle without real discovery.

Derived image/focus/alignment results are guarded against empty samples, singularity and nonfinite output before motion/target generation. Exposure requests require fresh acceptance and report failure on timeout. No dependencies, secrets, credentials, cryptography, external endpoints, permissions or path access policy were added. No external probing, exploit reproduction or hardware interaction occurred.

## Evidence and disposition

See verification.md for configured security/check commands and ASan/UBSan commands/results. Defensive rejection fixtures ran only against fixed code; vulnerable bounds paths were not executed. Review caught finite-but-out-of-domain persisted coordinates; domain validation fixed it and state-preserving rejection tests passed. FND-20261024121920-3bd2-repository-audit-008 and -037 are fixed within this scope, with exact canonical dispositions in findings.md/registry. Other requested fixes have functional regression evidence there.

No owner risk acceptance or waived gate is recorded. Windows/Linux execution, physical devices, a complete hostile-image dimension audit and whole-server security certification are outside this result. Clean automated checks alone are not the basis for the verdict; source bounds/lifetime analysis and focused executed tests support it.
