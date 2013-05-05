cmake_minimum_required(VERSION 2.6)
project (e3-server C)

enable_testing()

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DHAVE__BOOL")
ELSE(CMAKE_COMPILER_IS_GNUCC)
    MESSAGE(STATUS "Unknown compiler ${CMAKE_C_COMPILER_ID}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

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
add_subdirectory (src server)
add_subdirectory (eressea/src eressea)
