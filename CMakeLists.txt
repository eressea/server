cmake_minimum_required(VERSION 2.8)

if (WIN32)
FILE(TO_CMAKE_PATH "${CMAKE_MODULE_PATH}" CMAKE_MODULE_PATH )
FILE(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH )
endif(WIN32)

project (eressea-server C)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/" ${CMAKE_MODULE_PATH})

if (MSVC)
include(MSVC)
set (HAVE_STRDUP 0)
set (HAVE_STRLCAT 0)
set (HAVE_LIBBSD 0)
else (MSVC)

INCLUDE (CheckIncludeFile)
CHECK_INCLUDE_FILE(bsd/string.h HAVE_LIBBSD)

INCLUDE (CheckFunctionExists)
CHECK_FUNCTION_EXISTS(strdup HAVE_STRDUP)
IF (HAVE_LIBBSD)
INCLUDE (CheckLibraryExists)
CHECK_LIBRARY_EXISTS(bsd strlcat "bsd/string.h" HAVE_STRLCAT)
ELSE (HAVE_LIBBSD)
CHECK_FUNCTION_EXISTS(strlcat HAVE_STRLCAT)
ENDIF(HAVE_LIBBSD)

endif (MSVC)

if (MSVC)
find_package (PDCurses)
SET(CURSES_FOUND ${PDCURSES_FOUND})
SET(CURSES_LIBRARIES ${PDCURSES_LIBRARY})
SET(CURSES_INCLUDE_DIR ${PDCURSES_INCLUDE_DIR})
else (MSVC)
find_package (Curses)
endif (MSVC)

if (ERESSEA_DB STREQUAL "db")
find_package (BerkeleyDB REQUIRED QUIET)
else()
find_package (SQLite3 REQUIRED QUIET)
endif()

find_package (LibXml2 REQUIRED)
find_package (ToLua REQUIRED)
if (TOLUA_FOUND)
if (${TOLUA_VERSION_STRING} VERSION_EQUAL "5.2")
find_package (Lua 5.2 REQUIRED)
else ()
find_package (Lua51 REQUIRED)
endif()
endif(TOLUA_FOUND)

enable_testing()

add_subdirectory (cJSON)
add_subdirectory (storage)
add_subdirectory (iniparser)
add_subdirectory (clibs)
add_subdirectory (process)
add_subdirectory (src eressea)

install(DIRECTORY etc DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.txt")
install(DIRECTORY res conf DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.xml")
install(DIRECTORY res conf DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.json")
install(DIRECTORY scripts DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.lua")
install(DIRECTORY lunit DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.lua")
install(DIRECTORY share DESTINATION ${CMAKE_INSTALL_PREFIX})


