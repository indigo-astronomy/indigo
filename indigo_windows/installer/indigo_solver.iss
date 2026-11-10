; Copyright (c) 2026 Rumen Bogdanovski
; All rights reserved.
;
; You can use this software under the terms of 'INDIGO Astronomy
; open-source license'
; (see https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md).
;
; Inno Setup script of the stand-alone INDIGO Solver: the indigo_solver
; command line plate solver without the INDIGO server, drivers and libraries.
;
; indigo_solver.exe is built by the Visual Studio solution "indigo_windows.sln"
; into "..\..\build\<Configuration>\<Platform>" (e.g. "..\..\build\Release\x64")
; with the INDIGO sources it needs compiled in, so it needs only the
; third-party DLLs installed next to it. The quad indexes are not part of the
; installation, they are downloaded with "indigo_solver --download".
;
; Build with: iscc indigo_windows\installer\indigo_solver.iss   (from an Inno
; Setup 6 install), or use tools\build_windows_installer.sh which builds the
; solution first and then both installers.

#define MyAppName "INDIGO Solver"
#define MyAppVersion GetEnv("INDIGO_VERSION")
#if MyAppVersion == ""
  #define MyAppVersion "3.0"
#endif
#define MyAppBuild GetEnv("INDIGO_BUILD")
#if MyAppBuild == ""
  #define MyAppBuild "7"
#endif
#define MyAppPrerelease GetEnv("INDIGO_PRERELEASE")
#if MyAppPrerelease == ""
  #define MyAppPackageBuild MyAppBuild
#else
  #define MyAppPackageBuild MyAppBuild + "~" + MyAppPrerelease
#endif
#define MyAppPublisher "Rumen Bogdanovski"
#define MyAppURL "https://www.indigo-astronomy.org"

#define BuildPlatform GetEnv("INDIGO_BUILD_PLATFORM")
#if BuildPlatform == ""
  #define BuildPlatform "x64"
#endif
#define BuildConfiguration GetEnv("INDIGO_BUILD_CONFIGURATION")
#if BuildConfiguration == ""
  #define BuildConfiguration "Release"
#endif

#define BuildDir "..\..\build\" + BuildConfiguration + "\" + BuildPlatform

[Setup]
; another application than INDIGO, both can be installed side by side
AppId={{CB57093D-4D51-4A37-A7E3-25DDAEF0411E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}-{#MyAppPackageBuild}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\INDIGO Solver
DefaultGroupName=INDIGO Solver
DisableProgramGroupPage=yes
; Let the user choose a per-machine (admin) or per-user install; this also
; determines whether we edit the system-wide or the per-user PATH below.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..\dist
OutputBaseFilename=indigo-solver-windows-{#MyAppVersion}-{#MyAppPackageBuild}-{#BuildPlatform}-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=indigo.ico
WizardImageFile=logo.bmp
WizardSmallImageFile=appicon.bmp
UninstallDisplayIcon={app}\indigo.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "modifypath"; Description: "Add the INDIGO Solver ""bin"" folder to my PATH environment variable"; Flags: checkedonce

; {app}\bin holds indigo_solver.exe and the third-party DLLs it imports,
; Windows looks for these next to the exe file
[Files]
Source: "indigo.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\indigo_solver.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#BuildDir}\pthreadVC3.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#BuildDir}\zlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#BuildDir}\libraw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion

#include "path.iss"
