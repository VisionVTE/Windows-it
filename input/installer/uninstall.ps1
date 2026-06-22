# PowerShell Driver Uninstallation Script for Apple A18 Pro Input Driver
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
Write-Status "Apple A18 Pro Input Driver Uninstallation" "Cyan"
Write-Status "==================================================" "Cyan"

try {
    $targetDrivers = Get-WindowsDriver -Online -All | Where-Object { $_.OriginalFileName -match "AppleA18ProInput\.inf" }
    if ($null -ne $targetDrivers -and $targetDrivers.Count -gt 0) {
        foreach ($driver in $targetDrivers) {
            $oemName = $driver.Driver
            Write-Status "Uninstalling driver package: $oemName..." "Yellow"
            pnputil.exe /delete-driver $oemName /uninstall | Out-Host
        }
    } else {
        Write-Status "No matching input driver found in the Windows Driver Store." "Yellow"
    }
} catch {
    Write-Status "An error occurred during driver package removal: $_" "Red"
}

$otherDrivers = Get-WindowsDriver -Online -All | Where-Object { $_.OriginalFileName -match "AppleA18Pro" }
if ($null -eq $otherDrivers -or $otherDrivers.Count -eq 0) {
    Write-Status "Cleaning up self-signed driver certificates..." "Cyan"
    $certSubject = "CN=AppleA18ProDriver"
    $certStores = @(
        "Cert:\LocalMachine\My",
        "Cert:\LocalMachine\Root",
        "Cert:\LocalMachine\TrustedPublisher"
    )
    foreach ($storePath in $certStores) {
        if (Test-Path $storePath) {
            Get-ChildItem -Path $storePath | Where-Object { $_.Subject -eq $certSubject } | Remove-Item -Force
        }
    }
} else {
    Write-Status "Other suite drivers are still registered. Preserving shared code-signing certificate." "Green"
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$certFile = Join-Path $scriptDir "AppleA18ProDriver.cer"
if (Test-Path $certFile) { Remove-Item $certFile -Force | Out-Null }
$stageDir = Join-Path $scriptDir "Staging"
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force | Out-Null }

Write-Status "==================================================" "Cyan"
Write-Status "Input uninstallation complete!" "Green"
Write-Status "==================================================" "Cyan"
