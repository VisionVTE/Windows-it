@echo off
:: Batch wrapper to execute uninstall.ps1 with administrator privileges
:: and bypassed execution policy.

setlocal Env
set "scriptPath=%~dp0uninstall.ps1"

echo ==================================================
echo Launching Apple A18 Pro Chipset Driver Uninstaller...
echo ==================================================

net session >nul 2>&1
if %errorLevel% == 0 (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%scriptPath%"
) else (
    echo Requesting Administrator privileges...
    powershell.exe -NoProfile -Command "Start-Process powershell.exe -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File \"%scriptPath%\"' -Verb RunAs"
)

endlocal
