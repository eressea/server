# - Try to find the SQLite3 library
# Once done this will define
#
#  SQLITE3_FOUND - System has SQLite3
#  SQLITE3_INCLUDE_DIR - The SQLite3 include directory
#  SQLITE3_LIBRARIES - The libraries needed to use SQLite3
#  SQLITE3_DEFINITIONS - Compiler switches required for using SQLite3
#  SQLITE3_EXECUTABLE - The SQLite3 command line shell
#  SQLITE3_VERSION_STRING - the version of SQLite3 found (since CMake 2.8.8)

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
PKG_CHECK_MODULES(PC_SQLITE QUIET sqlite3)
set(SQLITE3_DEFINITIONS ${PC_SQLITE_CFLAGS_OTHER})

find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h
   HINTS
   ${PC_SQLITE_INCLUDEDIR}
   ${PC_SQLITE_INCLUDE_DIRS}
   )

find_library(SQLITE3_LIBRARIES NAMES sqlite3
   HINTS
   ${PC_SQLITE_LIBDIR}
   ${PC_SQLITE_LIBRARY_DIRS}
   )

find_program(SQLITE3_EXECUTABLE sqlite3)

if(PC_SQLITE_VERSION)
    set(SQLITE3_VERSION_STRING ${PC_SQLITE_VERSION})
elseif(SQLITE3_INCLUDE_DIR AND EXISTS "${SQLITE3_INCLUDE_DIR}/sqlite3.h")
    file(STRINGS "${SQLITE3_INCLUDE_DIR}/sqlite3.h" sqlite3_version_str
         REGEX "^#define[\t ]+SQLITE_VERSION[\t ]+\".*\"")

    string(REGEX REPLACE "^#define[\t ]+SQLITE_VERSION[\t ]+\"([^\"]*)\".*" "\\1"
           SQLITE3_VERSION_STRING "${sqlite3_version_str}")
    unset(sqlite3_version_str)
endif()

# handle the QUIETLY and REQUIRED arguments and set SQLITE3_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SQLite3
                                  REQUIRED_VARS SQLITE3_LIBRARIES SQLITE3_INCLUDE_DIR
                                  VERSION_VAR SQLITE3_VERSION_STRING)

mark_as_advanced(SQLITE3_INCLUDE_DIR SQLITE3_LIBRARIES SQLITE3_EXECUTABLE)
