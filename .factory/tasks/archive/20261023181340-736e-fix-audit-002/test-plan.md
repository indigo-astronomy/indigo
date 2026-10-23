# Test plan

AC-1: Test host lengths 0, size-1, size, size+1 and a large URL in both APIs, with and without explicit port. Resolver captures exact accepted host/port; rejected inputs never reach it.
AC-2: Table of invalid ports and authority syntax; valid ports 1 and 65535, including leading zeroes.
AC-3: tcp/udp and custom scheme hints, no scheme, null URL and null legacy protocol pointer; valid and invalid default ports. No external network/hardware.
Compile both production C sources into the test, replacing only getaddrinfo with a deterministic failing stub. Run normal and ASan/UBSan builds on macOS. Windows portability reviewed statically; Linux execution unavailable.
