install lua from https://sourceforge.net/projects/luabinaries/ to c:\Program Files\Lua
install luarocks from http://luarocks.github.io/luarocks/releases/ to c:\Program Files\Lua
run luarocks install lunitx

set environment variables:
LUA_PATH C:\Users\Enno\AppData\Roaming\Luarocks\share\lua\5.4\?.lua
ERESSEA_ROOT <where the orders and data files are>

Visual Studio:
- File -> Open -> CMake ...
    Open the CMakeLists.txt at the repositoy's base directory
- Project -> Delete Cache and Reconfigure
- Debug -> Debug and Launch settings ...
    opens launch.vs.json

{
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "eressea.exe",
      "args": [
        "-w0",
        "-v1",
        "-ce2",
        "-re2",
        "-D",
        "-i",
        "${workspaceRoot}",
        "-t",
        "1349",
        "${workspaceRoot}\\scripts\\run-turn.lua"  
      ],
      "currentDir": "${env.ERESSEA_ROOT}",
      "env": {
        "LUA_PATH": "${workspaceRoot}\\scripts\\?.lua;${workspaceRoot}\\scripts\\?\\init.lua;${env.LUA_PATH}"
      },
      "name": "eressea.exe"
    },
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "test_eressea.exe",
      "name": "test_eressea.exe",
      "args": []
    }
  ]
}
