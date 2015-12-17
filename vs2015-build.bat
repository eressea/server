@ECHO OFF
SET VSVERSION=14
SET SRCDIR=%CD%
CD ..
SET ERESSEA=%CD%

CD %SRCDIR%
mkdir build-vs%VSVERSION%
cd build-vs%VSVERSION%
"%ProgramFiles(x86)%\CMake\bin\cmake.exe" -G "Visual Studio %VSVERSION%" -DCMAKE_PREFIX_PATH="%ProgramFiles(x86)%/Lua/5.1;%ERESSEA%/dependencies-win32" -DCMAKE_MODULE_PATH="%SRCDIR%/cmake/Modules" -DCMAKE_SUPPRESS_REGENERATION=TRUE ..
PAUSE
