@echo off
setlocal EnableExtensions
rem ==============================================================
rem  build.cmd - compile/upload of the dash001 sketch without the
rem  Arduino IDE.
rem
rem  Usage:
rem    build.cmd           -> compile only (result in build_out\)
rem    build.cmd COM4      -> compile and upload to port COM4
rem    build.cmd 4         -> same (the COM prefix is added)
rem
rem  Optional config (set before running):
rem    set ARD_CLI=C:\path\arduino-cli.exe   -> force the binary path
rem
rem  Searches arduino-cli: project folder, Arduino IDE 2.x at
rem  C:\Program Files\Arduino IDE (recursive), LocalAppData and PATH.
rem  Reuses the cores installed by the IDE (%LocalAppData%\Arduino15).
rem  Increments the build number (build_nr.txt / build.h).
rem ==============================================================

set "SKETCH=%~dp0"
set "LIBS=%~dp0..\libraries"
set "BUILD=%~dp0build_out"
set "FQBN=esp32:esp32:esp32"

rem ---- build number: read build_nr.txt, increment, regenerate build.h ----
set "NRFILE=%~dp0build_nr.txt"
set "NR=0"
if exist "%NRFILE%" set /p NR=<"%NRFILE%"
set /a NR+=1
> "%NRFILE%" echo %NR%
> "%~dp0build.h" echo #ifndef BUILD_NR_H
>> "%~dp0build.h" echo #define BUILD_NR_H
>> "%~dp0build.h" echo.
>> "%~dp0build.h" echo #define BUILD_NR %NR%
>> "%~dp0build.h" echo.
>> "%~dp0build.h" echo #endif

rem ---- locate arduino-cli --------------------------------------
set "CLI="
if defined ARD_CLI if exist "%ARD_CLI%" set "CLI=%ARD_CLI%"
if not defined CLI if exist "%~dp0arduino-cli.exe" set "CLI=%~dp0arduino-cli.exe"
for %%D in (
    "%ProgramFiles%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%ProgramFiles%\Arduino IDE\resources\arduino-cli.exe"
    "%ProgramFiles(x86)%\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%LocalAppData%\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
    "%LocalAppData%\Programs\Arduino IDE\resources\arduino-cli.exe"
) do if not defined CLI if exist "%%~D" set "CLI=%%~D"
for /f "delims=" %%F in ('dir /s /b "%ProgramFiles%\Arduino IDE\arduino-cli.exe" 2^>nul') do if not defined CLI set "CLI=%%F"
for /f "delims=" %%F in ('dir /s /b "%LocalAppData%\Programs\Arduino IDE\arduino-cli.exe" 2^>nul') do if not defined CLI set "CLI=%%F"
for /f "delims=" %%F in ('where arduino-cli 2^>nul') do if not defined CLI set "CLI=%%F"
if not defined CLI set "CLI=arduino-cli"

"%CLI%" version >nul 2>&1
if errorlevel 1 goto :cli_error

if exist "%LocalAppData%\Arduino15" set "ARDUINO_DIRECTORIES_DATA=%LocalAppData%\Arduino15"

echo.
echo Sketch  : %SKETCH%
echo Libs    : %LIBS%
echo Build   : %BUILD%
echo Build Nr: %NR%
echo FQBN    : %FQBN%
echo CLI     : %CLI%
echo Data    : %ARDUINO_DIRECTORIES_DATA%
echo.

"%CLI%" compile --fqbn "%FQBN%" --libraries "%LIBS%" --output-dir "%BUILD%" "%SKETCH%"
if errorlevel 1 goto :compile_error

if "%~1"=="" goto :done

set "PORT=%~1"
if not "%PORT:~0,3%"=="COM" set "PORT=COM%PORT%"
echo.
echo Uploading to %PORT% ...
"%CLI%" upload -p %PORT% --fqbn "%FQBN%" --build-path "%BUILD%" "%SKETCH%"
if errorlevel 1 goto :upload_error

:done
echo.
echo Done.
goto :end

:cli_error
echo.
echo [ERROR] arduino-cli not found. Install it or set ARD_CLI.
goto :end

:compile_error
echo.
echo [ERROR] Compilation failed.
goto :end

:upload_error
echo.
echo [ERROR] Upload failed.
goto :end

:end
endlocal