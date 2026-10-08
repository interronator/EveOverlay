@echo off
setlocal
cd /d "%~dp0"
title Eve Overlay build

rem Usage: build.bat [clean] [debug] [tests] [run] [nopause]
set CLEAN=0
set RUN=0
set TESTS=0
set NOPAUSE=0
set CONFIG=Release
set PRESET=release
for %%A in (%*) do (
  if /I "%%A"=="clean" set CLEAN=1
  if /I "%%A"=="run" set RUN=1
  if /I "%%A"=="tests" set TESTS=1
  if /I "%%A"=="nopause" set NOPAUSE=1
  if /I "%%A"=="debug" set CONFIG=Debug& set PRESET=debug
)

echo === Eve Overlay build (%CONFIG%) ===
echo Folder: %CD%
echo.

where cmake >nul 2>&1
if errorlevel 1 goto :nocmake
where git >nul 2>&1
if errorlevel 1 goto :nogit

if "%VCPKG_ROOT%"=="" if exist "C:\vcpkg\vcpkg.exe" set VCPKG_ROOT=C:\vcpkg
if "%VCPKG_ROOT%"=="" goto :novcpkg
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" goto :novcpkg
echo vcpkg: %VCPKG_ROOT%

if "%CLEAN%"=="1" if exist build rmdir /s /q build

echo.
echo [1/3] Configuring (installs dependencies on first run)...
cmake --preset default
if errorlevel 1 goto :cfgfail

echo.
echo [2/3] Building...
if "%TESTS%"=="1" (
  cmake --build --preset %PRESET% --parallel
) else (
  cmake --build --preset %PRESET% --parallel --target EveOverlay
)
if errorlevel 1 goto :buildfail

echo.
echo [3/3] Checking output...
set OUT=Binaries\%CONFIG%
set EXE=%OUT%\EveOverlay.exe
if not exist "%EXE%" goto :noexe
if not exist "%OUT%\universeData\systems.csv" goto :noassets

if "%TESTS%"=="1" call :runtests
if errorlevel 1 goto :testfail

echo.
echo SUCCESS: %CD%\%EXE%
if "%RUN%"=="1" start "" "%EXE%"
goto :done

:runtests
if not exist "%TEMP%\EveOverlayConfigProbe" mkdir "%TEMP%\EveOverlayConfigProbe"
echo.
echo Running probes...
for %%P in (CoreProbe ServicesProbe UiProbe SyncerProbe) do (
  if exist "%OUT%\%%P.exe" (
    "%OUT%\%%P.exe"
    if errorlevel 1 exit /b 1
  )
)
if exist "%OUT%\ConfigProbe.exe" (
  "%OUT%\ConfigProbe.exe" "%TEMP%\EveOverlayConfigProbe"
  if errorlevel 1 exit /b 1
)
exit /b 0

:nocmake
echo [error] CMake was not found on PATH. Install it from https://cmake.org/download/
echo         (choose "Add CMake to the system PATH"), then reopen this window.
goto :done
:nogit
echo [error] Git was not found on PATH. vcpkg needs it to fetch dependencies.
echo         Install from https://git-scm.com/download/win
goto :done
:novcpkg
echo [error] vcpkg was not found. Install it from https://github.com/microsoft/vcpkg
echo         and set the VCPKG_ROOT environment variable to its folder.
goto :done
:cfgfail
echo [error] CMake configure failed. Make sure Visual Studio 2022 is installed with the
echo         "Desktop development with C++" workload (it includes ATL).
goto :done
:buildfail
echo [error] Build failed. See the messages above.
goto :done
:testfail
echo [error] A probe failed. See the messages above.
goto :done
:noexe
echo [error] Build finished but EveOverlay.exe was not found in Binaries\%CONFIG%.
goto :done
:noassets
echo [error] Build finished but Binaries\%CONFIG%\universeData\systems.csv is missing.
goto :done

:done
echo.
if "%NOPAUSE%"=="0" pause
endlocal
