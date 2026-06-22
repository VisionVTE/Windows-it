# PowerShell Driver Installation Script for Apple A18 Pro Chipset
# Requires Administrator privileges

$ErrorActionPreference = "Stop"

function Write-Status ($message, $color = "Green") {
    Write-Host "[Apple A18 Pro Chipset] $message" -ForegroundColor $color
}

# 1. Elevate to Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Status "Script is not running as Administrator. Attempting to elevate..." "Yellow"
    Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    Exit
}

Write-Status "==================================================" "Cyan"
Write-Status "Apple A18 Pro Chipset Driver Installation" "Cyan"
Write-Status "==================================================" "Cyan"

# 2. Check and Enable Test Signing
Write-Status "Checking test signing configuration..." "Cyan"
$bcdedit = bcdedit | Out-String
$rebootRequired = $false

if ($bcdedit -notmatch "testsigning\s+Yes") {
    Write-Status "Test signing is NOT enabled. Enabling test signing..." "Yellow"
    try {
        bcdedit /set testsigning on | Out-Null
        Write-Status "Test signing has been enabled successfully." "Green"
        $rebootRequired = $true
    } catch {
        Write-Status "Failed to enable test signing. Please run 'bcdedit /set testsigning on' manually in an elevated command prompt." "Red"
        Exit 1
    }
} else {
    Write-Status "Test signing is already enabled." "Green"
}

# 3. Locate Driver Files
Write-Status "Locating driver files..." "Cyan"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$infSource = $null
$sysSource = $null

$searchPaths = @(
    $scriptDir,
    (Join-Path $scriptDir ".."),
    (Join-Path $scriptDir "..\inf"),
    (Join-Path $scriptDir "..\x64\Release"),
    (Join-Path $scriptDir "..\ARM64\Release")
)

foreach ($path in $searchPaths) {
    if (Test-Path (Join-Path $path "AppleA18ProChipset.inf")) {
        $infSource = Join-Path $path "AppleA18ProChipset.inf"
    }
    if (Test-Path (Join-Path $path "AppleA18ProChipset.sys")) {
        $sysSource = Join-Path $path "AppleA18ProChipset.sys"
    }
}

if (-not $infSource -or -not $sysSource) {
    Write-Status "Could not find driver source files." "Red"
    Write-Status "Required: 'AppleA18ProChipset.inf' and 'AppleA18ProChipset.sys'" "Red"
    Write-Status "Please compile the project in Visual Studio first." "Yellow"
    Exit 1
}

Write-Status "Found INF: $infSource" "Green"
Write-Status "Found SYS: $sysSource" "Green"

# Create flat staging folder
$stageDir = Join-Path $scriptDir "Staging"
if (Test-Path $stageDir) {
    Remove-Item $stageDir -Recurse -Force | Out-Null
}
New-Item -ItemType Directory -Path $stageDir | Out-Null

Copy-Item $infSource -Destination (Join-Path $stageDir "AppleA18ProChipset.inf") -Force
Copy-Item $sysSource -Destination (Join-Path $stageDir "AppleA18ProChipset.sys") -Force

# 4. Generate Catalog File (.cat)
Write-Status "Locating WDK tools for catalog generation..." "Cyan"
$inf2catPath = $null
$kitsPaths = @(
    "C:\Program Files (x86)\Windows Kits\10\bin"
)

foreach ($kitsPath in $kitsPaths) {
    if (Test-Path $kitsPath) {
        $files = Get-ChildItem -Path $kitsPath -Filter "inf2cat.exe" -Recurse -ErrorAction SilentlyContinue
        if ($files) {
            $x64Inf2cat = $files | Where-Object { $_.FullName -match "x64" }
            if ($x64Inf2cat) {
                $inf2catPath = $x64Inf2cat[0].FullName
            } else {
                $inf2catPath = $files[0].FullName
            }
            break
        }
    }
}

if ($null -eq $inf2catPath) {
    $inf2catPath = Get-Command inf2cat.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}

if ($null -ne $inf2catPath) {
    Write-Status "Found Inf2Cat at: $inf2catPath" "Green"
    Write-Status "Generating catalog file..." "Cyan"
    try {
        & $inf2catPath /driver:$stageDir /os:10_x64,10_arm64 | Out-Null
        Write-Status "Catalog file AppleA18ProChipset.cat generated successfully." "Green"
    } catch {
        Write-Status "Failed to run Inf2Cat. Error: $_" "Red"
        Exit 1
    }
} else {
    Write-Status "WDK/Inf2Cat not found. Attempting to locate pre-built catalog file..." "Yellow"
    $catSource = $null
    foreach ($path in $searchPaths) {
        if (Test-Path (Join-Path $path "AppleA18ProChipset.cat")) {
            $catSource = Join-Path $path "AppleA18ProChipset.cat"
            break
        }
    }
    if ($null -ne $catSource) {
        Copy-Item $catSource -Destination (Join-Path $stageDir "AppleA18ProChipset.cat") -Force
        Write-Status "Copied pre-built catalog file: $catSource" "Green"
    } else {
        Write-Status "No catalog file found. Driver installation will fail on Windows 10/11." "Red"
        Write-Status "Please install WDK to compile the driver catalog." "Red"
        Exit 1
    }
}

# 5. Create Self-Signed Driver Certificate
Write-Status "Managing code signing certificates..." "Cyan"
$certSubject = "CN=AppleA18ProDriver" # Shared certificate name for the driver suite
$cert = Get-ChildItem -Path "Cert:\LocalMachine\My" | Where-Object { $_.Subject -eq $certSubject }

if ($null -eq $cert) {
    Write-Status "No existing driver certificate found. Generating a new self-signed certificate..." "Yellow"
    try {
        $cert = New-SelfSignedCertificate -Subject $certSubject `
                                           -CertStoreLocation "Cert:\LocalMachine\My" `
                                           -Type CodeSigning `
                                           -KeyUsage DigitalSignature `
                                           -FriendlyName "Apple A18 Pro Graphics/Chipset Test Certificate"
        Write-Status "Certificate generated successfully." "Green"
    } catch {
        Write-Status "Failed to generate self-signed certificate. Error: $_" "Red"
        Exit 1
    }
} else {
    Write-Status "Found existing certificate: $($cert.Thumbprint)" "Green"
    if ($cert.Count -gt 1) { $cert = $cert[0] }
}

$certFile = Join-Path $scriptDir "AppleA18ProDriver.cer"
try {
    Export-Certificate -Cert $cert -FilePath $certFile -Type CERT | Out-Null
    Write-Status "Exported certificate to $certFile" "Green"
} catch {
    Write-Status "Failed to export certificate. Error: $_" "Red"
    Exit 1
}

try {
    Import-Certificate -FilePath $certFile -CertStoreLocation "Cert:\LocalMachine\Root" | Out-Null
    Import-Certificate -FilePath $certFile -CertStoreLocation "Cert:\LocalMachine\TrustedPublisher" | Out-Null
    Write-Status "Certificate successfully trusted on this system." "Green"
} catch {
    Write-Status "Failed to configure trust stores. Error: $_" "Red"
    Exit 1
}

# 6. Sign Driver Files
Write-Status "Signing driver binaries..." "Cyan"
try {
    $sysPath = Join-Path $stageDir "AppleA18ProChipset.sys"
    $catPath = Join-Path $stageDir "AppleA18ProChipset.cat"
    
    Set-AuthenticodeSignature -FilePath $sysPath -Certificate $cert | Out-Null
    Set-AuthenticodeSignature -FilePath $catPath -Certificate $cert | Out-Null
    
    Write-Status "Driver files signed successfully." "Green"
} catch {
    Write-Status "Failed to sign driver files. Error: $_" "Red"
    Exit 1
}

# 7. Install Driver using pnputil
Write-Status "Registering and installing chipset driver..." "Cyan"
$infPath = Join-Path $stageDir "AppleA18ProChipset.inf"
try {
    $pnpOutput = pnputil.exe /add-driver $infPath /install
    $pnpOutput | Out-String | Write-Host
    Write-Status "Driver installation command executed." "Green"
} catch {
    Write-Status "Failed to install driver package. Error: $_" "Red"
    Exit 1
}

Write-Status "==================================================" "Cyan"
Write-Status "Chipset installation complete!" "Green"

if ($rebootRequired) {
    Write-Status "IMPORTANT: Test signing mode was just enabled. Reboot is required." "Yellow"
} else {
    Write-Status "The driver is ready for use." "Green"
}
Write-Status "==================================================" "Cyan"
