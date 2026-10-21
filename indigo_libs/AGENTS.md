# Implementation rules

Follow the root AGENTS.md and explicitly assigned role. This subtree contains the C/C++ INDIGO bus, protocol, base drivers and portable I/O layer. Preserve public ABI and platform support; keep operating-system-specific I/O here rather than in portable drivers. Vendor trees under externals/ remain vendor-owned. Tests live in indigo_test/unit/ and indigo_test/integration/.

- Keep business logic, external integrations, and presentation concerns separated according to the existing architecture.
- Preserve existing APIs unless the specification authorizes a change.
- Validate external data at boundaries and enforce authorization in trusted code.
- Use explicit error handling; avoid sensitive information in errors and logs.
- Avoid hidden mutable global state and unnecessary abstractions.
- Explain dependency additions and update the lockfile using the project's package manager.
- Include useful tests for behavior and failure cases proportional to the change.
- Use English for new comments and implementation documentation.
