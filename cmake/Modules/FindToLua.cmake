# - Try to find the ToLua library
# Once done this will define
#
#  TOLUA_FOUND - System has ToLua
#  TOLUA_INCLUDE_DIR - The ToLua include directory
#  TOLUA_LIBRARIES - The libraries needed to use ToLua
#  TOLUA_DEFINITIONS - Compiler switches required for using ToLua
#  TOLUA_EXECUTABLE - The ToLua command line shell
#  TOLUA_VERSION_STRING - the version of ToLua found (since CMake 2.8.8)

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_TOLUA QUIET ToLua)
set(TOLUA_DEFINITIONS ${PC_TOLUA_CFLAGS_OTHER})

find_path(TOLUA_INCLUDE_DIR NAMES tolua.h
   HINTS
   ${PC_TOLUA_DIR}/include
   ${PC_TOLUA_INCLUDEDIR}
   ${PC_TOLUA_INCLUDE_DIRS}
   )
find_library(TOLUA_LIBRARY NAMES tolua
   HINTS
   ${PC_TOLUA_DIR}/lib
   ${PC_TOLUA_LIBDIR}
   ${PC_TOLUA_LIBRARY_DIRS}
   )
find_program(TOLUA_EXECUTABLE tolua
   HINTS
   ${PC_TOLUA_DIR}/bin
   ${PC_TOLUA_LIBDIR}
   ${PC_TOLUA_LIBRARY_DIRS}
)

SET( TOLUA_LIBRARIES "${TOLUA_LIBRARY}" CACHE STRING "ToLua Libraries")

if(PC_TOLUA_VERSION)
    set(TOLUA_VERSION_STRING ${PC_TOLUA_VERSION})
elseif(TOLUA_INCLUDE_DIR AND EXISTS "${TOLUA_INCLUDE_DIR}/tolua.h")
    file(STRINGS "${TOLUA_INCLUDE_DIR}/tolua.h" tolua_version_str
         REGEX "^#define[\t ]+TOLUA_VERSION[\t ]+\".*\"")

    string(REGEX REPLACE "^#define[\t ]+TOLUA_VERSION[\t ]+\"tolua ([^\"]*)\".*" "\\1"
           TOLUA_VERSION_STRING "${tolua_version_str}")
    unset(tolua_version_str)
endif()

# handle the QUIETLY and REQUIRED arguments and set TOLUA_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ToLua
                                  REQUIRED_VARS TOLUA_LIBRARY TOLUA_INCLUDE_DIR TOLUA_EXECUTABLE
                                  VERSION_VAR TOLUA_VERSION_STRING)

mark_as_advanced(TOLUA_INCLUDE_DIR TOLUA_LIBRARIES TOLUA_EXECUTABLE)

