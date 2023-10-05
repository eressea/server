# Eressea PBEM Server

This repository contains the source code for the Play-by-Mail 
strategy game [Eressea](http://www.eressea.de/).

## Prerequisites

Eressea depends on a number of external libraries. On a recent
Debian-based Linux system, this is the apt-get command to
install all of them:

    sudo apt-get install git cmake gcc make luarocks \
        liblua5.2-dev libtolua-dev libncurses5-dev libsqlite3-dev \
        libcjson-dev libiniparser-dev libexpat1-dev libutf8proc-dev

## How to check out and build the Eressea server

This repository relies heavily on the use of submodules, and it pulls in
most of the code from those. The build system being used is CMake.
Here's how you clone and build the source on Linux or macOS:

    git clone --recursive https://github.com/eressea/server.git source
    cd source
    git submodule update --init
    ./configure
    s/build

If you got this far and all went well, you have built the server, and
it will have passed some basic functionality tests.

* [![Static Analysis](https://scan.coverity.com/projects/6742/badge.svg?flat=1)](https://scan.coverity.com/projects/6742/)
* [![Build Status](https://api.travis-ci.org/eressea/server.svg?branch=develop)](https://travis-ci.org/eressea/server)
* [![License: CC BY-NC-SA 4.0](https://licensebuttons.net/l/by-nc-sa/4.0/80x15.png)](http://creativecommons.org/licenses/by-nc-sa/4.0/)

## Building on Windows

To build on Windows, first install Visual Studio and VCPKG: https://github.com/microsoft/vcpkg#quick-start-windows
Set an environment variable named VCPKG_ROOT to the directory where you installed VCPKG globally, e.g. C:\VCPKG
Now, install the required packages:
```
VCPKG.EXE install sqlite3 expat pdcurses cjson iniparser tolua
VCPKG.EXE integrate install
```

Using luarocks, install the lunitx module (this can be complicated).

In Visual Studio, clone the reportitory from github, or if you've already done
that, choose File -> Open -> CMake and pick the CMakeLists.txt file in the root
of the repository. Build the project.

Select eressea.exe as the Stratup Item, then open the launch.vs.json file by 
choosing Debug -> "Debug and Launch Settings for eressea.exe".

Here, add your data the directory in "currentDir", and command line arguments 
in "args". On my own computer, the configuration looks like this:
```
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
        "-t1242",
        "-v1",
        "-D",
        "${workspaceRoot}\\scripts\\run-turn.lua"
      ],
      "currentDir": "${env.ERESSEA_ROOT}",
      "env": {
        "ERESSEA_ROOT": "${workspaceRoot}",
        "LUA_PATH": "${workspaceRoot}\\scripts\\?.lua;${env.LUA_PATH}"
      },
      "name": "eressea.exe"
    },
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "test_eressea.exe",
      "name": "test_eressea.exe"
    }
  ]
}
```

Note that I have an environment variable for my game directory called ERESSEA_ROOT. My LUA_PATH also contains the luarocks installation.

```
LUA_PATH=C:\Users\Enno\AppData\Roaming\Luarocks\share\lua\5.4\?.lua
ERESSEA_ROOT C:\Users\Enno\Documents\Eressea\test
```
