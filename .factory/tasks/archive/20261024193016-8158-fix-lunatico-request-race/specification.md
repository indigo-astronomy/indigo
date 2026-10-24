# Specification

An accepted POSITION GOTO/SYNC or STEPS request survives an overlapping initial/external poll and executes its requested command. The framework remains responsible for BUSY rejection. A rejected duplicate must not leave pending ownership behind. Completion reflects measured position and preserves the requested target. External motion, temperature compensation, errors, abort, disconnect and reconnect retain their existing contracts.

Use a deterministic bounded simulator reply hold and exact command assertions. Verify both exports and serial/UDP variants where configured. Hardware is unavailable and will not be tested. Public property names/counts remain unchanged.
