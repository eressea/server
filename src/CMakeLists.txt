cmake_minimum_required(VERSION 3.13)
project (server C)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})
include_directories (${CRYPTO_INCLUDE_DIR})
include_directories (${CLIBS_INCLUDE_DIR})
include_directories (${STORAGE_INCLUDE_DIR})
include_directories (${TOLUA_INCLUDE_DIR})
include_directories (${LUA_INCLUDE_DIR})
include_directories (${INIPARSER_INCLUDE_DIR})

if (CMAKE_COMPILER_IS_GNUCC)
  add_compile_options(-Wvla)
endif()
if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wsign-compare -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts -Wno-long-long)
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

macro(ADD_LUA_MODULE MODULE_NAME FILES)
  add_library (${MODULE_NAME} SHARED ${FILES})
  set_target_properties(${MODULE_NAME}
    PROPERTIES
    PREFIX ""
  )
endmacro(ADD_LUA_MODULE)

macro(TOLUA_BINDING PKGFILE FILES)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${PKGFILE}.c
    DEPENDS ${FILES} ${PKGFILE}
    COMMAND ${TOLUA_EXECUTABLE}
    ARGS -o ${CMAKE_CURRENT_SOURCE_DIR}/${PKGFILE}.c ${PKGFILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endmacro(TOLUA_BINDING)

TOLUA_BINDING(log.pkg util/log.h)
TOLUA_BINDING(locale.pkg bind_locale.h)
TOLUA_BINDING(config.pkg bind_config.h)
TOLUA_BINDING(process.pkg bind_process.h)
TOLUA_BINDING(game.pkg bind_eressea.h config.h)
TOLUA_BINDING(eressea.pkg bind_eressea.h)
TOLUA_BINDING(settings.pkg kenel/config.h)

set (PARSER_SRC
  ${DB_SRC}
  ${UTIL_SRC}
  )

set (ERESSEA_SRC
  vortex.c
  alchemy.c
  automate.c
  battle.c
  chaos.c
  contact.c
  creport.c
  direction.c
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

set(SERVER_SRC
  bind_building.c
  bind_config.c
  bind_eressea.c
  bind_faction.c
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
  console.c
  helpers.c
  signals.c
  main.c
  )

if (CURSES_FOUND)
set (SERVER_SRC ${SERVER_SRC}
  bind_gmtool.c
  gmtool.c
  listbox.c
)
endif(CURSES_FOUND)

#find_program(IWYU_PATH NAMES include-what-you-use iwyu)
if(IWYU_PATH)
  #  set(C_INCLUDE_WHAT_YOU_USE "${IWYU_PATH} -Xiwyu --no_fwd_decls")
else(IWYU_PATH)
	# message(STATUS "Could not find the program include-what-you-use")
endif()

add_library(version STATIC ${VERSION_SRC})
if(DEFINED ERESSEA_VERSION)
  target_compile_definitions(version PRIVATE ERESSEA_VERSION="${ERESSEA_VERSION}")
endif()

if(DEFINED ERESSEA_BUILDNO)
  target_compile_definitions(version PRIVATE ERESSEA_BUILDNO="${ERESSEA_BUILDNO}")
endif()
add_library(parser ${PARSER_SRC})

target_link_libraries(parser
  ${CLIBS_LIBRARIES}
  ${CRYPTO_LIBRARIES}
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
target_link_libraries(game ${EXTRA_LIBS} parser version)

add_executable(eressea ${SERVER_SRC})
target_link_libraries(eressea
  game
  ${TOLUA_LIBRARIES}
  ${LUA_LIBRARIES}
  ${STORAGE_LIBRARIES}
  ${CJSON_LIBRARIES}
  ${IniParser_LIBRARIES}
)

set(TESTS_SRC
  alchemy.test.c
  automate.test.c
  battle.test.c
  contact.test.c
  creport.test.c
  direction.test.c
  donations.test.c
  economy.test.c
  give.test.c
  guard.test.c
  json.test.c
  jsonconf.test.c
  laws.test.c
  lighthouse.test.c
  magic.test.c
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
  study.test.c
  summary.test.c
  test_eressea.c
  tests.c
  tests.test.c
  travelthru.test.c
  upkeep.test.c
  volcano.test.c
  vortex.test.c
  wormhole.test.c
  spells/flyingship.test.c
  spells/magicresistance.test.c
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
  ${CJSON_LIBRARIES}
  ${IniParser_LIBRARIES}
  )

set_target_properties(test_eressea eressea PROPERTIES C_STANDARD 90)

if (iwyu_path)
set_property(TARGET eressea PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path})
set_property(TARGET test_eressea PROPERTY C_INCLUDE_WHAT_YOU_USE ${iwyu_path})
endif (iwyu_path)

add_test(server test_eressea)
#add_test(NAME E3
#  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/game-e3
#  COMMAND $<TARGET_FILE:eressea> runtests.lua )
#add_test(NAME E2
#	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/game-e2
#  COMMAND $<TARGET_FILE:eressea> runtests.lua )

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

target_include_directories (game PUBLIC ${CJSON_INCLUDE_DIRS})
target_include_directories (game PUBLIC ${IniParser_INCLUDE_DIRS})

if (SQLite3_FOUND)
target_include_directories (game PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(eressea ${SQLite3_LIBRARIES})
target_link_libraries(test_eressea ${SQLite3_LIBRARIES})
target_compile_definitions(game PRIVATE USE_SQLITE)
elseif (DB_FOUND)
#include_directories (${DB_INCLUDE_DIR})
target_link_libraries(eressea ${DB_LIBRARIES})
target_link_libraries(test_eressea ${DB_LIBRARIES})
target_compile_definitions(game PRIVATE USE_DB)
endif(SQLite3_FOUND)

if (READLINE_FOUND)
#include_directories (${READLINE_INCLUDE_DIR})
target_link_libraries(eressea ${READLINE_LIBRARY})
target_compile_definitions(eressea PRIVATE DUSE_READLINE)
endif (READLINE_FOUND)

if (CURSES_FOUND)
target_include_directories (eressea PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(eressea ${CURSES_LIBRARIES})
target_compile_definitions(eressea PRIVATE USE_CURSES)
endif(CURSES_FOUND)

if (EXPAT_FOUND)
target_include_directories (game PRIVATE ${EXPAT_INCLUDE_DIRS})
target_link_libraries(eressea ${EXPAT_LIBRARIES})
target_link_libraries(test_eressea ${EXPAT_LIBRARIES})
endif (EXPAT_FOUND)
