@echo off
setlocal EnableDelayedExpansion

echo.
echo =====================================================
echo   Fusion Browser v9.0 - Build Script
echo =====================================================
echo.

:: PROJECT_DIR = folder where this .bat lives, NO trailing backslash
set "PROJECT_DIR=%~dp0"
if "%PROJECT_DIR:~-1%"=="\" set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"

set "BUILD_DIR=%PROJECT_DIR%\build"
set "DIST_DIR=%PROJECT_DIR%\dist\Fusion"
set "OUTPUT_DIR=%PROJECT_DIR%\installer_output"

:: -------------------------------------------------------
:: Step 0: Find Qt
:: -------------------------------------------------------

if defined QTDIR (
    if exist "%QTDIR%\bin\qmake.exe" (
        echo [OK] Qt from environment: %QTDIR%
        goto :qt_found
    )
)

echo [..] Searching for Qt automatically...

for %%R in (C D E F) do (
    for %%V in (6.10.2 6.10.1 6.10.0 6.9.1 6.9.0 6.8.3 6.8.2 6.8.1 6.8.0 6.7.3 6.7.2 6.7.1 6.7.0 6.6.3 6.6.2 6.6.1 6.6.0) do (
        for %%C in (msvc2022_64 msvc2019_64 mingw_64 mingw1310_64 mingw1120_64 mingw900_64) do (
            if exist "%%R:\Qt\%%V\%%C\bin\qmake.exe" (
                set "QTDIR=%%R:\Qt\%%V\%%C"
                echo [OK] Found: !QTDIR!
                goto :qt_found
            )
        )
    )
)

where qmake >nul 2>&1
if %errorlevel% equ 0 (
    for /f "delims=" %%Q in ('where qmake') do (
        set "_QP=%%~dpQ"
        set "QTDIR=!_QP:~0,-5!"
        echo [OK] Found via PATH: !QTDIR!
        goto :qt_found
    )
)

:: Not found - ask user
echo.
echo [!!] Qt not found automatically.
echo.
echo Enter the Qt kit path, for example:
echo   C:\Qt\6.10.2\mingw_64
echo   C:\Qt\6.8.2\msvc2022_64
echo.
:ask_qt
set "QTDIR="
set /p "QTDIR=Qt path: "
if not defined QTDIR goto :ask_qt
set "QTDIR=%QTDIR:"=%"
if "%QTDIR:~-1%"=="\" set "QTDIR=%QTDIR:~0,-1%"
if /i "%QTDIR:~-4%"=="\bin" set "QTDIR=%QTDIR:~0,-4%"

if exist "%QTDIR%\bin\qmake.exe" goto :qt_found

:: Scan inside user-given root
echo [..] Scanning inside %QTDIR% ...
set "_FOUND="
for /f "delims=" %%F in ('dir /s /b "%QTDIR%\qmake.exe" 2^>nul') do (
    if not defined _FOUND set "_FOUND=%%~dpF"
)
if defined _FOUND (
    set "_FOUND=!_FOUND:~0,-5!"
    set "QTDIR=!_FOUND!"
    echo [OK] Using: !QTDIR!
    goto :qt_found
)

echo ERROR: qmake.exe not found in %QTDIR%
echo Install Qt 6.6+ from https://www.qt.io/download-qt-installer
pause
exit /b 1

:qt_found
set "PATH=%QTDIR%\bin;%PATH%"
echo [OK] Qt: %QTDIR%
echo.

:: -------------------------------------------------------
:: Step 0b: Find CMake
:: -------------------------------------------------------
where cmake >nul 2>&1
if %errorlevel% equ 0 goto :cmake_found
for %%P in ("C:\Program Files\CMake\bin" "C:\Program Files (x86)\CMake\bin" "C:\CMake\bin") do (
    if exist "%%~P\cmake.exe" set "PATH=%%~P;!PATH!" & goto :cmake_found
)
echo ERROR: CMake not found. https://cmake.org/download/
pause
exit /b 1
:cmake_found
for /f "tokens=3" %%V in ('cmake --version 2^>nul') do echo [OK] CMake %%V & goto :cmake_ok
:cmake_ok
echo.

:: -------------------------------------------------------
:: Step 0c: Detect compiler
:: -------------------------------------------------------
set "GENERATOR="
set "CMAKE_EXTRA="

where cl >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] MSVC already in PATH
    goto :check_ninja
)

for %%V in (
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if exist "%%~V" (
        echo [OK] Initializing MSVC...
        call "%%~V" >nul 2>&1
        goto :check_ninja
    )
)

where g++ >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] MinGW-w64 from PATH
    set "GENERATOR=MinGW Makefiles"
    goto :compiler_done
)

:: MinGW bundled with Qt: QTDIR = C:\Qt\6.x.x\mingw_64
:: Tools folder is at:   C:\Qt\Tools\mingwXXX_64
:: So go up 2 levels from QTDIR then into Tools\
set "_QTROOT=%QTDIR%"
for %%A in ("%_QTROOT%\..") do set "_QTROOT=%%~fA"
for %%A in ("%_QTROOT%\..") do set "_QTROOT=%%~fA"

for /d %%G in ("%_QTROOT%\Tools\mingw*") do (
    if exist "%%~G\bin\g++.exe" (
        echo [OK] MinGW bundled with Qt: %%~G
        set "PATH=%%~G\bin;!PATH!"
        set "GENERATOR=MinGW Makefiles"
        goto :compiler_done
    )
)

:: Also try same drive C:\Qt\Tools regardless of where QTDIR is
for %%R in (C D E) do (
    for /d %%G in ("%%R:\Qt\Tools\mingw*") do (
        if exist "%%~G\bin\g++.exe" (
            echo [OK] MinGW at %%~G
            set "PATH=%%~G\bin;!PATH!"
            set "GENERATOR=MinGW Makefiles"
            goto :compiler_done
        )
    )
)

echo.
echo [!!] MinGW not found. Looking in C:\Qt\Tools\ ...
for /d %%G in ("C:\Qt\Tools\mingw*") do echo     Found: %%G
echo.
echo Enter MinGW bin path (e.g. C:\Qt\Tools\mingw1310_64\bin):
set "_MINGW="
set /p "_MINGW=MinGW bin: "
if not defined _MINGW (
    echo ERROR: MinGW required for this Qt build.
    pause
    exit /b 1
)
set "_MINGW=%_MINGW:"=%"
if "%_MINGW:~-1%"=="\" set "_MINGW=%_MINGW:~0,-1%"
if not exist "%_MINGW%\g++.exe" (
    echo ERROR: g++.exe not found at %_MINGW%
    pause
    exit /b 1
)
set "PATH=%_MINGW%;!PATH!"
set "GENERATOR=MinGW Makefiles"
goto :compiler_done

:check_ninja
where ninja >nul 2>&1
if %errorlevel% equ 0 (
    set "GENERATOR=Ninja"
    echo [OK] Ninja found
    goto :compiler_done
)
for %%P in ("C:\Program Files\CMake\bin" "C:\Program Files (x86)\CMake\bin") do (
    if exist "%%~P\ninja.exe" (
        set "PATH=%%~P;!PATH!"
        set "GENERATOR=Ninja"
        echo [OK] Ninja (from CMake dir)
        goto :compiler_done
    )
)
set "GENERATOR=Visual Studio 17 2022"
set "CMAKE_EXTRA=-A x64"
echo [OK] Using Visual Studio 17 2022 generator

:compiler_done
echo [OK] Generator: %GENERATOR%
echo.

:: -------------------------------------------------------
:: Step 1: Configure CMake
:: -------------------------------------------------------
echo [1/5] Configuring CMake...
echo       Source:    "%PROJECT_DIR%"
echo       Build:     "%BUILD_DIR%"
echo       Qt:        "%QTDIR%"
echo       Generator: %GENERATOR%
echo.

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

:: Build cmake command into a variable to avoid ^ continuation issues with spaces
set "CMAKE_CMD=cmake"
set "CMAKE_CMD=%CMAKE_CMD% -S "%PROJECT_DIR%""
set "CMAKE_CMD=%CMAKE_CMD% -B "%BUILD_DIR%""
set "CMAKE_CMD=%CMAKE_CMD% -G "%GENERATOR%""
if defined CMAKE_EXTRA set "CMAKE_CMD=%CMAKE_CMD% %CMAKE_EXTRA%"
set "CMAKE_CMD=%CMAKE_CMD% -DCMAKE_BUILD_TYPE=Release"
set "CMAKE_CMD=%CMAKE_CMD% -DCMAKE_PREFIX_PATH="%QTDIR%""

%CMAKE_CMD%

if %errorlevel% neq 0 (
    echo.
    echo ERROR: CMake configuration failed.
    echo Common causes:
    echo   - Qt WebEngine not installed (run Qt Maintenance Tool, add WebEngine)
    echo   - Compiler/Qt mismatch (MinGW Qt needs MinGW compiler)
    echo   - CMake version too old (need 3.20+)
    pause
    exit /b 1
)
echo [OK] CMake configured.

:: -------------------------------------------------------
:: Step 2: Build
:: -------------------------------------------------------
echo.
echo [2/5] Building... (2-5 minutes)
echo.

cmake --build "%BUILD_DIR%" --config Release --parallel
if %errorlevel% neq 0 (
    echo ERROR: Build failed.
    pause
    exit /b 1
)
echo [OK] Build done.

:: -------------------------------------------------------
:: Step 3: Prepare dist
:: -------------------------------------------------------
echo.
echo [3/5] Preparing dist\Fusion...

if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"

set "EXE_SRC="
for %%P in (
    "%PROJECT_DIR%\dist\Release\Fusion.exe"
    "%PROJECT_DIR%\dist\Fusion.exe"
    "%BUILD_DIR%\Release\Fusion.exe"
    "%BUILD_DIR%\Fusion.exe"
    "%BUILD_DIR%\dist\Release\Fusion.exe"
    "%BUILD_DIR%\dist\Fusion.exe"
) do (
    if exist "%%~P" if not defined EXE_SRC set "EXE_SRC=%%~P"
)

if not defined EXE_SRC (
    for /f "delims=" %%F in ('dir /s /b "%PROJECT_DIR%\Fusion.exe" 2^>nul') do (
        if not defined EXE_SRC set "EXE_SRC=%%F"
    )
)

if not defined EXE_SRC (
    echo ERROR: Fusion.exe not found after build.
    pause
    exit /b 1
)

copy /Y "%EXE_SRC%" "%DIST_DIR%\Fusion.exe"
echo [OK] Copied: %EXE_SRC%

if exist "%PROJECT_DIR%\resources\fusion.ico" (
    copy /Y "%PROJECT_DIR%\resources\fusion.ico" "%DIST_DIR%\fusion.ico"
)

:: -------------------------------------------------------
:: Step 4: windeployqt
:: -------------------------------------------------------
echo.
echo [4/5] Running windeployqt...

windeployqt --release --webengine "%DIST_DIR%\Fusion.exe"
if %errorlevel% neq 0 (
    echo [WARN] Retrying without --webengine flag...
    windeployqt --release "%DIST_DIR%\Fusion.exe"
)

for %%P in ("%QTDIR%\bin\QtWebEngineProcess.exe" "%QTDIR%\libexec\QtWebEngineProcess.exe") do (
    if exist "%%~P" if not exist "%DIST_DIR%\QtWebEngineProcess.exe" (
        copy /Y "%%~P" "%DIST_DIR%\" & echo [OK] Copied QtWebEngineProcess.exe
    )
)
for %%P in ("%QTDIR%\resources\icudtl.dat" "%QTDIR%\bin\icudtl.dat") do (
    if exist "%%~P" if not exist "%DIST_DIR%\icudtl.dat" (
        copy /Y "%%~P" "%DIST_DIR%\" & echo [OK] Copied icudtl.dat
    )
)
echo [OK] Deploy done.

:: -------------------------------------------------------
:: Step 5: Inno Setup (optional)
:: -------------------------------------------------------
echo.
echo [5/5] Building installer...

set "ISCC="
for %%P in (
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    "C:\Program Files\Inno Setup 6\ISCC.exe"
) do if exist "%%~P" set "ISCC=%%~P"

if not defined ISCC (
    echo [SKIP] Inno Setup 6 not found. https://jrsoftware.org/isinfo.php
) else (
    if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"
    "%ISCC%" "%PROJECT_DIR%\installer.iss"
    if %errorlevel% equ 0 echo [OK] Installer created.
)

:: -------------------------------------------------------
:: Done
:: -------------------------------------------------------
echo.
echo =====================================================
echo   DONE!
echo   App: %DIST_DIR%\Fusion.exe
if exist "%OUTPUT_DIR%\Fusion_Setup_v9.0.exe" (
    echo   Installer: %OUTPUT_DIR%\Fusion_Setup_v9.0.exe
)
echo =====================================================
echo.
start "" "%DIST_DIR%"
pause
