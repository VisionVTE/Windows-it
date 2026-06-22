@echo off
:: Batch wrapper to execute install.ps1 with administrator privileges
:: and bypassed execution policy.

setlocal Env
set "scriptPath=%~dp0install.ps1"

echo ==================================================
echo Launching Apple A18 Pro Driver Installer...
echo ==================================================

:: Check for administrative rights
net session >nul 2>&1
if %errorLevel% == 0 (
    :: Already elevated, run PowerShell script
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%scriptPath%"
) else (
    :: Try to self-elevate via PowerShell Start-Process RunAs
    echo Requesting Administrator privileges...
    powershell.exe -NoProfile -Command "Start-Process powershell.exe -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File \"%scriptPath%\"' -Verb RunAs"
)

endlocal
