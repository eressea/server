cmake_minimum_required(VERSION 2.6)
project (arda-server C)

enable_testing()

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckSymbolExists)
CHECK_INCLUDE_FILES (stdbool.h HAVE_STDBOOL_H)
CHECK_SYMBOL_EXISTS (_Bool "stdbool.h" HAVE__BOOL)
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H)

CONFIGURE_FILE (
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_BINARY_DIR}/include/config.h)
INCLUDE_DIRECTORIES (${CMAKE_BINARY_DIR}/include)

find_package (Lua 5 REQUIRED)
find_package (ToLua REQUIRED)
find_package (LibXml2 REQUIRED)
find_package (Curses REQUIRED)
find_package (SQLite3 REQUIRED)

add_subdirectory (crypto)
add_subdirectory (quicklist)
add_subdirectory (iniparser)
add_subdirectory (cutest)
add_subdirectory (critbit)
add_subdirectory (core/src eressea)
add_subdirectory (src server)
