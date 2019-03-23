#define MyAppName "qView"
#define MyAppPublisher "jurplel and qView contributors"
#define MyAppURL "https://intvhq.com/qview/"
#define MyAppExeName "qView.exe"

; Update these when building
#define MyAppVersion "2.0"
#define MyAppYear "2019"

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