@ECHO OFF
SET VSVERSION=15
IF NOT "%1" == "" SET VSVERSION=%1%

SET SRCDIR=%CD%
CD ..
SET ERESSEA=%CD%

CD %SRCDIR%
IF exist build-vs%VSVERSION% goto HAVEDIR
mkdir build-vs%VSVERSION%
:HAVEDIR
cd build-vs%VSVERSION%
"%ProgramFiles%\CMake\bin\cmake.exe" -G "Visual Studio %VSVERSION%" -DCMAKE_PREFIX_PATH="%ProgramFiles(x86)%\Lua\5.1;%ERESSEA%\dependencies-win32" -DCMAKE_SUPPRESS_REGENERATION=TRUE ..
PAUSE
