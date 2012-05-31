cmake_minimum_required(VERSION 2.4)
project (example C)

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
else (WIN32)
  include_directories (/usr/include/lua5.1/ /usr/include/libxml2)
endif (WIN32)

include_directories (
 ../external/bson
 ../external/crypto
 ../external/cutest
 ../external/critbit
 ../external/iniparser
 ../external/pdcurses
 ../external/quicklist
 ../shared/src
)

set (LIB_SRCS
 ../external/bson/bson.c
 ../external/bson/numbers.c
 ../external/critbit/critbit.c
 ../external/crypto/md5.c
 ../external/crypto/mt19937ar.c
 ../external/cutest/CuTest.c
 ../external/dlmalloc/malloc.c
 ../external/quicklist/quicklist.c
 ../external/iniparser/iniparser.c
 ../shared/src/build/gamecode.c
 ../shared/src/build/kernel.c
 ../shared/src/build/lib.c
 ../shared/src/build/util.c
)

set (LINK_LIBS tolua xml2 ncurses lua5.1 pthread)

if (NEED_SQLITE)
 set(LIB_SRCS
   ${LIB_SRCS}
   ../external/sqlite3/sqlite3.c
 )
 include_directories (../external/sqlite3)
else (NEED_SQLITE)
 set (LINK_LIBS ${LINK_LIBS} sqlite3)
endif (NEED_SQLITE)
 
set (EXE_SRCS
  src/server.c
)

add_executable (example ${LIB_SRCS} ${EXE_SRCS})
if (WIN32)
else (WIN32)
  target_link_libraries (example ${LINK_LIBS})
endif (WIN32)
