@echo off
setlocal
chcp 65001 >nul
set "BWL_ROOT=%~dp0"
cd /d "%TEMP%"
(
    powershell.exe -STA -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%BWL_ROOT%uninstall-portable.ps1" -AppDirectory "%BWL_ROOT%."
    exit /b
)
