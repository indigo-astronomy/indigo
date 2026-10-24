# W-side experiment quality review

Verdict: pass

Reviewed base 344aba0b57889fd2dfa0dc3e94e19530252e0dcd plus current W probe, complete JSONL, existing orientation finding update and concise REFACTOR comparison. Same-agent sequential review, not independent review. Recomputed both repetitions of each signed DEC change, checked idle noise, actual settled Gm=W and every trial's before/during/after W state. Compared direct n/s signs and retained E measurements; both sides show 0 south / 1 north. No claim about a specific electrical/firmware component follows from this data.

Reviewed the W-target selection change: sidereal+1h and MD=1 before ordinary MS, existing limits unchanged, bounded arrival and actual W validation before pulses. The previous small RA-axis excursion guard is intentionally inapplicable to the newly owner-authorized larger side-changing move; no unsupported forced side flag is used. Cleanup trace returns exact initial motors, original park/side E, tracking off/R6 and saved target acknowledgements. Code/mapping/version remain unchanged; only evidence/finding/doc are written. Existing finding is extended, not duplicated or confused with repaired numeric encoding. No blocking quality finding in this scoped experiment. Verification.md owns results, limits and check evidence.
