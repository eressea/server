@ECHO OFF
SET VSVERSION=10
SET SRCDIR=%CD%
CD ..
SET ERESSEA=%CD%

mkdir build-vs%VSVERSION%
cd build-vs%VSVERSION%
"%ProgramFiles(x86)%\CMake\bin\cmake.exe" -G "Visual Studio %VSVERSION%" -DCMAKE_PREFIX_PATH="%ProgramFiles(x86)%/Lua/5.1;%ERESSEA%/dependencies-win32" -DCMAKE_MODULE_PATH="%ERESSEA%/server/cmake/Modules" -DCMAKE_SUPPRESS_REGENERATION=TRUE ..
