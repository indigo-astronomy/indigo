# Test plan

| Scenario | Test | Acceptance |
| --- | --- | --- |
| 0.2 file name, catalogue, selection boundaries, seven scales | unit `index_selection` | AC-004 |
| `INDEX_00_2DEG_GAIA` offered for download, not in use list without the file | agent `index_property_reflects_installed_files` | AC-002 |
| labels with the new field ranges, scale list in usage | agent stage-order cases, `test_indigo_solver_tool` | AC-007 |
| 0.2° x 0.16° fields from the index stars, every field with >= 12 index stars solves, at least half solve | simulator `index 0.2 self consistency` | AC-003 |
| the 0.5° case unchanged (threshold 8) with the other indexes unselected generically | simulator `index 0.5 self consistency` | AC-005 |
| real 2 m RCC frame | manual, `indigo_solver` and Gaia cross-match (not automated: owner's frame not committed) | AC-005 |

Not covered: Linux/Windows, a sky-wide real-frame sample. The 100-field random sweep is one-off evidence; the
standard suite keeps the twelve fixed fields.
