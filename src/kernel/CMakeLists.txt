cmake_minimum_required (VERSION 2.4)
project (kernel)

include_directories (..)
include_directories (${LIBXML2_INCLUDE_DIR})
include_directories (${CRITBIT_INCLUDE_DIR})
include_directories (${QUICKLIST_INCLUDE_DIR})
include_directories (${CUTEST_INCLUDE_DIR})
include_directories (${INIPARSER_INCLUDE_DIR})
include_directories (${CRYPTO_INCLUDE_DIR})

add_library (kernel
  alchemy.c
  alliance.c
  battle.c
  battle_test.c
  binarystore.c
  build.c
  building.c
  building_test.c
  calendar.c
  command.c
  config.c
  connection.c
  curse.c
  curse_test.c
  equipment.c
  equipment_test.c
  faction.c
  group.c
  item.c
  item_test.c
  magic.c
  magic_test.c
  message.c
  move.c
  move_test.c
  names.c
  order.c
  pathfinder.c
  plane.c
  player.c
  pool.c
  pool_test.c
  race.c
  region.c
  reports.c
  reports_test.c
  resources.c
  save.c
  ship.c
  ship_test.c
  skill.c
  spellbook.c
  spellbook_test.c
  spell.c
  spell_test.c
  sqlite.c
  teleport.c
  terrain.c
  textstore.c
  unit.c
  xmlreader.c
)
