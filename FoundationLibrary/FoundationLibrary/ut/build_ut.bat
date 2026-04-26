@echo off
setlocal EnableDelayedExpansion

:: ---------------------------------------------------------------------------
:: Foundation Library unit test build script.
:: Compiles all library sources and UT sources into a single test executable
:: using MSVC x64, then runs it.  Exit code mirrors the test result.
:: ---------------------------------------------------------------------------

:: --- Locate vcvarsall.bat ---------------------------------------------------
set VCVARSALL=
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
)
if not defined VCVARSALL (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat"
    )
)
if not defined VCVARSALL (
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
        set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
    )
)
if not defined VCVARSALL (
    echo ERROR: Could not find vcvarsall.bat. Make sure Visual Studio 2022 is installed.
    exit /b 1
)

:: --- Set up x64 environment ------------------------------------------------
call "%VCVARSALL%" x64
if errorlevel 1 (
    echo ERROR: vcvarsall.bat failed.
    exit /b 1
)

:: --- Prepare output directory ----------------------------------------------
set "BASE_DIR=%~dp0..\..\"
echo BASE_DIR:
echo !BASE_DIR!

set "BUILD_DIR=%BASE_DIR%FoundationLibrary\ut\build"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: --- Move to project directory so unquoted globs expand correctly ----------
:: (cmd's for loop only expands wildcards in unquoted tokens)
pushd "%BASE_DIR%"

:: --- Source files -----------------------------------------------------------
set SOURCES=
for %%f in (FoundationLibrary\source\*.c) do set "SOURCES=!SOURCES! FoundationLibrary\source\%%~nxf"
for %%f in (FoundationLibrary\ut\*.c)     do set "SOURCES=!SOURCES! FoundationLibrary\ut\%%~nxf"

:: --- Compiler flags --------------------------------------------------------
::   /nologo          Suppress banner
::   /W3              Warning level 3
::   /WX              Treat warnings as errors
::   /sdl             SDL security checks
::   /TC              Compile all files as C (not C++)
::   /MTd             Multithreaded debug static runtime
::   /Zi              Full debug information
::   /EHs-c-          Disable C++ exception handling
::   /D_DEBUG         Debug build define
::   /D_CONSOLE       Console subsystem define
::   /I               Include search paths

set CL_FLAGS=/nologo /W3 /WX /sdl /TC /MTd /Zi /EHs-c- /D_DEBUG /D_CONSOLE
set INCLUDES=/I"%BASE_DIR%FoundationLibrary\include" /I"%BASE_DIR%FoundationLibrary\ut"
set OUT_EXE="%BUILD_DIR%\FlUt.exe"
set OUT_PDB="%BUILD_DIR%\FlUt.pdb"

echo SOURCES:
echo !SOURCES!

:: --- Compile and link -------------------------------------------------------
echo Building unit tests...
cl %CL_FLAGS% %INCLUDES% %SOURCES% /Fe%OUT_EXE% /Fd%OUT_PDB% /Fo"%BUILD_DIR%\\" /link /SUBSYSTEM:CONSOLE
if errorlevel 1 (
    echo ERROR: Build failed.
    popd
    exit /b 1
)

popd

:: --- Run tests --------------------------------------------------------------
echo.
echo Running unit tests...
echo.
%OUT_EXE%
set TEST_RESULT=%errorlevel%

echo.
if %TEST_RESULT% equ 0 (
    echo Build and test run completed successfully.
) else (
    echo Unit tests reported failures.
)

exit /b %TEST_RESULT%
