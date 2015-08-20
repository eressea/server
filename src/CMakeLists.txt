cmake_minimum_required(VERSION 2.6)
project (server C)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CRITBIT_INCLUDE_DIR})
include_directories (${CJSON_INCLUDE_DIR})
include_directories (${STORAGE_INCLUDE_DIR})
include_directories (${CRYPTO_INCLUDE_DIR})
include_directories (${QUICKLIST_INCLUDE_DIR})
include_directories (${CUTEST_INCLUDE_DIR})
include_directories (${LUA_INCLUDE_DIR})
include_directories (${BSON_INCLUDE_DIR})
include_directories (${INIPARSER_INCLUDE_DIR})

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts -Wno-long-long")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DHAVE__BOOL")
elseif(MSVC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX /MP")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrt.lib")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
ELSE(CMAKE_COMPILER_IS_GNUCC)
    MESSAGE(STATUS "Unknown compiler ${CMAKE_C_COMPILER_ID}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

add_subdirectory(util)
add_subdirectory(kernel)
add_subdirectory(items)
add_subdirectory(attributes)
add_subdirectory(spells)
add_subdirectory(triggers)
add_subdirectory(modules)
add_subdirectory(races)

MACRO(ADD_LUA_MODULE MODULE_NAME FILES)
  ADD_LIBRARY (${MODULE_NAME} SHARED ${FILES})
  SET_TARGET_PROPERTIES(${MODULE_NAME}
    PROPERTIES
    PREFIX ""
  )
ENDMACRO(ADD_LUA_MODULE)

MACRO(TOLUA_BINDING PKGFILE FILES)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${PKGFILE}.c
    DEPENDS ${FILES} ${PKGFILE}
    COMMAND ${TOLUA_EXECUTABLE}
    ARGS -o ${CMAKE_CURRENT_SOURCE_DIR}/${PKGFILE}.c ${PKGFILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
ENDMACRO(TOLUA_BINDING)

IF(NOT MSVC)
TOLUA_BINDING(log.pkg util/log.h)
TOLUA_BINDING(locale.pkg bind_locale.h)
TOLUA_BINDING(config.pkg bind_config.h)
TOLUA_BINDING(process.pkg bind_process.h)
TOLUA_BINDING(game.pkg bind_eressea.h config.h)
TOLUA_BINDING(eressea.pkg bind_eressea.h)
TOLUA_BINDING(settings.pkg bind_settings.h)
ENDIF()

set (ERESSEA_SRC
  move.c
  repair.c
  spells.c
  battle.c
  alchemy.c
  upkeep.c
  vortex.c
  names.c
  lighthouse.c
  reports.c
  eressea.c
  callback.c
  direction.c
  keyword.c
  skill.c
  json.c
  creport.c
  jsreport.c
  economy.c
  give.c
  items.c
  laws.c
  magic.c
  market.c
  morale.c
  monster.c
  randenc.c
  chaos.c
  report.c
  spy.c
  study.c
  summary.c
  monsters.c
  wormhole.c
  ${SPELLS_SRC}
  ${RACES_SRC}
  ${ITEMS_SRC}
  ${MODULES_SRC}
  ${TRIGGERS_SRC}
  ${ATTRIBUTES_SRC}
  ${KERNEL_SRC}
  ${UTIL_SRC}
)

set(SERVER_SRC
  main.c
  building_action.c
  console.c
  helpers.c
  bind_tolua.c
  bind_building.c
  bind_config.c
  bind_locale.c
  bind_eressea.c
  bind_faction.c
  bind_dict.c
  bindings.c
  bind_message.c
  bind_monsters.c
  bind_process.c
  bind_region.c
  bind_settings.c
  bind_ship.c
  bind_storage.c
  bind_unit.c
)

if (SQLITE3_FOUND)
set (SERVER_SRC ${SERVER_SRC}
  sqlite.c
  bind_sqlite.c
)
endif (SQLITE3_FOUND)

if (CURSES_FOUND)
set (SERVER_SRC ${SERVER_SRC}
  gmtool.c
  listbox.c
  bind_gmtool.c
)
endif(CURSES_FOUND)

add_library(game ${ERESSEA_SRC})
add_executable(eressea ${SERVER_SRC})
target_link_libraries(eressea
  game
  ${TOLUA_LIBRARIES}
  ${LUA_LIBRARIES}
  ${QUICKLIST_LIBRARIES}
  ${STORAGE_LIBRARIES}
  ${CRITBIT_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${CJSON_LIBRARIES}
  ${INIPARSER_LIBRARIES}
)

set(TESTS_SRC
  wormhole.test.c
  test_eressea.c
  tests.c
  battle.test.c
  vortex.test.c
  tests.test.c
  reports.test.c
  callback.test.c
  direction.test.c
  economy.test.c
  json.test.c
  keyword.test.c
  give.test.c
  laws.test.c
  magic.test.c
  market.test.c
  move.test.c
  skill.test.c
  spells.test.c
  spy.test.c
  upkeep.test.c
  ${ATTRIBUTES_TESTS}
  ${UTIL_TESTS}
  ${KERNEL_TESTS}
  ${ITEMS_TESTS}
)

add_executable(test_eressea ${TESTS_SRC})
target_link_libraries(test_eressea
  game
  ${CUTEST_LIBRARIES}
  ${LUA_LIBRARIES}
  ${QUICKLIST_LIBRARIES}
  ${STORAGE_LIBRARIES}
  ${CRITBIT_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${CJSON_LIBRARIES}
  ${INIPARSER_LIBRARIES}
  )

add_test(server test_eressea)
#add_test(NAME E3
#  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/game-e3
#  COMMAND $<TARGET_FILE:eressea> runtests.lua )
#add_test(NAME E2
#	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/game-e2
#  COMMAND $<TARGET_FILE:eressea> runtests.lua )

install(TARGETS eressea DESTINATION "bin")

if (SQLITE3_FOUND)
target_link_libraries(eressea ${SQLITE3_LIBRARIES})
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_SQLITE")
endif(SQLITE3_FOUND)

if (CURSES_FOUND)
include_directories (${CURSES_INCLUDE_DIR})
target_link_libraries(eressea ${CURSES_LIBRARIES})
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_CURSES")
endif(CURSES_FOUND)

if (LIBXML2_FOUND)
include_directories (${LIBXML2_INCLUDE_DIR})
target_link_libraries(eressea ${LIBXML2_LIBRARIES})
target_link_libraries(test_eressea ${LIBXML2_LIBRARIES})
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_LIBXML2")
endif (LIBXML2_FOUND)
