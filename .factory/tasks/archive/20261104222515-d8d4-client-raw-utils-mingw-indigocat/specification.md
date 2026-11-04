# Specification

- libindigo_client (Linux and macOS Makefile, MinGW Makefile) contains indigo_raw_utils.c. libindigo keeps it too: the drivers and agents link only libindigo, and an application links one or the other.
- indigo_windows/gcc/Makefile builds libindigocat.dll and libindigocat.lib from every indigo_libs/indigocat/*.c.
- The SDK bundle contains libindigocat.dll, libindigocat.lib, indigo/indigocat/*.h and indigo_raw_utils.h.
- Out of scope: the Visual Studio projects (indigo.vcxproj already contains both, indigo_client.vcxproj is unchanged) and the Xcode project, which has no libindigo_client target.
- No public API changes.
