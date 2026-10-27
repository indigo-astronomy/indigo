# Test and regression plan

Status: complete

## Coverage matrix
| Scenario | Acceptance | Method | Expected result |
| --- | --- | --- | --- |
| T-001 | AC-001 | `git ls-files` case-insensitive search for `refactor.md`; `find` for untracked copies; count of `refactor/` | none outside `.factory/`; 122 files; renames detected by Git |
| T-002 | AC-002, AC-006 | `git grep` of project files; `plutil -lint`; grep for `vcxproj` and `.factory` in pbxproj; diff classification of remaining pbxproj changes | no matches; lint OK; remaining changes are reorderings |
| T-003 | AC-003 | `git grep -i refactor.md` outside `.factory/` | only the changelog exclusion and the archive pointer in `indigo_drivers/AGENTS.md` |
| T-004 | AC-004 | grep of added non-comment lines in code diff; existence of every cited archive file; build of `system_alpaca` and `test_system_alpaca_omnisim`; `py_compile` of the tool | only the printf message; all files exist; builds OK |
| T-005 | AC-005 | chapter list of each README; diff of `## Testing` sections; comparison with `git show HEAD:` | exact chapter order; Testing unchanged |
| T-006 | AC-007 | `./.factory/scripts/check`, `security`, `test` | pass |

No driver test run is required: no driver behavior changed (comment-only source changes).
