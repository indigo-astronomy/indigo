# Software delivery decision

Owner: Peter Polakovic. On 2026-10-27 the owner instructed `urob commit, merge, push` after the repair report stated version3.0.0.4, passing36/36 tests and that physical-99 resolution remained unverified. This authorizes committing the reviewed software repair, fast-forwarding master and pushing origin/master. It does not turn either earlier failed hardware attempt into a pass.

The owner previously explicitly deferred physical acceptance for software delivery (`ok, urob commit, merge a push. budeme pokracovat neskor na linuxe`) and subsequently requested this comparison/repair while leaving the camera asleep. Delivery therefore retains physical validation as a follow-up with notice and wake confirmation before any camera use. No new hardware was used for closure. The old failed3.0.0.3 hardware record remains in README/TEST_SUMMARY, and3.0.0.4 has only a passing fake SDK record.

Acceptance is software-scoped: original acquisition behavior, ownership-safe failed-claim rollback, conservative retry safeguards, normal build dependency, version bump and independent reviewed regression evidence. Actual OS binding identity, physical capture and Linux/Windows runtime are unverified. The task is not full-driver or physical certification.

The full repository audit remains overdue; existing owner-scoped Pentax completion deferral covers this continuation. No unrelated work or audit checkpoint reset. Keep the follow-up limitation in the final response and retained evidence. No approval is inferred beyond the owner's explicit delivery instruction and existing deferrals.
