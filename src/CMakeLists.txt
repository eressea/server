cmake_minimum_required(VERSION 3.13)
project (server C)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_COMPILER_IS_GNUCC)
  add_compile_options(-Wvla)
endif()
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wsign-compare -Wall -Werror -Wno-switch -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts -Wno-long-long)
elseif(MSVC)
  add_compile_options(/WX /MP /FC)
#  set(EXTRA_C_FLAGS /WX /MP /D_CRT_SECURE_NO_WARNINGS /D_USE_MATH_DEFINES)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrt.lib")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libc.lib /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcd.lib /NODEFAULTLIB:libcmtd.lib /NODEFAULTLIB:msvcrtd.lib")
else()
    message(STATUS "unknown compiler ${CMAKE_C_COMPILER_ID}")
endif()

if(CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wtautological-compare -Weverything")
    message(STATUS "compiler is clang: ${CMAKE_C_COMPILER_ID}")
elseif(CMAKE_COMPILER_IS_GCC)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
        OUTPUT_VARIABLE GCC_VERSION)
    if (GCC_VERSION VERSION_GREATER 4.9)
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfloat-conversion")
    endif()
endif(CMAKE_COMPILER_IS_CLANG)

string(REGEX REPLACE "/W[3|4]" "/w" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

add_subdirectory(util)
add_subdirectory(kernel)
add_subdirectory(items)
add_subdirectory(attributes)
add_subdirectory(spells)
add_subdirectory(triggers)
add_subdirectory(modules)
add_subdirectory(races)

if (TOLUA_EXECUTABLE)
  set (_TOLUA_FILES
    log.pkg.c
    locale.pkg.c
    config.pkg.c
    process.pkg.c
    game.pkg.c
    eressea.pkg.c
    settings.pkg.c
  )

  macro(TOLUA_BINDING _NAME _FILES)
    set(_PKGFILE ${_NAME}.pkg)
    set(_SRCFILE ${_PKGFILE}.c)
    add_custom_command(
      OUTPUT ${_SRCFILE}
    DEPENDS ${_FILES}
    MAIN_DEPENDENCY ${_PKGFILE}
    COMMAND ${TOLUA_EXECUTABLE}
    ARGS -n ${_NAME} -o ${_SRCFILE} ${CMAKE_CURRENT_SOURCE_DIR}/${_PKGFILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endmacro(TOLUA_BINDING)

TOLUA_BINDING(log util/log.h)
TOLUA_BINDING(locale bind_locale.h)
TOLUA_BINDING(config bind_config.h)
TOLUA_BINDING(process bind_process.h)
TOLUA_BINDING(game bind_eressea.h config.h)
TOLUA_BINDING(eressea bind_eressea.h)
TOLUA_BINDING(settings kernel/config.h)

FOREACH(_FILE ${_TOLUA_FILES})
    LIST(APPEND _SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${_FILE})
ENDFOREACH(_FILE)
SET(TOLUA_SRC ${_SOURCES})
else(TOLUA_EXECUTABLE)
SET(TOLUA_SRC ${_TOLUA_FILES})
endif(TOLUA_EXECUTABLE)

set (PARSER_SRC
  ${DB_SRC}
  ${UTIL_SRC}
)

set (ERESSEA_SRC
  ${TOLUA_SRC}
  vortex.c
  alchemy.c
  automate.c
  battle.c
  chaos.c
  contact.c
  creport.c
  defaults.c
  donations.c
  recruit.c
  economy.c
  eressea.c
  exparse.c
  gamedb.c
  give.c
  guard.c
  items.c
  json.c
  jsonconf.c
  laws.c
  lighthouse.c
  magic.c
  market.c
  monsters.c
  morale.c
  move.c
  names.c
  orderfile.c
  piracy.c
  prefix.c
  randenc.c
  renumber.c
  report.c
  reports.c
  sort.c
  spells.c
  spy.c
  steal.c
  study.c
  summary.c
  travelthru.c
  teleport.c
  upkeep.c
  volcano.c
  wormhole.c
  ${SPELLS_SRC}
  ${RACES_SRC}
  ${ITEMS_SRC}
  ${MODULES_SRC}
  ${TRIGGERS_SRC}
  ${ATTRIBUTES_SRC}
  ${KERNEL_SRC}
  )

set(CHECK_SRC
  checker.c
  )

if (LUA_FOUND)
set(SERVER_SRC
  bind_building.c
  bind_eressea.c
  bind_faction.c
  bind_gmtool.c
  bind_locale.c
  bind_message.c
  bind_monsters.c
  bind_order.c
  bind_process.c
  bind_region.c
  bind_ship.c
  bind_storage.c
  bind_tolua.c
  bind_unit.c
  bindings.c
  crimport.c
  console.c
  helpers.c
)
else(LUA_FOUND)
set(SERVER_SRC
  processing.c
)
endif(LUA_FOUND)

if (CURSES_FOUND)
set (SERVER_SRC ${SERVER_SRC}
  bind_config.c
  gmtool.c
  listbox.c
)
endif(CURSES_FOUND)

set (SERVER_SRC ${SERVER_SRC}
  main.c
  signals.c
)

add_library(version STATIC ${VERSION_SRC})
if(DEFINED ERESSEA_VERSION)
  target_compile_definitions(version PRIVATE ERESSEA_VERSION="${ERESSEA_VERSION}")
endif()

if(DEFINED ERESSEA_BUILDNO)
  target_compile_definitions(version PRIVATE ERESSEA_BUILDNO="${ERESSEA_BUILDNO}")
endif()
add_library(parser ${PARSER_SRC})

target_include_directories (parser PUBLIC ${CLIBS_INCLUDE_DIR})
target_include_directories (parser PUBLIC ${INIPARSER_INCLUDE_DIR})
target_include_directories (parser PUBLIC ${STORAGE_INCLUDE_DIR})
target_include_directories (parser PUBLIC ${UTF8PROC_INCLUDE_DIR})

target_link_libraries(parser
  ${CLIBS_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${UTF8PROC_LIBRARY}
  )

add_executable(checker ${CHECK_SRC})
target_link_libraries(checker parser)

if (HAVE_LIBBSD)
  set (EXTRA_LIBS ${EXTRA_LIBS} bsd)
endif (HAVE_LIBBSD)
if (HAVE_LIBM)
  set (EXTRA_LIBS ${EXTRA_LIBS} m)
endif (HAVE_LIBM)

add_library(game ${ERESSEA_SRC})
target_include_directories (game PUBLIC ${INIPARSER_INCLUDE_DIR})
target_include_directories (game PUBLIC ${CRPAT_INCLUDE_DIR})
target_link_libraries(game ${EXTRA_LIBS} parser version)

add_executable(eressea ${SERVER_SRC})
target_link_libraries(eressea
  game
  ${STORAGE_LIBRARIES}
  ${CRPAT_LIBRARIES}
  ${CJSON_LIBRARY}
  ${INIPARSER_LIBRARY}
)

if (LUA_FOUND)
target_compile_definitions(game PRIVATE HAVE_LUA)
target_compile_definitions(eressea PRIVATE HAVE_LUA)
target_include_directories (game PUBLIC ${TOLUA_INCLUDE_DIR})
target_include_directories (game PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(eressea
  ${TOLUA_LIBRARY}
  ${LUA_LIBRARIES}
)

add_test(NAME lua-core
  COMMAND eressea -v1 ../scripts/run-tests.lua
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
set_property(TEST lua-core PROPERTY RUN_SERIAL)
add_test(NAME lua-e2
  COMMAND eressea -v1 ../scripts/run-tests-e2.lua
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
set_property(TEST lua-e2 PROPERTY RUN_SERIAL)
add_test(NAME lua-e3
  COMMAND eressea -v1 ../scripts/run-tests-e3.lua
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests)
set_property(TEST lua-e3 PROPERTY RUN_SERIAL)
set_tests_properties(lua-core lua-e2 lua-e3 PROPERTIES LABELS "lua;ci-only")

find_program(LUAROCKS_EXECUTABLE luarocks)
if(LUAROCKS_EXECUTABLE)
execute_process(COMMAND ${LUAROCKS_EXECUTABLE} path --lr-path OUTPUT_VARIABLE LR_PATH)
execute_process(COMMAND ${LUAROCKS_EXECUTABLE} path --lr-cpath OUTPUT_VARIABLE LR_CPATH)
string(REPLACE ";" "\;" LUA_PATH "${CMAKE_SOURCE_DIR}/scripts/?.lua;${LR_PATH}")
string(REPLACE ";" "\;" LUA_CPATH "${LR_CPATH}")
set_tests_properties(lua-core lua-e2 lua-e3 PROPERTIES ENVIRONMENT "LUA_PATH=${LUA_PATH};LUA_CPATH=${LUA_CPATH}")
endif(LUAROCKS_EXECUTABLE)

endif (LUA_FOUND)

set(TESTS_SRC
  alchemy.test.c
  automate.test.c
  battle.test.c
  contact.test.c
  creport.test.c
  defaults.test.c
  donations.test.c
  economy.test.c
  give.test.c
  guard.test.c
  json.test.c
  jsonconf.test.c
  laws.test.c
  lighthouse.test.c
  magic.test.c
  magicresistance.test.c
  market.test.c
  monsters.test.c
  move.test.c
  names.test.c
  orderfile.test.c
  piracy.test.c
  prefix.test.c
  recruit.test.c
  renumber.test.c
  report.test.c
  reports.test.c
  sort.test.c
  spells.test.c
  spy.test.c
  steal.test.c
  study.test.c
  summary.test.c
  teleport.test.c
  test_eressea.c
  tests.c
  tests.test.c
  travelthru.test.c
  upkeep.test.c
  volcano.test.c
  vortex.test.c
  wormhole.test.c
  spells/combatspells.test.c
  spells/flyingship.test.c
  spells/charming.test.c
  spells/regioncurse.test.c
  triggers/shock.test.c
  ${ATTRIBUTES_TESTS}
  ${UTIL_TESTS}
  ${KERNEL_TESTS}
  ${ITEMS_TESTS}
)

add_executable(test_eressea ${TESTS_SRC})
target_link_libraries(test_eressea
  game
  cutest
  ${LUA_LIBRARIES}
  ${CLIBS_LIBRARIES}
  ${STORAGE_LIBRARIES}
  ${CJSON_LIBRARY}
  ${INIPARSER_LIBRARY}
  )

set_target_properties(test_eressea eressea PROPERTIES C_STANDARD 99)

find_program(IWYU_PATH NAMES include-what-you-use iwyu)
if (IWYU_PATH)
set_property(TARGET eressea PROPERTY C_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
set_property(TARGET test_eressea PROPERTY C_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
endif (IWYU_PATH)

add_test(NAME unit-tests COMMAND test_eressea)

install(TARGETS eressea DESTINATION "bin")

if (HAVE_EXECINFO_H AND HAVE_SIGNAL_H)
  add_compile_definitions(HAVE_BACKTRACE)
endif ()

if (HAVE_LIBBSD)
  add_compile_definitions(HAVE_LIBBSD)
endif (HAVE_LIBBSD)

if (HAVE_STRLCAT)
  add_compile_definitions(HAVE_BSDSTRING)
endif (HAVE_STRLCAT)

if (HAVE_STRDUP)
  add_compile_definitions(HAVE_STRDUP)
endif(HAVE_STRDUP)

if (HAVE_LIBBSD)
target_link_libraries(parser bsd)
endif (HAVE_LIBBSD)

target_include_directories (game PUBLIC ${CJSON_INCLUDE_DIR})
target_include_directories (game PUBLIC ${INIPARSER_INCLUDE_DIR})
target_include_directories (game PUBLIC ${UTF8PROC_INCLUDE_DIR})

target_include_directories (game PRIVATE ${SQLITE3_INCLUDE_DIR})
target_link_libraries(eressea ${SQLITE3_LIBRARY})
target_link_libraries(test_eressea ${SQLITE3_LIBRARY})

if (READLINE_FOUND)
target_include_directories (eressea PRIVATE ${READLINE_INCLUDE_DIR})
target_link_libraries(eressea ${READLINE_LIBRARY})
target_compile_definitions(eressea PRIVATE HAVE_READLINE)
endif (READLINE_FOUND)

if (CURSES_FOUND)
target_include_directories (eressea PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(eressea ${CURSES_LIBRARY})
target_compile_definitions(eressea PRIVATE HAVE_CURSES)
endif(CURSES_FOUND)

if (EXPAT_FOUND)
target_include_directories (game PRIVATE ${EXPAT_INCLUDE_DIRS})
target_link_libraries(eressea ${EXPAT_LIBRARIES})
target_link_libraries(test_eressea ${EXPAT_LIBRARIES})
endif (EXPAT_FOUND)
