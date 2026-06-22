# PowerShell Driver Installation Script for Apple A18 Pro GPU
# Requires Administrator privileges

$ErrorActionPreference = "Stop"

# Helper function to print colored status messages
function Write-Status ($message, $color = "Green") {
    Write-Host "[Apple A18 Pro] $message" -ForegroundColor $color
}

# 1. Elevate to Administrator if not already elevated
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Status "Script is not running as Administrator. Attempting to elevate..." "Yellow"
    Start-Process powershell -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs
    Exit
}

Write-Status "==================================================" "Cyan"
Write-Status "Apple A18 Pro GPU Driver Installation" "Cyan"
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

# Staging layout: Search current dir, parent dir, or build directories
$infSource = $null
$dllSource = $null

$searchPaths = @(
    $scriptDir,
    (Join-Path $scriptDir ".."),
    (Join-Path $scriptDir "..\inf"),
    (Join-Path $scriptDir "..\x64\Release"),
    (Join-Path $scriptDir "..\ARM64\Release")
)

foreach ($path in $searchPaths) {
    if (Test-Path (Join-Path $path "AppleA18Pro.inf")) {
        $infSource = Join-Path $path "AppleA18Pro.inf"
    }
    if (Test-Path (Join-Path $path "AppleA18ProDisplay.dll")) {
        $dllSource = Join-Path $path "AppleA18ProDisplay.dll"
    }
}

if (-not $infSource -or -not $dllSource) {
    Write-Status "Could not find driver source files." "Red"
    Write-Status "Required: 'AppleA18Pro.inf' and 'AppleA18ProDisplay.dll'" "Red"
    Write-Status "Please compile the project in Visual Studio first." "Yellow"
    Exit 1
}

Write-Status "Found INF: $infSource" "Green"
Write-Status "Found DLL: $dllSource" "Green"

# Create a flat staging folder for packaging/signing
$stageDir = Join-Path $scriptDir "Staging"
if (Test-Path $stageDir) {
    Remove-Item $stageDir -Recurse -Force | Out-Null
}
New-Item -ItemType Directory -Path $stageDir | Out-Null

Copy-Item $infSource -Destination (Join-Path $stageDir "AppleA18Pro.inf") -Force
Copy-Item $dllSource -Destination (Join-Path $stageDir "AppleA18ProDisplay.dll") -Force

# 4. Generate Catalog File (.cat)
Write-Status "Locating WDK tools for catalog generation..." "Cyan"
$inf2catPath = $null

# Common Windows Kits installation directories
$kitsPaths = @(
    "C:\Program Files (x86)\Windows Kits\10\bin"
)

foreach ($kitsPath in $kitsPaths) {
    if (Test-Path $kitsPath) {
        $files = Get-ChildItem -Path $kitsPath -Filter "inf2cat.exe" -Recurse -ErrorAction SilentlyContinue
        if ($files) {
            # Prefer the x64 version if available
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
    Write-Status "Inf2Cat.exe was not found on this system." "Yellow"
    Write-Status "Please make sure Windows Driver Kit (WDK) is installed." "Yellow"
    Write-Status "Will attempt to search in PATH..." "Yellow"
    $inf2catPath = Get-Command inf2cat.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}

if ($null -ne $inf2catPath) {
    Write-Status "Found Inf2Cat at: $inf2catPath" "Green"
    Write-Status "Generating catalog file..." "Cyan"
    
    # Run Inf2Cat
    try {
        & $inf2catPath /driver:$stageDir /os:10_x64,10_arm64 | Out-Null
        Write-Status "Catalog file AppleA18Pro.cat generated successfully." "Green"
    } catch {
        Write-Status "Failed to run Inf2Cat. Error: $_" "Red"
        Exit 1
    }
} else {
    Write-Status "WDK/Inf2Cat not found. If a pre-built catalog file exists, we will attempt to copy it." "Yellow"
    $catSource = $null
    foreach ($path in $searchPaths) {
        if (Test-Path (Join-Path $path "AppleA18Pro.cat")) {
            $catSource = Join-Path $path "AppleA18Pro.cat"
            break
        }
    }
    if ($null -ne $catSource) {
        Copy-Item $catSource -Destination (Join-Path $stageDir "AppleA18Pro.cat") -Force
        Write-Status "Copied pre-built catalog file: $catSource" "Green"
    } else {
        Write-Status "No catalog file could be generated or found. Driver installation will fail on Windows 10/11." "Red"
        Write-Status "Please install the WDK to run Inf2Cat." "Red"
        Exit 1
    }
}

# 5. Create Self-Signed Driver Certificate
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
                                           -FriendlyName "Apple A18 Pro Graphics Test Certificate"
        Write-Status "Certificate generated successfully." "Green"
    } catch {
        Write-Status "Failed to generate self-signed certificate. Error: $_" "Red"
        Exit 1
    }
} else {
    Write-Status "Found existing certificate: $($cert.Thumbprint)" "Green"
    # If multiple, grab the first one
    if ($cert.Count -gt 1) {
        $cert = $cert[0]
    }
}

# Export the certificate so we can import it into trusted stores
$certFile = Join-Path $scriptDir "AppleA18ProDriver.cer"
try {
    Export-Certificate -Cert $cert -FilePath $certFile -Type CERT | Out-Null
    Write-Status "Exported certificate to $certFile" "Green"
} catch {
    Write-Status "Failed to export certificate. Error: $_" "Red"
    Exit 1
}

# Import into Trusted Root Certification Authorities and Trusted Publishers
Write-Status "Adding certificate to Trusted Root and Trusted Publishers stores..." "Cyan"
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
    $dllPath = Join-Path $stageDir "AppleA18ProDisplay.dll"
    $catPath = Join-Path $stageDir "AppleA18Pro.cat"
    
    # Use native PowerShell cmdlet to sign (no signtool.exe dependency!)
    Set-AuthenticodeSignature -FilePath $dllPath -Certificate $cert | Out-Null
    Set-AuthenticodeSignature -FilePath $catPath -Certificate $cert | Out-Null
    
    Write-Status "Driver files signed successfully." "Green"
} catch {
    Write-Status "Failed to sign driver files. Error: $_" "Red"
    Exit 1
}

# 7. Install Driver using pnputil
Write-Status "Registering and installing driver..." "Cyan"
$infPath = Join-Path $stageDir "AppleA18Pro.inf"
try {
    # Run pnputil to install driver
    $pnpOutput = pnputil.exe /add-driver $infPath /install
    $pnpOutput | Out-String | Write-Host
    Write-Status "Driver installation command executed." "Green"
} catch {
    Write-Status "Failed to install driver package via pnputil. Error: $_" "Red"
    Exit 1
}

Write-Status "==================================================" "Cyan"
Write-Status "Installation phase complete!" "Green"

if ($rebootRequired) {
    Write-Status "IMPORTANT: Test signing mode was just enabled." "Yellow"
    Write-Status "You MUST reboot your system before the driver can load." "Yellow"
    
    $choices = [System.Management.Automation.Host.ChoiceDescription[]]@(
        New-Object System.Management.Automation.Host.ChoiceDescription "&Yes", "Reboot now"
        New-Object System.Management.Automation.Host.ChoiceDescription "&No", "Do not reboot now"
    )
    $decision = $Host.UI.PromptForChoice("Reboot System", "Would you like to reboot your computer now?", $choices, 1)
    if ($decision -eq 0) {
        Write-Status "Rebooting system..." "Yellow"
        Restart-Computer
    } else {
        Write-Status "Please reboot your system manually at your earliest convenience." "Yellow"
    }
} else {
    Write-Status "The driver is ready for use. No reboot is required." "Green"
}
Write-Status "==================================================" "Cyan"
