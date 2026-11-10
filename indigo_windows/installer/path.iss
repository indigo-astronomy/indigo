; Copyright (c) 2026 Rumen Bogdanovski
; All rights reserved.
;
; You can use this software under the terms of 'INDIGO Astronomy
; open-source license'
; (see https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md).
;
; The [Code] of the INDIGO installers (indigo.iss, indigo_solver.iss): with the
; "modifypath" task the "bin" folder of the installation is added to the PATH
; of the machine (admin install) or of the user, and removed on uninstall.
;
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
