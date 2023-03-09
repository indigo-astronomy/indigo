# INDIGO Client SDK for Windows

## Building your application with Microsoft Visual C (MSVC)
Using the INDIGO Client SDK with MSVC requires several additional headers related to the threads library in addition to the INDIGO Client headers. The compiler must be able to see pthreads*.h headers provided in this bundle. Also the preprocesor definition INDIGO_WINDOWS must be set for the project.

### Configuring your MSVC project
Open 'Properties' of youir project and add the folloing configurations:
1. Add *"SDK_DIR\include\indigo"* and *"SDK_DIR\include\pthread"* paths as 'C/C++ -> General -> Additional Include Directories' in your project.
2. Add *"SDK_DIR\lib"* in 'Linker -> General -> Additional Library Directories'
3. Add *"INDIGO_WINDOWS"* to the preprocessor in 'C/C++ -> Preprocessor -> Preprocessor Definitions'
4. Add *"libindigo_client.lib"* as an additional dependency in 'Linker-> Input -> Additional Dependencies'

Here SDK_DIR is the base directory of INDIGO Client SDK.

## Building your application with GCC (MinGW)
In order to build your application with MinGW you need to add *"$SDK_DIR\include\indigo"* as include path (-I), add *"$SDK_DIR\lib"* as a library location (-L), define *"INDIGO_WINDOWS"* (-D) and link against *"libindigo_client.lib"*. Here $SDK_DIR is the base directory of INDIGO Client SDK.

The command line to compile your app named indigo_app should look something like this:
```
gcc -o indigo_app indigo_app.c -DINDIGO_WINDOWS -I.\include\indigo -L.\lib -lindigo_client.lib
```

## Runing and distributing your Application
Your application depends on **libindigo_client.dll** and **libwinpthread-1.dll** dynamic libraries and they need to be placed in the same folder as your application in order to be able to run it.

Distribute your Application bundled with **libindigo_client.dll** and **libwinpthread-1.dll** along with the other dependencies.
