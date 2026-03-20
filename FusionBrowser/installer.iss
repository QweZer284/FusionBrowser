; ════════════════════════════════════════════════════════════════════
;  Fusion Browser v9.0  —  Inno Setup 6 Installer Script
;  Build with: ISCC installer.iss
; ════════════════════════════════════════════════════════════════════

#define MyAppName      "Fusion Browser"
#define MyAppVersion   "9.0"
#define MyAppPublisher "Fusion Browser"
#define MyAppExeName   "Fusion.exe"
#define MyAppURL       "https://github.com/fusion-browser"
#define MySourceDir    "dist\Fusion"

[Setup]
; Unique GUID — do not change after first release
AppId={{A7C3B2D1-E4F5-4A6B-9C0D-1E2F3A4B5C6D}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\Fusion
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
; No admin required — installs to user's Program Files
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=installer_output
OutputBaseFilename=Fusion_Setup_v{#MyAppVersion}
SetupIconFile=resources\fusion.ico
Compression=lzma2/ultra64
SolidCompression=yes
InternalCompressLevel=ultra64
WizardStyle=modern
WizardResizable=yes
DisableWelcomePage=no
DisableDirPage=no
DisableProgramGroupPage=no
; Modern-looking wizard
WizardImageFile=compiler:WizModernImage.bmp
WizardSmallImageFile=compiler:WizModernSmallImage.bmp
; High DPI
DPIAware=yes
; Uninstall
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName} {#MyAppVersion}
; Minimum Windows version: Windows 10
MinVersion=10.0

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon";    Description: "{cm:CreateDesktopIcon}";    GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon";Description: "{cm:CreateQuickLaunchIcon}";GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 6.1; Check: not IsAdminInstallMode

[Files]
; Main application folder — all files recursively
Source: "{#MySourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start menu
Name: "{group}\{#MyAppName}";            Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\fusion.ico"
Name: "{group}\Удалить {#MyAppName}";    Filename: "{uninstallexe}"
; Desktop (optional)
Name: "{autodesktop}\{#MyAppName}";      Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\fusion.ico"; Tasks: desktopicon
; Quick launch
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
; Offer to launch after install
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
; Clean up user data on uninstall (optional, commented out by default)
; Filename: "{cmd}"; Parameters: "/c rmdir /s /q ""{userprofile}\.fusion9_cache"""; Flags: runhidden

[Registry]
; Register as browser (optional — adds to "Open With" list)
Root: HKCU; Subkey: "Software\Clients\StartMenuInternet\FusionBrowser"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Clients\StartMenuInternet\FusionBrowser\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#MyAppName}"
Root: HKCU; Subkey: "Software\Clients\StartMenuInternet\FusionBrowser\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "Fusion Browser — быстрый браузер на Chromium"
Root: HKCU; Subkey: "Software\Clients\StartMenuInternet\FusionBrowser\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKCU; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "FusionBrowser"; ValueData: "Software\Clients\StartMenuInternet\FusionBrowser\Capabilities"; Flags: uninsdeletevalue

[Messages]
; Russian overrides
russian.BeveledLabel=Fusion Browser v{#MyAppVersion}

[Code]
// Close Fusion if running before install/uninstall
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;
  while FindWindowByClassName('Qt661QWindowIcon') <> 0 do begin
    if MsgBox('Fusion Browser запущен. Закройте его перед установкой.', mbConfirmation, MB_OKCANCEL) = IDCANCEL then begin
      Result := False;
      Exit;
    end;
  end;
end;

function InitializeUninstall(): Boolean;
begin
  Result := True;
  if FindWindowByClassName('Qt661QWindowIcon') <> 0 then begin
    MsgBox('Пожалуйста, закройте Fusion Browser перед удалением.', mbError, MB_OK);
    Result := False;
  end;
end;
