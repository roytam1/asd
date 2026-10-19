@echo off
rem -------------------------------------------------------------
rem  Build ASD / UNASD with MSVC 6.0 (VC98)
rem
rem  Usage:  build.bat
rem
rem  The exe's are placed in the "build" subdirectory.
rem -------------------------------------------------------------

rem always build in the directory of this script
rem (pushd is used because it also works when the source is on a share)
pushd "%~dp0"
if errorlevel 1 (
  echo Could not enter the source directory !
  exit /b 1
)

set VCVARS=%VSCOMNTOOLS%..\..\VC98\Bin\VCVARS32.BAT
if not exist "%VCVARS%" set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT

call "%VCVARS%"

rem check that the compiler is available
cl >nul 2>&1
if errorlevel 1 (
  echo Could not setup the MSVC environment !
  popd
  exit /b 1
)

if not exist build mkdir build

echo.
echo *** Building ASD.EXE ***
cl /nologo /W3 /O2 /I. /Fo.\build\ /Fe.\build\asd.exe asd.c crc.c encode.c util.c
if errorlevel 1 (
  echo *** FAILED TO BUILD ASD.EXE ***
  popd
  exit /b 1
)

echo.
echo *** Building UNASD.EXE ***
cl /nologo /W3 /O2 /I. /Fo.\build\ /Fe.\build\unasd.exe unasd.c crc.c encode.c util.c
if errorlevel 1 (
  echo *** FAILED TO BUILD UNASD.EXE ***
  popd
  exit /b 1
)

echo.
echo *** Done, the exe's are in the build directory ***
dir /b .\build\*.exe

popd
