# INDIGO client SDK for Windows

INDIGO client SDK for windows can be built with MSVC and GCC (mingw) compilers. It will poduce **indigo_client.dll** to be used in cutom projects.

## Building using MSVC
.\msvc folder contains 2 batch files 
- **build_x86_release.bat** - builds x86 version of indigo_client.dll and the corresponding link library.
- **build_x64_release.bat** - builds x64 version of indigo_client.dll and the corresponding link library.

Or you can open indigo_client.sln with MS Visual Studio and build it.

## Building using GCC (mingw)
.\gcc folder contains a Makefile that will build **libindigo_client.dll** and the corresponding link library. In addition it will build **indigo_prop_tool.exe**.

To build x86 or x64 version of the library the correct paths to the 32bit GCC and tools or 64bit GCC and tools shoud be set and execute **mingw32-make.exe**.
