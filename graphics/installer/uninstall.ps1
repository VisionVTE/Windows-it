# PowerShell Driver Uninstallation Script for Apple A18 Pro GPU
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
Write-Status "Apple A18 Pro GPU Driver Uninstallation" "Cyan"
Write-Status "==================================================" "Cyan"

# 2. Uninstall Driver via Pnputil
Write-Status "Locating installed driver in Windows Driver Store..." "Cyan"
try {
    # Retrieve all online drivers and find one matching original filename
    $targetDrivers = Get-WindowsDriver -Online -All | Where-Object { $_.OriginalFileName -match "AppleA18Pro\.inf" }
    
    if ($null -ne $targetDrivers -and $targetDrivers.Count -gt 0) {
        foreach ($driver in $targetDrivers) {
            $oemName = $driver.Driver
            Write-Status "Uninstalling driver package: $oemName ($($driver.OriginalFileName))..." "Yellow"
            
            # Delete and uninstall driver
            $uninstallOutput = pnputil.exe /delete-driver $oemName /uninstall
            $uninstallOutput | Out-String | Write-Host
        }
        Write-Status "Driver package uninstallation complete." "Green"
    } else {
        Write-Status "No matching driver found in the Windows Driver Store." "Yellow"
        
        # Fallback method: Use devcon if installed, or direct pnputil scan
        Write-Status "Checking for active device node..." "Cyan"
        # Apple device vendor & product ID: VEN_106B&DEV_16B5
        $devNode = Get-PnpDevice | Where-Object { $_.InstanceId -like "*VEN_106B&DEV_16B5*" } -ErrorAction SilentlyContinue
        if ($devNode) {
            Write-Status "Found hardware device node: $($devNode.Name). Removing..." "Yellow"
            # Command-line fallback to disable/remove
            pnputil.exe /remove-device $devNode.InstanceId | Out-String | Write-Host
        }
    }
} catch {
    Write-Status "An error occurred during driver package removal: $_" "Red"
    # Don't halt the whole uninstall process if pnputil fails, proceed to clean certs
}

# 3. Clean up Certificates
Write-Status "Cleaning up self-signed driver certificates..." "Cyan"
$certSubject = "CN=AppleA18ProDriver"
$certStores = @(
    "Cert:\LocalMachine\My",
    "Cert:\LocalMachine\Root",
    "Cert:\LocalMachine\TrustedPublisher"
)

$deletedCertsCount = 0
foreach ($storePath in $certStores) {
    if (Test-Path $storePath) {
        $certs = Get-ChildItem -Path $storePath | Where-Object { $_.Subject -eq $certSubject }
        if ($certs) {
            foreach ($cert in $certs) {
                Write-Status "Removing certificate $($cert.Thumbprint) from $storePath" "Yellow"
                Remove-Item -Path $cert.PSPath -Force
                $deletedCertsCount++
            }
        }
    }
}

if ($deletedCertsCount -gt 0) {
    Write-Status "Removed $deletedCertsCount developer certificate(s) from trust stores." "Green"
} else {
    Write-Status "No developer certificates found to remove." "Green"
}

# 4. Clean up temporary installer files in the script directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$certFile = Join-Path $scriptDir "AppleA18ProDriver.cer"
if (Test-Path $certFile) {
    Remove-Item $certFile -Force | Out-Null
    Write-Status "Removed certificate file: $certFile" "Green"
}

$stageDir = Join-Path $scriptDir "Staging"
if (Test-Path $stageDir) {
    Remove-Item $stageDir -Recurse -Force | Out-Null
    Write-Status "Cleaned staging folder: $stageDir" "Green"
}

Write-Status "==================================================" "Cyan"
Write-Status "Uninstallation complete!" "Green"
Write-Status "==================================================" "Cyan"
