; Inno Setup script for the Windows build of INDIGO (server, tools, agents
; Copyright (c) 2025 Rumen Bogdanovski
; All rights reserved.
;
; You can use this software under the terms of 'INDIGO Astronomy
; open-source license'
; (see https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md).
;
; Inno Setup script for the Windows build of INDIGO (server, tools, agents
; and drivers).
;
; The binaries are built by the Visual Studio solution "indigo_windows.sln"
; and land in "..\..\build\<Configuration>\<Platform>" (e.g.
; "..\..\build\Release\x64"). This script packages the whole output folder
; (indigo_server.exe, indigo_prop_tool.exe, indigo.dll, all
; indigo_*_*.dll drivers/agents and their bundled third-party DLLs) into a
; single Windows installer.
;
; Build with: iscc indigo_windows\installer\indigo.iss   (from an Inno Setup
; 6 install), or use tools\build_windows_installer.sh which builds the
; solution first and then invokes ISCC.
;
; This script expects the build output to already exist in
; ..\..\build\{#BuildConfiguration}\{#BuildPlatform} (run the Visual Studio
; build first, or let build_windows_installer.sh do it for you).

#define MyAppName "INDIGO"
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
AppId={{9F3E5B8B-9B0B-4E9D-8B90-0B7C3F9E5C21}
AppName={#MyAppName}
AppVersion={#MyAppVersion}-{#MyAppPackageBuild}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\INDIGO
DefaultGroupName=INDIGO
DisableProgramGroupPage=yes
; Let the user choose a per-machine (admin) or per-user install; this also
; determines whether we edit the system-wide or the per-user PATH below.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..\dist
OutputBaseFilename=indigo-windows-{#MyAppVersion}-{#MyAppPackageBuild}-{#BuildPlatform}-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
WizardImageFile=logo.bmp
WizardSmallImageFile=appicon.bmp
UninstallDisplayIcon={app}\bin\indigo_server.exe

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "modifypath"; Description: "Add the INDIGO ""bin"" folder to my PATH environment variable"; Flags: checkedonce

[Files]
; Everything produced by the Visual Studio build: indigo_server, tools,
; indigo.dll, all driver/agent DLLs and any bundled third-party DLLs
; (camera/mount/focuser vendor SDKs etc.).
Source: "{#BuildDir}\*.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "{#BuildDir}\*.dll"; DestDir: "{app}\bin"; Flags: ignoreversion

[Code]
const
  IND_WM_SETTINGCHANGE = $001A;
  IND_HWND_BROADCAST = $FFFF;
  IND_SMTO_ABORTIFHUNG = $0002;
  EnvKeyHKLM = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';
  EnvKeyHKCU = 'Environment';

function SendMessageTimeoutA(hWnd: LongInt; Msg: LongInt; wParam: LongInt;
  lParam: AnsiString; fuFlags: LongInt; uTimeout: LongInt; var lpdwResult: LongInt): LongInt;
  external 'SendMessageTimeoutA@user32.dll stdcall';

function EnvRootKey(): Integer;
begin
  if IsAdminInstallMode then
    Result := HKEY_LOCAL_MACHINE
  else
    Result := HKEY_CURRENT_USER;
end;

function EnvSubKey(): String;
begin
  if IsAdminInstallMode then
    Result := EnvKeyHKLM
  else
    Result := EnvKeyHKCU;
end;

procedure BroadcastEnvironmentChange();
var
  Res: LongInt;
begin
  SendMessageTimeoutA(IND_HWND_BROADCAST, IND_WM_SETTINGCHANGE, 0, 'Environment',
    IND_SMTO_ABORTIFHUNG, 5000, Res);
end;

function PathContainsDir(Path, Dir: string): Boolean;
var
  P: Integer;
  Entry, Remaining: string;
begin
  Result := False;
  Remaining := Path;
  while Length(Remaining) > 0 do
  begin
    P := Pos(';', Remaining);
    if P = 0 then
    begin
      Entry := Remaining;
      Remaining := '';
    end
    else
    begin
      Entry := Copy(Remaining, 1, P - 1);
      Remaining := Copy(Remaining, P + 1, Length(Remaining));
    end;
    if CompareText(Trim(Entry), Dir) = 0 then
    begin
      Result := True;
      exit;
    end;
  end;
end;

procedure AddDirToPath(Dir: string);
var
  Path: string;
begin
  if not RegQueryStringValue(EnvRootKey(), EnvSubKey(), 'Path', Path) then
    Path := '';

  if PathContainsDir(Path, Dir) then
    exit;

  if (Path <> '') and (Path[Length(Path)] <> ';') then
    Path := Path + ';' + Dir
  else
    Path := Path + Dir;

  if RegWriteStringValue(EnvRootKey(), EnvSubKey(), 'Path', Path) then
    BroadcastEnvironmentChange();
end;

procedure RemoveDirFromPath(Dir: string);
var
  Path, NewPath, Entry, Remaining: string;
  P: Integer;
begin
  if not RegQueryStringValue(EnvRootKey(), EnvSubKey(), 'Path', Path) then
    exit;

  NewPath := '';
  Remaining := Path;
  while Length(Remaining) > 0 do
  begin
    P := Pos(';', Remaining);
    if P = 0 then
    begin
      Entry := Remaining;
      Remaining := '';
    end
    else
    begin
      Entry := Copy(Remaining, 1, P - 1);
      Remaining := Copy(Remaining, P + 1, Length(Remaining));
    end;

    if (Trim(Entry) <> '') and (CompareText(Trim(Entry), Dir) <> 0) then
    begin
      if NewPath = '' then
        NewPath := Entry
      else
        NewPath := NewPath + ';' + Entry;
    end;
  end;

  if RegWriteStringValue(EnvRootKey(), EnvSubKey(), 'Path', NewPath) then
    BroadcastEnvironmentChange();
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    if WizardIsTaskSelected('modifypath') then
      AddDirToPath(ExpandConstant('{app}\bin'));
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RemoveDirFromPath(ExpandConstant('{app}\bin'));
end;
