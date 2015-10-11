@ECHO OFF
IF EXIST ..\build-vs10 SET BUILD=..\build-vs10\eressea\Debug
IF EXIST ..\build-vs11 SET BUILD=..\build-vs11\eressea\Debug
IF EXIST ..\build-vs12 SET BUILD=..\build-vs12\eressea\Debug
IF EXIST ..\build-vs14 SET BUILD=..\build-vs14\eressea\Debug
SET SERVER=%BUILD%\eressea.exe
%BUILD%\test_eressea.exe
%SERVER% ..\scripts\run-tests.lua
%SERVER% ..\scripts\run-tests-e2.lua
%SERVER% ..\scripts\run-tests-e3.lua
PAUSE
RMDIR /s /q reports
