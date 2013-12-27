cmake_minimum_required(VERSION 2.6)
project (eressea-server C)

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

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DHAVE__BOOL")
ELSEIF(MSVC)
ELSE(CMAKE_COMPILER_IS_GNUCC)
    MESSAGE(STATUS "Unknown compiler ${CMAKE_C_COMPILER_ID}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

find_package (Lua 5 REQUIRED)
find_package (ToLua REQUIRED)
find_package (LibXml2 REQUIRED)
find_package (Curses REQUIRED)
find_package (SQLite3 REQUIRED)

add_subdirectory (src server)
add_subdirectory (crypto)
add_subdirectory (quicklist)
add_subdirectory (iniparser)
add_subdirectory (cutest)
add_subdirectory (critbit)
add_subdirectory (core/src eressea)
