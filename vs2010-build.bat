@ECHO OFF
mkdir build-vs10
cd build-vs10
SET ERESSEA=%Userprofile%/Documents/Projects/Eressea
"%ProgramFiles(x86)%\CMake 2.8\bin\cmake.exe" -G "Visual Studio 10" -DCMAKE_PREFIX_PATH="%ProgramFiles(x86)%/Lua/5.1;%ERESSEA%/dependencies-win32" -DCMAKE_MODULE_PATH="%ERESSEA%/server/cmake/Modules" -DCMAKE_SUPPRESS_REGENERATION=TRUE ..
