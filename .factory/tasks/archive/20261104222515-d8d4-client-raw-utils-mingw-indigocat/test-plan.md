# Test plan

- Build libindigo_client on Linux, check the symbols of the static and the shared library.
- Link a client application against it without any other copy of indigo_raw_utils.c and run it on a real frame.
- Relink the other clients of the library, to catch duplicate symbols.
- Dry run the MinGW Makefile and compile its new object rules with native gcc, as there is no MinGW compiler on this host.
- ./.factory/scripts/check, ./.factory/scripts/security and make -C indigo_test test-unit.
- MinGW and macOS builds on the owner's machines.
