cmake_minimum_required(VERSION 2.6)
project (server C)

find_package (Lua51 REQUIRED)
find_package (LibXml2 REQUIRED)
find_package (SQLite3 REQUIRED)
find_package (ToLua REQUIRED)
find_package (Curses REQUIRED)

include_directories (${ERESSEA_INCLUDE_DIR})
include_directories (${CRITBIT_INCLUDE_DIR})
include_directories (${CRYPTO_INCLUDE_DIR})
include_directories (${QUICKLIST_INCLUDE_DIR})
include_directories (${CUTEST_INCLUDE_DIR})
include_directories (${LUA_INCLUDE_DIR})
include_directories (${LIBXML2_INCLUDE_DIR})
include_directories (${BSON_INCLUDE_DIR})
include_directories (${INIPARSER_INCLUDE_DIR})

add_executable(server main.c)
target_link_libraries(server
  ${ERESSEA_LIBRARY}
  ${BINDINGS_LIBRARY}
  ${TOLUA_LIBRARIES}
  ${SQLITE3_LIBRARIES}
)
