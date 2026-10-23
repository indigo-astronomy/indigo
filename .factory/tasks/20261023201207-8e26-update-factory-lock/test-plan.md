# Test plan

Validate old-to-new upstream source delta, map uniqueness/coverage and byte identity of repair files. Verify the plan at the updated pin has no missing updates. Run configured check/test/security, plutil and independent PBX inventory/object checks. Bind reviewed Git objects and run committed-range process gates over the added provenance commit and combined repair range. Confirm published refs equal tested commits before cleanup. No new implementation tests needed for a provenance-only update.
