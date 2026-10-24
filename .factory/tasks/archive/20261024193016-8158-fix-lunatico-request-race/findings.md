# Finding repair

FND-20261024183351-e8e1-fix-motion-image-alpaca-001 is the assigned functional finding. The unchanged driver deterministically loses held-poll GOTO/SYNC requests across both exports and transport variants. The repair preserves admitted POSITION/STEPS snapshots and pending ownership until their queued worker starts; old polls cannot overwrite their public target/state. Successful private measured position still advances, including return-to-original-position requests. Abort/disconnect clear canceled ownership; compensation reserves motion only after checking pending intent.

The test design and reinspection corrections are recorded in quality-review.md. Exact baseline, negative reproductions and final verification outcomes are in verification.md and evidence/. The final recorded suites passed129/129 and ASan passed11/11; central closure records a verified fix; no additional outstanding security or functional finding was discovered in the final scoped reinspection.
