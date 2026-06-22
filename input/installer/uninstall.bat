@echo off
setlocal Env
set "scriptPath=%~dp0uninstall.ps1"
echo Launching Apple A18 Pro Input Driver Uninstaller...
net session >nul 2>&1
if %errorLevel% == 0 (
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%scriptPath%"
) else (
    powershell.exe -NoProfile -Command "Start-Process powershell.exe -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File \"%scriptPath%\"' -Verb RunAs"
)
endlocal
