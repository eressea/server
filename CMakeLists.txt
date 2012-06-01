cmake_minimum_required(VERSION 2.4)
project (server C)

## CMake configuration ##
# You can pass these to cmake with -DNEED_OPTION=1
#
# if your system does not have libsqlite3-dev installed:
# set(NEED_SQLITE 1)

if (${CMAKE_C_COMPILER} MATCHES ".*tcc")
  set(CMAKE_C_FLAGS "-Wall -g")
  add_definitions(-DTINYCC)
endif(${CMAKE_C_COMPILER} MATCHES ".*tcc")

if (${CMAKE_C_COMPILER} MATCHES ".*gcc")
  set(CMAKE_C_FLAGS "-g -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts -pedantic -Wno-long-long")
endif(${CMAKE_C_COMPILER} MATCHES ".*gcc")

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
  include_directories ( ../dependencies/pdcurses )
else (WIN32)
  include_directories ( /usr/include/lua5.1/ /usr/include/libxml2 )
endif (WIN32)

include_directories (
 ../dependencies/bson
 ../dependencies/crypto
 ../dependencies/cutest
 ../dependencies/critbit
 ../dependencies/iniparser
 ../dependencies/quicklist
 ../core/src
)

set (LIB_SRCS
 ../dependencies/bson/bson.c
 ../dependencies/bson/numbers.c
 ../dependencies/critbit/critbit.c
 ../dependencies/crypto/md5.c
 ../dependencies/crypto/mt19937ar.c
 ../dependencies/cutest/CuTest.c
 ../dependencies/dlmalloc/malloc.c
 ../dependencies/quicklist/quicklist.c
 ../dependencies/iniparser/iniparser.c
 ../core/src/build/gamecode.c
 ../core/src/build/kernel.c
 ../core/src/build/lib.c
 ../core/src/build/util.c
)

set (LINK_LIBS tolua xml2 ncurses lua5.1 pthread)

if (NEED_SQLITE)
 set( LIB_SRCS ${LIB_SRCS} ../dependencies/sqlite3/sqlite3.c )
 include_directories ( ../dependencies/sqlite3 )
else (NEED_SQLITE)
 set ( LINK_LIBS ${LINK_LIBS} sqlite3 )
endif (NEED_SQLITE)
 
set ( EXE_SRCS src/server.c )

add_executable ( server ${LIB_SRCS} ${EXE_SRCS} )
if (WIN32)
else (WIN32)
  target_link_libraries (server ${LINK_LIBS})
endif (WIN32)
