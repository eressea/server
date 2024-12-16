@ECHO OFF
IF EXIST ..\build\x64-Debug\eressea.exe SET BUILD=..\build\x64-Debug

SET SERVER=%BUILD%\eressea.exe
%BUILD%\test_eressea.exe
SET LUA_PATH=..\scripts\?.lua;%LUA_PATH%

%SERVER% -v1 ..\scripts\run-tests.lua
%SERVER% -v1 -re2 ..\scripts\run-tests-e2.lua
%SERVER% -v1 -re3 ..\scripts\run-tests-e3.lua
%SERVER% --version
PAUSE
RMDIR /s /q reports
DEL score score.alliances datum turn
