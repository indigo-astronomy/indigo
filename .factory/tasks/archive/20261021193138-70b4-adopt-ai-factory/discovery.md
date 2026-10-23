# Read-only discovery evidence

Target canonical Git root: /Users/polakovic/Development/indigo
Base/current starting revision: `eba315b7041b1a19f64a1ad7864dc1a6c0019ea8`. Branch master, clean tracked/untracked status; fetched origin refs/heads/master resolves to the same revision. Existing branches/worktree/remotes preserved. One worktree, no submodules reported. Git 2.54.0 (Apple Git-157), Python 3.9.6, macOS arm64. Git user identity is configured; identity values are omitted here. Remote branch protection/PR requirements are unavailable and no integration is attempted.

Template URL: https://github.com/polakovic/ai_factory
TEMPLATE_COMMIT: `11392a11253bae4d1c5d5db97e8a532917728c09`
Separate detached source checkout: /private/tmp/indigo-ai-factory-pin-20261021
The HTTPS fetch first failed for network resolution in the sandbox, then for unavailable HTTPS authentication; the successful SSH fetch obtained refs/heads/main once from the equivalent approved repository. Its fetched SHA was checked out detached and verified clean before template use; the pin was first stored outside the target. No later ref fetch or moving-main template read occurred.

Persistent registry: owner-designated sibling AI Factory checkout; origin is the equivalent approved SSH URL, branch main, clean tracked state. The existing local inventory was valid and ignored before registration; only aggregate results are retained, not its contents or other projects' paths.

## Inspected artifacts
Root AGENTS.md and CLAUDE.md; indigo_drivers/AGENTS.override.md; indigo_test/AGENTS.md; indigo_tools/driver_editor/AGENTS.md; all exact legacy REVIEW.md files including the owner-authorized indigo_tests/REVIEW.md. No ancestor AGENTS.md was present at filesystem/user/development ancestors checked. Vendor libusb has its own AGENTS.md and is left untouched.

Build/layout evidence: README.md, Makefile, Makefile.inc, Makefile.drvs, Makefile.drv, component Makefiles, indigo_test/Makefile and test_runner.h, .travis.yml, indigo.xcodeproj/project.pbxproj, indigo_windows.sln, Directory.Build.props; DEVELOPMENT.md and MAKEFILES.md. README and native CI are retained. No src/ or tests/ application roots are invented. Existing ignored .vs/.vscode/build entries were inventoried by location, not secret/config contents.

## Tracked layout counts at baseline
- `bin_externals`: 5 tracked files
- `indigo_docs`: 59 tracked files
- `indigo_drivers`: 4325 tracked files
- `indigo_examples`: 10 tracked files
- `indigo_libs`: 1485 tracked files
- `indigo_linux_drivers`: 15 tracked files
- `indigo_mac_drivers`: 16 tracked files
- `indigo_optional_drivers`: 12 tracked files
- `indigo_server`: 42 tracked files
- `indigo_test`: 334 tracked files
- `indigo_tests`: 9 tracked files
- `indigo_tools`: 31 tracked files
- `indigo_windows`: 22 tracked files
- `obsoleted_drivers`: 34 tracked files
- `systemd`: 4 tracked files
- `todo`: 3 tracked files
- `tools`: 20 tracked files

## Exclusions and coverage limits
No secret values, local credential files, ignored caches/build outputs or other registry paths were included. Vendor externals/bin_externals, obsoleted drivers and owner-protected driver directories were excluded from broad searches/mutations. indigo_tests is excluded except the approved review-evidence migration/deletion. Template task/findings archives were excluded from routine discovery and never imported. A broad baseline inventory is not a full source audit or hardware validation.

## Integration plan record
integration-plan.json is the pre-bootstrap source/destination/collision table for 71 initial support files. migration.md adds scoped relocations, test-support extensions, project-owned files, legacy deletions and Xcode navigation changes. template-map.json records the final reviewed transferable mapping; project-owned state, checks, maintenance, findings, reports, local review policy and lock are excluded from future automatic updates.
