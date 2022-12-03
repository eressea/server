cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 99)
if (WIN32)
file(TO_CMAKE_PATH "${CMAKE_MODULE_PATH}" CMAKE_MODULE_PATH )
file(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH )
# make subdirectories build to the same output folders (DLLs):
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif (WIN32)

project (server C)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

if (MSVC)
  find_package (PDCurses)
  set (CURSES_FOUND ${PDCURSES_FOUND})
  set (CURSES_LIBRARIES ${PDCURSES_LIBRARY})
  set (CURSES_INCLUDE_DIRS ${PDCURSES_INCLUDE_DIR})
endif(MSVC)

if (NOT CURSES_FOUND)
  set(CURSES_NEED_WIDE TRUE)
  set(CURSES_NEED_NCURSES TRUE)
  find_package (Curses)
  if (NOT CURSES_FOUND)
    set(CURSES_NEED_WIDE FALSE)
    find_package (Curses REQUIRED)
  endif(NOT CURSES_FOUND)  
endif (NOT CURSES_FOUND)

find_package (EXPAT REQUIRED)
find_package (Lua 5.2 REQUIRED)
find_package (Utf8Proc REQUIRED)

find_library(SQLITE3_LIBRARY sqlite3 REQUIRED)
find_path(SQLITE3_INCLUDE_DIR NAMES sqlite3.h REQUIRED)

find_library(CJSON_LIBRARY cjson REQUIRED)
find_path(CJSON_INCLUDE_DIR NAMES cJSON.h PATH_SUFFIXES cjson REQUIRED)

find_library(INIPARSER_LIBRARY iniparser REQUIRED)
find_path(INIPARSER_INCLUDE_DIR NAMES iniparser.h PATH_SUFFIXES iniparser REQUIRED)

find_library(TOLUA_LIBRARY tolua REQUIRED)
find_path(TOLUA_INCLUDE_DIR NAMES tolua.h HINTS ${LUA_INCLUDE_DIR} REQUIRED)

if (MSVC)
set (HAVE_STRDUP 0)
set (HAVE_STRLCAT 0)
set (HAVE_LIBBSD 0)
set (HAVE_SIGNAL_H 0)
set (HAVE_EXECINFO_H 0)
else (MSVC)
include (CheckIncludeFile)
check_include_file(signal.h HAVE_SIGNAL_H)
check_include_file(execinfo.h HAVE_EXECINFO_H)

include (CheckLibraryExists)
check_library_exists(m fmin "" HAVE_LIBM)
check_library_exists(bsd strlcat "" HAVE_LIBBSD)

include (CheckFunctionExists)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strlcat HAVE_STRLCAT)

if (HAVE_LIBBSD)
  set (HAVE_STRLCAT 1)
endif (HAVE_LIBBSD)
endif (MSVC)

include_directories(include)

enable_testing()

add_subdirectory (tools)
add_subdirectory (storage)
add_subdirectory (clibs)
add_subdirectory (process)
add_subdirectory (src eressea)

install(DIRECTORY etc DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.txt")
install(DIRECTORY res conf DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.po")
install(DIRECTORY res conf DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.xml")
install(DIRECTORY res conf DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.json")
install(DIRECTORY scripts DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.lua")
install(DIRECTORY lunit DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.lua")
install(DIRECTORY share DESTINATION ${CMAKE_INSTALL_PREFIX})


