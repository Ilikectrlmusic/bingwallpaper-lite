@echo off
setlocal
rem English uninstall: choose full uninstall or task-only removal, with English result dialogs.
set "BWL_ROOT=%~dp0"
cd /d "%TEMP%"
(
    powershell.exe -STA -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%BWL_ROOT%uninstall-portable.ps1" -AppDirectory "%BWL_ROOT%." -Language en
    exit /b
)
