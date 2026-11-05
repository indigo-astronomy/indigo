# Specification

AC-001: Inventory every public Sequence method and associate it with executable scenarios and a coverage classification. Construction alone is not runtime coverage.

AC-002: Execute prepared JavaScript through the real Scripting Agent against actual simulator drivers and owning science agents. Cover loops, conditional flow, policies, waits, selectors, camera settings/acquisition, focus, guiding, mount/dome/rotator/GPS and solver/flip controls. Record unavailable prerequisites explicitly.

AC-003: Assert observed state, values and completion for individual cases; distinguish script evaluation from sequence completion. Test abort, pause/resume, reset, failure, recovery, restart and timer cleanup. Bound every wait and stop test-owned operations on failure.

AC-004: Keep scripts reusable, use private output storage and only simulator selections. Never reconfigure physical devices or overwrite user scripts/configurations. Preserve a machine-readable report and exact invocation.

Complexity: high, due to asynchronous workflows across multiple agents, 108 public methods and simulator limitations. Risk: medium, simulator-only controls with local file creation; no production code changes planned.

Owner scope clarification: deliver tests and documented findings only. Product defects may remain reproducible failures; the task acceptance concerns complete and honest test execution, not a green product suite. No production repair or security-risk acceptance is authorized. The test transport relay must be explicit in documentation and results and may forward only real requests, never fabricate agent replies. Native token propagation remains a separate direct regression.
