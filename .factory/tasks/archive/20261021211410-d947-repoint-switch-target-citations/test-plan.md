# Test plan

Documentation change only, so no product tests apply. Checks: `git grep` for citations without the new wording, `git show 85c7c3d9207b:indigo_drivers/REVIEW_SWITCH_TARGETS.md`, a scan of lines that now start with the citation, `git diff --check`, `./.factory/scripts/check`, `./.factory/scripts/findings check` and the committed-range process check.
