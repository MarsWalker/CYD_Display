@echo off
rem Forward arguments to the PowerShell builder, preserving paths containing spaces.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
exit /b %errorlevel%
