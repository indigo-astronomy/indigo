# INDIGO Client SDK for Windows

## Building your application with Microsoft Visual C (MSVC)
Using the INDIGO Client SDK with MSVC requires several additional headers related to the threads library in addition to the INDIGO Client headers. The compiler must be able to see pthreads*.h headers provided in this bundle. Also the preprocesor definition INDIGO_WINDOWS must be set for the project.

### Configuring your MSVC project
Open 'Properties' of your project and add the folloing configurations:
1. Add "&lt;SDK_DIR&gt;\include\indigo" and "&lt;SDK_DIR&gt;\include\pthread" paths as 'C/C++ -> General -> Additional Include Directories' in your project.
2. Add "&lt;SDK_DIR&gt;\lib" in 'Linker -> General -> Additional Library Directories'
3. Add "INDIGO_WINDOWS" to the preprocessor in 'C/C++ -> Preprocessor -> Preprocessor Definitions'
4. Add "libindigo_client.lib" as an additional dependency in 'Linker-> Input -> Additional Dependencies'
5. To be able to run and debug your application in Visual Studio you need to add "&lt;SDK_DIR&gt;\lib" to your PATH. Add "PATH=&lt;SDK_DIR&gt;\lib;%PATH%" in 'Configuration Properties -> Debugging -> Environment'

**Note!** &lt;SDK_DIR&gt; is a placeholder for the INDIGO Client SDK base directory, &lt;SDK_DIR&gt; must be replaced with the real path.

## Building your application with GCC (MinGW)
In order to build your application with MinGW you need to add "&lt;SDK_DIR&gt;\include\indigo" as include path (-I), add "&lt;SDK_DIR&gt;\lib" as a library location (-L), define "INDIGO_WINDOWS" (-D) and link against "libindigo_client.lib". Here &lt;SDK_DIR&gt; is a placeholder for the base directory of INDIGO Client SDK.

The command line to compile your app named indigo_app should look something like this:
```
gcc -o indigo_app indigo_app.c -DINDIGO_WINDOWS -I.\include\indigo -L.\lib -lindigo_client.lib
```

## Runing and distributing your Application
Your application depends on **libindigo_client.dll** and **libwinpthread-1.dll** dynamic libraries. This is why they need to be in your PATH or in the same folder as your application in order to be able to run it.

Distribute your Application bundled with **libindigo_client.dll** and **libwinpthread-1.dll** along with the other dependencies.
