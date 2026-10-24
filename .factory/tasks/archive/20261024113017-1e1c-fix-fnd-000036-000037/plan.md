# Plan

1. Replace overlapping sscanf field parsing with length-checked span copies and explicit separators; bound item counts before indexing and validate remote endpoint fields.
2. Validate RAW signature/dimensions and size arithmetic, exact reads, ROI conversions/bounds and output I/O. Copy validated rows.
3. Run focused tests with sanitizers and native tool builds; independently review final diff, close verified findings, run Factory gates and archive evidence in one local task commit. No new dependencies.
