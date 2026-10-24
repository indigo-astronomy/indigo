# Specification

Complexity medium; risk medium: malformed controller text crosses a fixed-size INFO boundary and client credentials cross JSON framing. Existing transport and property contracts remain unchanged.

- AC-001: Combined firmware text stays NUL terminated inside the INFO value, including oversized application, web and combined fields; ordinary application / web presentation remains unchanged.
- AC-002: AP and station SSIDs/passwords escape quotes, backslashes and all JSON control bytes without changing ordinary or UTF-8 bytes. No partial request is sent if encoding exceeds capacity.
- AC-003: Existing simulator behavior and library golden requests pass; driver version increases and generated files remain reproducible.

No property additions/removals, new dependencies, lifecycle or generator changes. Hardware and Linux/Windows execution are unavailable in this local macOS task.
