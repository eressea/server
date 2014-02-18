cmake_minimum_required(VERSION 2.6)
project (server C)

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts -Wno-long-long")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DHAVE__BOOL")
ELSEIF(MSVC)
ELSE(CMAKE_COMPILER_IS_GNUCC)
    MESSAGE(STATUS "Unknown compiler ${CMAKE_C_COMPILER_ID}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

find_package (Lua 5 REQUIRED)
find_package (ToLua REQUIRED)
find_package (SQLite3 REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CRITBIT_INCLUDE_DIR})
include_directories (${STORAGE_INCLUDE_DIR})
include_directories (${CRYPTO_INCLUDE_DIR})
include_directories (${QUICKLIST_INCLUDE_DIR})
include_directories (${CUTEST_INCLUDE_DIR})
include_directories (${LUA_INCLUDE_DIR})
include_directories (${LIBXML2_INCLUDE_DIR})
include_directories (${BSON_INCLUDE_DIR})
include_directories (${INIPARSER_INCLUDE_DIR})
include_directories (${CURSES_INCLUDE_DIR})

add_subdirectory(util)
add_subdirectory(kernel)
add_subdirectory(items)
add_subdirectory(attributes)
add_subdirectory(bindings)
add_subdirectory(spells)
add_subdirectory(triggers)
add_subdirectory(modules)
add_subdirectory(races)

set(TESTS
  laws_test.c
  economy_test.c
  market_test.c
)

set (SERVER_SRC
  eressea.c
  archetype.c
  creation.c
  creport.c
  economy.c
  give.c
  items.c
  laws.c
  market.c
  monster.c
  randenc.c
  report.c
  spy.c
  study.c
  summary.c
  xmlreport.c
  gmtool.c
  monsters.c
  ${BINDINGS_SRC}
  ${SPELLS_SRC}
  ${RACES_SRC}
  ${ITEMS_SRC}
  ${MODULES_SRC}
  ${TRIGGERS_SRC}
  ${ATTRIBUTES_SRC}
  ${KERNEL_SRC}
  ${UTIL_SRC}
)

add_executable(eressea ${SERVER_SRC} main.c)
target_link_libraries(eressea
  ${LUA_LIBRARIES}
  ${TOLUA_LIBRARIES}
  ${QUICKLIST_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${STORAGE_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${CRITBIT_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${CURSES_LIBRARIES}
  ${INIPARSER_LIBRARIES}
  )

set(SERVER_TEST_SRC
  test_eressea.c
  tests.c
  tests_test.c
  economy_test.c
  market_test.c
  laws_test.c
  ${UTIL_TESTS}
  ${KERNEL_TESTS}
)

add_executable(test_eressea ${SERVER_SRC} ${SERVER_TEST_SRC})
target_link_libraries(test_eressea ${CUTEST_LIBRARIES})
target_link_libraries(test_eressea
  ${LUA_LIBRARIES}
  ${TOLUA_LIBRARIES}
  ${QUICKLIST_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${STORAGE_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${CRITBIT_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${CURSES_LIBRARIES}
  ${INIPARSER_LIBRARIES}
  )

add_test(server test_eressea)
add_test(NAME E3 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/game-e3 COMMAND $<TARGET_FILE:eressea> -e run_tests)
add_test(NAME E2 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/game-e2 COMMAND $<TARGET_FILE:eressea> -e run_tests)
