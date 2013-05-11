cmake_minimum_required(VERSION 2.6)
project (eressea C)

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts -Wno-long-long")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -DHAVE__BOOL")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

set (ERESSEA_LIBRARY ${PROJECT_NAME} CACHE INTERNAL "Eressea Core Library")
set (ERESSEA_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Eressea Core headers")

add_subdirectory(bindings)

set (ERESSEA_LIBRARIES
  ${CUTEST_LIBRARIES}
  ${CRITBIT_LIBRARIES}
  ${QUICKLIST_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${INIPARSER_LIBRARIES}
  ${LUA_LIBRARIES}
  ${LIBXML2_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${CURSES_LIBRARIES}
  CACHE STRING "Eressea Libraries"
)

include_directories (${ERESSEA_INCLUDE_DIR})
include_directories (${CRITBIT_INCLUDE_DIR})
include_directories (${CRYPTO_INCLUDE_DIR})
include_directories (${QUICKLIST_INCLUDE_DIR})
include_directories (${CUTEST_INCLUDE_DIR})
include_directories (${LUA_INCLUDE_DIR})
include_directories (${TOLUA_INCLUDE_DIR})
include_directories (${LIBXML2_INCLUDE_DIR})
include_directories (${INIPARSER_INCLUDE_DIR})

FILE (GLOB_RECURSE LIB_HDR *.h)

set (TEST_SRC
  tests.c
  tests_test.c
  gamecode/economy_test.c
  gamecode/laws_test.c
  gamecode/market_test.c
  kernel/ally_test.c
  kernel/battle_test.c
  kernel/building_test.c
  kernel/curse_test.c
  kernel/equipment_test.c
  kernel/item_test.c
  kernel/magic_test.c
  kernel/move_test.c
  kernel/pool_test.c
  kernel/reports_test.c
  kernel/ship_test.c
  kernel/spellbook_test.c
  kernel/spell_test.c
  util/base36_test.c
  util/bsdstring_test.c
  util/functions_test.c
  util/umlaut_test.c
)

set (LIB_SRC
  attributes/alliance.c
  attributes/attributes.c
  attributes/fleechance.c
  attributes/follow.c
  attributes/giveitem.c
  attributes/gm.c
  attributes/hate.c
  attributes/iceberg.c
  attributes/key.c
  attributes/matmod.c
  attributes/moved.c
  attributes/movement.c
  attributes/object.c
  attributes/orcification.c
  attributes/otherfaction.c
  attributes/overrideroads.c
  attributes/racename.c
  attributes/raceprefix.c
  attributes/reduceproduction.c
  attributes/targetregion.c
  eressea.c
  gamecode/archetype.c
  gamecode/creation.c
  gamecode/creport.c
  gamecode/economy.c
  gamecode/give.c
  gamecode/items.c
  gamecode/laws.c
  gamecode/market.c
  gamecode/monster.c
  gamecode/randenc.c
  gamecode/report.c
  gamecode/spy.c
  gamecode/study.c
  gamecode/summary.c
  gamecode/xmlreport.c
  gmtool.c
  items/artrewards.c
  items/demonseye.c
  items/itemtypes.c
  items/phoenixcompass.c
  items/seed.c
  items/speedsail.c
  items/weapons.c
  items/xerewards.c
  kernel/alchemy.c
  kernel/alliance.c
  kernel/ally.c
  kernel/battle.c
  kernel/binarystore.c
  kernel/build.c
  kernel/building.c
  kernel/calendar.c
  kernel/command.c
  kernel/config.c
  kernel/connection.c
  kernel/curse.c
  kernel/equipment.c
  kernel/faction.c
  kernel/group.c
  kernel/item.c
  kernel/magic.c
  kernel/message.c
  kernel/move.c
  kernel/names.c
  kernel/order.c
  kernel/pathfinder.c
  kernel/plane.c
  kernel/player.c
  kernel/pool.c
  kernel/race.c
  kernel/region.c
  kernel/reports.c
  kernel/resources.c
  kernel/save.c
  kernel/ship.c
  kernel/skill.c
  kernel/spellbook.c
  kernel/spell.c
  kernel/sqlite.c
  kernel/teleport.c
  kernel/terrain.c
  kernel/textstore.c
  kernel/unit.c
  kernel/xmlreader.c
  modules/arena.c
  modules/autoseed.c
  modules/dungeon.c
  modules/gmcmd.c
  modules/museum.c
  modules/score.c
  modules/weather.c
  modules/wormhole.c
  modules/xmas.c
  triggers/changefaction.c
  triggers/changerace.c
  triggers/clonedied.c
  triggers/createcurse.c
  triggers/createunit.c
  triggers/gate.c
  triggers/giveitem.c
  triggers/killunit.c
  triggers/removecurse.c
  triggers/shock.c
  triggers/timeout.c
  triggers/triggers.c
  triggers/unguard.c
  triggers/unitmessage.c
  util/attrib.c
  util/base36.c
  util/bsdstring.c
  util/console.c
  util/crmessage.c
  util/dice.c
  util/event.c
  util/filereader.c
  util/functions.c
  util/goodies.c
  util/language.c
  util/listbox.c
  util/lists.c
  util/log.c
  util/message.c
  util/nrmessage.c
  util/os.c
  util/parser.c
  util/rand.c
  util/resolve.c
  util/sql.c
  util/strings.c
  util/translation.c
  util/umlaut.c
  util/unicode.c
  util/xml.c
)

add_library(${ERESSEA_LIBRARY} ${LIB_SRC} ${BIND_SRC} ${LIB_HDR})
target_link_libraries(${ERESSEA_LIBRARY} ${ERESSEA_LIBRARIES})

add_executable(${PROJECT_NAME}_test ${TEST_SRC})
target_link_libraries(${PROJECT_NAME}_test ${ERESSEA_LIBRARY})

add_test(${PROJECT_NAME} ${PROJECT_NAME}_test)
