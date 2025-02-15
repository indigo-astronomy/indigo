# INDIGO client SDK for Windows

INDIGO client SDK for windows can be built GCC (mingw) compilers. It will poduce **indigo_client.dll** to be used in cutom projects.

## Building using GCC (mingw)
.\gcc folder contains a Makefile that will build **libindigo_client.dll** and the corresponding link library. In addition it will build **indigo_prop_tool.exe**.

To build x86 or x64 version of the library the correct paths to the 32bit GCC and tools or 64bit GCC and tools shoud be set and execute **mingw32-make.exe**.

For building with Visual studio use indigo_windows.sln in top level directory.
