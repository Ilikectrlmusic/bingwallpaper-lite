@echo off
setlocal
rem English uninstall: remove the application and data, with English result dialogs.
set "BWL_ROOT=%~dp0"
cd /d "%TEMP%"
(
    powershell.exe -STA -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%BWL_ROOT%uninstall-portable.ps1" -AppDirectory "%BWL_ROOT%." -Language en
    exit /b
)
