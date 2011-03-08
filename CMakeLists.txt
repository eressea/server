cmake_minimum_required(VERSION 2.4)
project (example C)

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
include_directories (../external ../shared/src)

set (LIB_SRCS
 ../external/sqlite3.c
 ../external/md5.c
 ../external/bson/bson.c
 ../external/bson/numbers.c
 ../external/dlmalloc/malloc.c
 ../external/cutest/CuTest.c
 ../shared/src/build/gamecode.c
 ../shared/src/build/kernel.c
 ../shared/src/build/lib.c
 ../shared/src/build/util.c
)

set (EXE_SRCS
  src/server.c
)

add_executable (example ${LIB_SRCS} ${EXE_SRCS})
if (WIN32)
else (WIN32)
  target_link_libraries (example tolua xml2 ncurses lua5.1 pthread)
endif (WIN32)
