# Plan

1. Read records and scoped rules; capture existing library and imager baseline.
2. Prepare disjoint read-only subagent patch proposals for image/math, agents, and core I/O; coordinator remains sole repository writer.
3. Add regression cases and apply minimal production changes, bump affected driver version and document behavior.
4. Rebuild shared library and affected agents, run unit and relevant integration suites plus configured Factory checks.
5. Review final diff independently where feasible, reconcile all selected findings, archive evidence and commit a tested candidate. Leave integration/publication pending under existing authorization.
