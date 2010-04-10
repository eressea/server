cmake_minimum_required(VERSION 2.4)
project (eressea)

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
else (WIN32)
  include_directories (/usr/include/lua5.1/ /usr/include/libxml2)
  set(CMAKE_C_FLAGS "-g -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts")
endif (WIN32)
include_directories (../external ../server/src)

set (LIB_SRCS
 ../external/sqlite3.c
 ../external/md5.c
 ../external/bson/bson.c
 ../external/bson/numbers.c
 ../external/dlmalloc/malloc.c
 ../server/src/build/gamecode.c
 ../server/src/build/kernel.c
 ../server/src/build/lib.c
 ../server/src/build/util.c
)

set (ERESSEA_SRCS
 src/server.c
)

add_executable (eressea ${LIB_SRCS} ${ERESSEA_SRCS})
if (WIN32)
else (WIN32)
  target_link_libraries (eressea tolua xml2 ncurses lua5.1 pthread)
endif (WIN32)

add_definitions(-DUNITY_BUILD)
set (TESTS_SRCS
 ../server/src/tests.c
 ../external/cutest/CuTest.c
)
add_executable (tests ${LIB_SRCS} ${TESTS_SRCS})
if (WIN32)
else (WIN32)
  target_link_libraries (tests tolua xml2 ncurses lua5.1 pthread)
endif (WIN32)
