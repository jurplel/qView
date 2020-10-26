#define MyAppName "qView"
#define MyAppPublisher "jurplel and qView contributors"
#define MyAppURL "https://interversehq.com/qview/"
#define MyAppExeName "qView.exe"

; Update these when building
#define MyAppVersion "4.0"
#define MyAppYear "2020"

[Setup]
AppId={{A6A9BAAB-C59E-4EAB-ACE1-3EEDE3031880}
AppName={#MyAppName}
AppPublisher={#MyAppPublisher}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf}\{#MyAppName}
LicenseFile=../../LICENSE
OutputBaseFilename={#MyAppName}-{#MyAppVersion}-win64
SetupIconFile=qView.ico
WizardSmallImageFile=wiz-small.bmp
WizardImageFile=wiz.bmp
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoVersion={#MyAppVersion}
AppCopyright=Copyright � 2018-{#MyAppYear}, {#MyAppPublisher}
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
Name: "fileassociation"; Description: "Create file associations"; GroupDescription: "Other:";

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
; Key that specifies exe path to file associations
Root: HKCR; Subkey: "{#MyAppName}.1"; Flags: uninsdeletekey; Tasks: fileassociation
Root: HKCR; Subkey: "{#MyAppName}.1\shell\open\command"; ValueType: string; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: fileassociation

; File associations that point to the above key
Root: HKCR; Subkey: ".bmp\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".cur\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".gif\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".icns\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".ico\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".jp2\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".jpeg\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".jpg\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".mng\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".pbm\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".pgm\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".png\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".ppm\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".svg\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".svgz\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".tif\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".tiff\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".wbmp\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".webp\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".xbm\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".xpm\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".apng\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".avif\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".avifs\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".heic\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".heics\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".heif\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation
Root: HKCR; Subkey: ".heifs\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppName}.1"; ValueData: ""; Flags: uninsdeletevalue; Tasks: fileassociation

; Capabilities keys for default programs/apps to work
Root: HKLM; Subkey: "Software\{#MyAppName}"; Flags: uninsdeletekey; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#MyAppName}"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "Practical and minimal image viewer"; Tasks: fileassociation

Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".bmp"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".cur"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".gif"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".icns"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ico"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jp2"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jpeg"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".jpg"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".mng"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".pbm"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".pgm"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".png"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".ppm"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".svg"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".svgz"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".tif"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".tiff"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".wbmp"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".webp"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".xbm"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".xpm"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".apng"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".avif"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".avifs"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".heic"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".heics"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".heif"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation
Root: HKLM; Subkey: "Software\{#MyAppName}\Capabilities\FileAssociations"; ValueType: string; ValueName: ".heifs"; ValueData: "{#MyAppName}.1"; Tasks: fileassociation

Root: HKLM; Subkey: "Software\RegisteredApplications"; ValueType: string; ValueName: "qView"; ValueData: "Software\qView\Capabilities"; Flags: uninsdeletevalue; Tasks: fileassociation

[Code]
// Prompt if you want to remove user data when uninstalling
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    if MsgBox('Do you want to also delete saved settings?',
      mbConfirmation, MB_YESNO) = IDYES
    then
      RegDeleteKeyIncludingSubkeys(HKEY_CURRENT_USER, 'Software\qView');
      RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'Software\qView');
      RegDeleteKeyIncludingSubkeys(HKEY_LOCAL_MACHINE, 'Software\WOW6432node\qView');
    end;
end;