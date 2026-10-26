# Owner-authorized delivery scope

Peter Polakovic, explicit chat instruction on 2026-10-26: "ok, urob commit, merge a push. budeme pokracovat neskor na linuxe".

The owner authorizes committing, integrating into local master and pushing the software implementation now, with physical K-m acceptance deferred to a later Linux session. This changes the delivery gate from hardware-blocked to software delivery with an explicit hardware-validation exception. It does not convert any failed USB attempt into a pass, certify real JPEG/PEF/DNG capture or accept an undisclosed security finding.

Required software gates remain: native build, 31-case production-backed fake USB and sanitizer coverage, configured checks/tests/security, independent final-diff quality/security review, canonical findings integrity, archival and committed-range process validation. The prior checkpoint remains in history; no rebase/amend/squash is permitted. The final closure commit retains the owner decision and archived evidence.

The existing overdue-audit deferral covers this Pentax task only. The full audit remains due before unrelated work or release. No automatic reminder or new Linux task is created. Linux continuation requirements are retained in linux-follow-up.md and the driver REFACTOR record.
