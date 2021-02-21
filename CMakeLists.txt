cmake_minimum_required(VERSION 3.13)

if (WIN32)
file(TO_CMAKE_PATH "${CMAKE_MODULE_PATH}" CMAKE_MODULE_PATH )
file(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH )
endif(WIN32)

project (eressea-server C)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
if (WIN32)
# make subdirectories build to the same output folders (DLLs):
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif (WIN32)

if (MSVC)
find_package (PDCurses)
set (CURSES_FOUND ${PDCURSES_FOUND})
set (CURSES_LIBRARIES ${PDCURSES_LIBRARY})
set (CURSES_INCLUDE_DIRS ${PDCURSES_INCLUDE_DIR})
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

if (NOT CURSES_FOUND)
  set(CURSES_NEED_WIDE TRUE)
  set(CURSES_NEED_NCURSES TRUE)
  find_package (Curses)
endif (NOT CURSES_FOUND)

#find_package (BerkeleyDB REQUIRED)
find_package (SQLite3 REQUIRED)
find_package (IniParser REQUIRED)
find_package (CJSON REQUIRED)
find_package (EXPAT REQUIRED)

find_package (Lua)
if (NOT LLUA_FOUND)
find_package (Lua51 REQUIRED)
endif()

enable_testing()

add_subdirectory (tolua)
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


