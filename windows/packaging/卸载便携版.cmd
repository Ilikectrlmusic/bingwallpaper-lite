@echo off
setlocal
chcp 65001 >nul
rem 中文卸载入口：直接完整卸载程序与数据，并显示中文结果。
set "BWL_ROOT=%~dp0"
cd /d "%TEMP%"
(
    powershell.exe -STA -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%BWL_ROOT%uninstall-portable.ps1" -AppDirectory "%BWL_ROOT%." -Language zh
    exit /b
)
