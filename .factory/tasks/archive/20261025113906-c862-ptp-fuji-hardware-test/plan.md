# Development plan

Status: final

1. Build master on the Pi in its own task branch, run the existing hardware suite against the X-T1 (Sony charger also present).
2. Make the suite vendor-neutral for Fujifilm labels and RAF; diagnose remaining failures from debug traces.
3. Fix D-FJ1 (Bulb offered without USB BULB), D-FJ2 (rejected write not read back), D-FJ3 (non-PTP vendor device attached) with fake regressions, each confirmed failing without its fix.
4. Diagnose USB BULB on the X-T1 with a temporary Pi-only build, remove it; recorded hardware and fake runs, sanitizers, records, review, archive, commit.
