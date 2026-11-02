# Specification

AC-1: Reject quad star references outside the supplied stars storage, one-past references and pointers between element boundaries without subtracting unrelated pointers. Rejection frees partial catalog and leaves it empty.
AC-2: Valid references across all element positions preserve catalog indexes and index serialization.
AC-3: Existing index roundtrip/mapping controls pass with optimization and ASan/UBSan.

Baseline b6d3ee192, handwritten solver helper and agent, no .driver. Current implementation allocates a catalog and subtracts quad star pointers before range check; existing independent stack-object assertion is layout-dependent under sanitizers. No public property change, hardware transport, synchronization or driver lifecycle change. Native library builds macOS/Linux/Windows; host is macOS arm64. Hardware testing: none. Medium complexity, medium functional UB boundary risk.
