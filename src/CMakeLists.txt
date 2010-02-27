cmake_minimum_required(VERSION 2.4)
project (eressea)

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
else (WIN32)
  include_directories (src /usr/include/lua5.1/ /usr/include/libxml2)
  set(CMAKE_C_FLAGS "-g -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts")
endif (WIN32)
include_directories (external)

set (LIB_SRCS
 external/sqlite3.c
 external/md5.c
 external/bson/bson.c
 external/bson/numbers.c
 external/dlmalloc/malloc.c
 src/build/gamecode.c
 src/build/kernel.c
 src/build/lib.c
 src/build/util.c
)

set (ERESSEA_SRCS
 eressea/src/server.c
)

add_executable (eressea ${LIB_SRCS} ${ERESSEA_SRCS})
target_link_libraries (eressea tolua xml2 ncurses lua5.1 pthread)
if (WIN32)
else (WIN32)
  target_link_libraries (eressea tolua xml2 ncurses lua5.1 pthread)
endif (WIN32)
