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
; installation: a wizard page offers to download the selected ones at the end
; of the setup, and they can be downloaded at any time with
; "indigo_solver --download".
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

[Code]
var
  IndexPage: TInputOptionWizardPage;
  IndexScales: TArrayOfString;

procedure AddIndex(Scale, Description: String);
var
  Count: Integer;
begin
  Count := GetArrayLength(IndexScales);
  SetArrayLength(IndexScales, Count + 1);
  IndexScales[Count] := Scale;
  IndexPage.Add(Scale + ' - ' + Description);
end;

procedure InitializeWizard();
begin
  IndexPage := CreateInputOptionPage(wpSelectTasks, 'Plate Solving Indexes', 'Which indexes do you want to download now? This step is optional.', 'indigo_solver needs at least one index to solve an image. Select the indexes for the field of view of your equipment, they are downloaded at the end of the setup.' + #13#10#13#10 + 'You can skip this step and install or remove the indexes at any time with the commands "indigo_solver --download 1,2,5" (or "all"), "indigo_solver --remove 1,2,5" and list them with "indigo_solver --list".', False, False);
  // the scales, fields and sizes of indigo_solver_index_fovs, index_fields and indigo_solver_index_sizes in indigo_drivers/agent_platesolver/indigo_solver.c
  AddIndex('0.2', 'Gaia DR3, field of view 0.2 - 0.45 deg, 1216 MB');
  AddIndex('0.5', 'Gaia DR3, field of view 0.45 - 1 deg, 911 MB');
  AddIndex('1', 'Tycho-2, field of view 1 - 2 deg, 538 MB');
  AddIndex('2', 'Tycho-2, field of view 2 - 4.5 deg, 358 MB');
  AddIndex('5', 'Tycho-2, field of view 4.5 - 10 deg, 178 MB');
  AddIndex('10', 'Tycho-2, field of view 10 - 20 deg, 92 MB');
  AddIndex('20', 'Tycho-2, field of view over 20 deg, 36 MB');
end;

function SelectedIndexes(): String;
var
  I: Integer;
begin
  Result := '';
  for I := 0 to GetArrayLength(IndexScales) - 1 do
  begin
    if IndexPage.Values[I] then
    begin
      if Result <> '' then
        Result := Result + ',';
      Result := Result + IndexScales[I];
    end;
  end;
end;

// The indexes belong to the user who started the setup, %USERPROFILE%\.indigo\solver is also the directory of the
// Platesolver Agent, so the tool runs as that user even in an elevated setup. A failed download doesn't fail the setup.
<event('CurStepChanged')>
procedure DownloadIndexes(CurStep: TSetupStep);
var
  Scales: String;
  ResultCode: Integer;
begin
  if CurStep <> ssPostInstall then
    exit;
  Scales := SelectedIndexes();
  if Scales = '' then
    exit;
  WizardForm.StatusLabel.Caption := 'Downloading the selected indexes...';
  if not ExecAsOriginalUser(ExpandConstant('{app}\bin\indigo_solver.exe'), '--download ' + Scales, '', SW_SHOWNORMAL, ewWaitUntilTerminated, ResultCode) or (ResultCode <> 0) then
    SuppressibleMsgBox('INDIGO Solver is installed, but some of the indexes were not downloaded.' + #13#10#13#10 + 'You can repeat the download at any time with the command:' + #13#10 + 'indigo_solver --download ' + Scales, mbError, MB_OK, IDOK);
end;
