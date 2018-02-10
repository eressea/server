# What is this?

This repository contains the source code for the Play-by-Mail strategy game [Eressea](http://www.eressea.de/).

# Prerequisites

Eressea depends on a number of external libraries. On a recent Debian-based Linux system, this is the apt-get command to install all of them:

    sudo apt-get install git cmake gcc make libxml2-dev liblua5.2-dev libtolua-dev libncurses5-dev libsqlite3-dev

# How to check out and build the Eressea server

This repository relies heavily on the use of submodules, and it pulls in most of the code from those. The build system being used is cmake, which can create Makefiles on Unix, or Visual Studio project files on Windows. Here's how you clone and build the source on Ubuntu:

    git clone --recursive git://github.com/eressea/server.git
    cd server
    ./configure
    ./s/build

If you got this far and all went well, you have built a server (it is linked from the `game` subdirectory), and it will have passed some basic functionality tests.

* [![Static Analysis](https://scan.coverity.com/projects/6742/badge.svg?flat=1)](https://scan.coverity.com/projects/6742/)
* [![Build Status](https://api.travis-ci.org/eressea/server.svg?branch=develop)](https://travis-ci.org/eressea/server)
