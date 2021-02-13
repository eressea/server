# What is this?

This repository contains the source code for the Play-by-Mail 
strategy game [Eressea](http://www.eressea.de/).

# Prerequisites

Eressea depends on a number of external libraries. On a recent
Debian-based Linux system, this is the apt-get command to
install all of them:

    sudo apt-get install git cmake gcc make luarocks libxml2-dev \
        liblua5.2-dev libtolua-dev libncurses5-dev libsqlite3-dev \
        libcjson-dev libiniparser-dev

# How to check out and build the Eressea server

This repository relies heavily on the use of submodules, and it pulls in
most of the code from those. The build system being used is cmake, which
can create Makefiles on Unix, or Visual Studio project files on Windows.
Here's how you clone and build the source on Linux or macOS:

    git clone --recursive git://github.com/eressea/server.git source
    cd source
    git submodule update --init
    s/build

If you got this far and all went well, you have built the server, and
it will have passed some basic functionality tests.

* [![Static Analysis](https://scan.coverity.com/projects/6742/badge.svg?flat=1)](https://scan.coverity.com/projects/6742/)
* [![Build Status](https://api.travis-ci.org/eressea/server.svg?branch=develop)](https://travis-ci.org/eressea/server)
* [![License: CC BY-NC-SA 4.0](https://licensebuttons.net/l/by-nc-sa/4.0/80x15.png)](http://creativecommons.org/licenses/by-nc-sa/4.0/)
