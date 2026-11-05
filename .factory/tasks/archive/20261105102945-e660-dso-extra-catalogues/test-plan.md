# Test plan

| Criterion | Scenario |
| --- | --- |
| AC1 | Table validation: ranges and counts per type; spot checks of known objects |
| AC2 | Generation report review; Sh2-105 merges with NGC 6888; overlay on a solved frame shows no duplicate of it |
| AC3 | Diff has no changed catalogue row; duplicate id check |
| AC4 | Header diff is an append; description printed for every type |
| AC5 | Sky map filter count on baseline and new table |
| AC6 | Generator output format and the validation run |
| AC7 | Baseline and new object, static library and shared library built with the same flags |

Executable harnesses are ad hoc. A permanent test is not added because the table is static data with no logic.
