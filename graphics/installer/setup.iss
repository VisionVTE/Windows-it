; Inno Setup Installation Script for Apple A18 Pro GPU Display Driver
; This script compiles into a single executable installer wizard.

#define MyshortName "AppleA18ProDriver"
#define MyAppName "Apple A18 Pro GPU Driver"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Apple Inc. (Educational / Reference)"
#define MyAppExeName "AppleA18ProDriverInstaller.exe"

[Setup]
; Unique AppId (generated for this project)
AppId={{A18PRODF-E000-4000-8000-000000000000}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyshortName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\README.md
; Require Administrator privileges for installation
PrivilegesRequired=admin
OutputBaseFilename=AppleA18ProDriverSetup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64 arm64
; Support running on x64 and ARM64 Windows
ArchitecturesAllowed=x64 arm64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Driver binaries - try to pack both architectures if built, otherwise fail gracefully
Source: "..\x64\Release\AppleA18ProDisplay.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist; Check: IsX64
Source: "..\ARM64\Release\AppleA18ProDisplay.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist; Check: IsARM64

; Driver Installation INF File
Source: "..\inf\AppleA18Pro.inf"; DestDir: "{app}"; Flags: ignoreversion

; Support script installation
Source: "install.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "uninstall.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Run]
; Run the PowerShell installation script in hidden mode during post-installation
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\install.ps1"""; Flags: runhidden runascurrentuser; Description: "Registering driver and installing certificates..."; StatusMsg: "Installing Apple A18 Pro GPU Driver..."

[UninstallRun]
; Run the PowerShell uninstallation script in hidden mode prior to deleting files
Filename: "powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\uninstall.ps1"""; Flags: runhidden runascurrentuser

[Code]
// Helper function to check if the target operating system is x64 (64-bit Intel/AMD)
function IsX64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = alX64);
end;

// Helper function to check if the target operating system is ARM64
function IsARM64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = alARM64);
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    // Check if driver files exist in the app directory. 
    // If neither dll is found (e.g. they built under a different folder structure), 
    // warn the user that they need to copy AppleA18ProDisplay.dll to the install folder.
    if not FileExists(ExpandConstant('{app}\AppleA18ProDisplay.dll')) then
    begin
      MsgBox('Warning: AppleA18ProDisplay.dll was not packaged in the installer.' + #13#10 +
             'Please make sure to place the compiled DLL in the installation directory: ' + #13#10 +
             ExpandConstant('{app}') + ' and run install.bat as administrator.', mbWarning, MB_OK);
    end;
  end;
end;
