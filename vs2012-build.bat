@ECHO OFF
mkdir build-vs11
cd build-vs11
"c:\Program Files (x86)\CMake 2.8\bin\cmake.exe" -G "Visual Studio 11" -DCMAKE_PREFIX_PATH="C:/Program Files (x86)/Lua/5.1;C:/Users/Enno/Documents/Eressea/dependencies-win32" -DCMAKE_MODULE_PATH="C:/Users/Enno/Documents/Eressea/server/cmake/Modules" ..
