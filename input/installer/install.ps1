# PowerShell Driver Installation Script for Apple A18 Pro Input Driver
# Requires Administrator privileges

$ErrorActionPreference = "Stop"

function Write-Status ($message, $color = "Green") {
    Write-Host "[Apple A18 Pro Input] $message" -ForegroundColor $color
}

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Status "Script is not running as Administrator. Attempting to elevate..." "Yellow"
    Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    Exit
}

Write-Status "==================================================" "Cyan"
Write-Status "Apple A18 Pro Input Driver Installation" "Cyan"
Write-Status "==================================================" "Cyan"

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
        Write-Status "Failed to enable test signing." "Red"
        Exit 1
    }
} else {
    Write-Status "Test signing is already enabled." "Green"
}

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
    if (Test-Path (Join-Path $path "AppleA18ProInput.inf")) {
        $infSource = Join-Path $path "AppleA18ProInput.inf"
    }
    if (Test-Path (Join-Path $path "AppleA18ProInput.sys")) {
        $sysSource = Join-Path $path "AppleA18ProInput.sys"
    }
}

if (-not $infSource -or -not $sysSource) {
    Write-Status "Could not find driver source files." "Red"
    Write-Status "Required: 'AppleA18ProInput.inf' and 'AppleA18ProInput.sys'" "Red"
    Exit 1
}

Write-Status "Found INF: $infSource" "Green"
Write-Status "Found SYS: $sysSource" "Green"

$stageDir = Join-Path $scriptDir "Staging"
if (Test-Path $stageDir) {
    Remove-Item $stageDir -Recurse -Force | Out-Null
}
New-Item -ItemType Directory -Path $stageDir | Out-Null

Copy-Item $infSource -Destination (Join-Path $stageDir "AppleA18ProInput.inf") -Force
Copy-Item $sysSource -Destination (Join-Path $stageDir "AppleA18ProInput.sys") -Force

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
        Write-Status "Catalog file AppleA18ProInput.cat generated successfully." "Green"
    } catch {
        Write-Status "Failed to run Inf2Cat. Error: $_" "Red"
        Exit 1
    }
} else {
    Write-Status "WDK/Inf2Cat not found. Attempting to locate pre-built catalog file..." "Yellow"
    $catSource = $null
    foreach ($path in $searchPaths) {
        if (Test-Path (Join-Path $path "AppleA18ProInput.cat")) {
            $catSource = Join-Path $path "AppleA18ProInput.cat"
            break
        }
    }
    if ($null -ne $catSource) {
        Copy-Item $catSource -Destination (Join-Path $stageDir "AppleA18ProInput.cat") -Force
        Write-Status "Copied pre-built catalog file: $catSource" "Green"
    } else {
        Write-Status "No catalog file found. Driver installation will fail." "Red"
        Exit 1
    }
}

Write-Status "Managing code signing certificates..." "Cyan"
$certSubject = "CN=AppleA18ProDriver"
$cert = Get-ChildItem -Path "Cert:\LocalMachine\My" | Where-Object { $_.Subject -eq $certSubject }

if ($null -eq $cert) {
    Write-Status "No existing driver certificate found. Generating a new self-signed certificate..." "Yellow"
    try {
        $cert = New-SelfSignedCertificate -Subject $certSubject `
                                           -CertStoreLocation "Cert:\LocalMachine\My" `
                                           -Type CodeSigning `
                                           -KeyUsage DigitalSignature `
                                           -FriendlyName "Apple A18 Pro Driver Suite Test Certificate"
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
} catch {
    Write-Status "Failed to export certificate." "Red"
    Exit 1
}

try {
    Import-Certificate -FilePath $certFile -CertStoreLocation "Cert:\LocalMachine\Root" | Out-Null
    Import-Certificate -FilePath $certFile -CertStoreLocation "Cert:\LocalMachine\TrustedPublisher" | Out-Null
    Write-Status "Certificate successfully trusted on this system." "Green"
} catch {
    Write-Status "Failed to configure trust stores." "Red"
    Exit 1
}

Write-Status "Signing driver binaries..." "Cyan"
try {
    $sysPath = Join-Path $stageDir "AppleA18ProInput.sys"
    $catPath = Join-Path $stageDir "AppleA18ProInput.cat"
    
    Set-AuthenticodeSignature -FilePath $sysPath -Certificate $cert | Out-Null
    Set-AuthenticodeSignature -FilePath $catPath -Certificate $cert | Out-Null
    
    Write-Status "Driver files signed successfully." "Green"
} catch {
    Write-Status "Failed to sign driver files." "Red"
    Exit 1
}

Write-Status "Registering and installing input driver..." "Cyan"
$infPath = Join-Path $stageDir "AppleA18ProInput.inf"
try {
    $pnpOutput = pnputil.exe /add-driver $infPath /install
    $pnpOutput | Out-String | Write-Host
} catch {
    Write-Status "Failed to install driver package." "Red"
    Exit 1
}

Write-Status "==================================================" "Cyan"
Write-Status "Input installation complete!" "Green"
if ($rebootRequired) { Write-Status "Reboot is required." "Yellow" }
Write-Status "==================================================" "Cyan"
