#define MyAppName "qView"
#define MyAppPublisher "jurplel and qView contributors"
#define MyAppURL "https://interverse.tk/qview/"
#define MyAppExeName "qView.exe"

; Update these when building
#define MyAppVersion "1.0"
#define MyAppYear "2018"

[Setup]
AppId={{A6A9BAAB-C59E-4EAB-ACE1-3EEDE3031880}
AppName={#MyAppName}
AppPublisher={#MyAppPublisher}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={pf}\{#MyAppName}
LicenseFile=../../LICENSE
OutputBaseFilename={#MyAppName}-{#MyAppVersion}-win64
SetupIconFile=qView.ico
WizardSmallImageFile=wiz-small.bmp
WizardImageFile=wiz.bmp
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppVersion}
AppCopyright=Copyright © 2018-{#MyAppYear}, {#MyAppPublisher}
MinVersion=0,6.1
DisableProgramGroupPage=yes
ChangesAssociations=yes
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "startentry"; Description: "Create a start menu shortcut"; GroupDescription: "{cm:AdditionalIcons}";
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "qView64.exe"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Flags: ignoreversion
Source: "qView.VisualElementsManifest.xml"; DestDir: "{app}"; Flags: ignoreversion
Source: "win-tile-m.png"; DestDir: "{app}"; Flags: ignoreversion
Source: "win-tile-s.png"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: startentry
Name: "{commondesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Registry]
;  File Associations
Root: HKLM; Subkey: "Software\Classes\.bmp"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.cur"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.gif"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.icns"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.ico"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.jp2"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.jpeg"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.jpg"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.mng"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.pbm"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.pgm"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.png"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.ppm"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.svg"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.svgz"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.tif"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.tiff"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.wbmp"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.webp"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.xbm"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.xpm"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\{#MyAppName}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppName}"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\{#MyAppName}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[Code]
// Prompt if you want to remove user data when uninstalling
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    if MsgBox('Do you want to also delete all saved user data?',
      mbConfirmation, MB_YESNO) = IDYES
    then
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\qView');
      RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'Software\qView');
      RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'Software\WOW6432node\qView');
    end;
end;