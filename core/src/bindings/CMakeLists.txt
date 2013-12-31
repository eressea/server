cmake_minimum_required(VERSION 2.6)
project (bindings C)

IF(CMAKE_COMPILER_IS_GNUCC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Werror -Wno-unknown-pragmas -Wstrict-prototypes -Wpointer-arith -Wno-char-subscripts -Wno-long-long")
ELSEIF(MSVC)
ELSE(CMAKE_COMPILER_IS_GNUCC)
    MESSAGE(STATUS "Unknown compiler ${CMAKE_C_COMPILER_ID}")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

set (BINDINGS_LIBRARY ${PROJECT_NAME} CACHE INTERNAL "Eressea Lua Bindings")

find_package (Lua 5 REQUIRED)
find_package (ToLua REQUIRED)

include_directories (${TOLUA_INCLUDE_DIR})

FILE (GLOB_RECURSE BINDINGS_HDR *.h)

MACRO(ADD_LUA_MODULE MODULE_NAME FILES)
  ADD_LIBRARY (${MODULE_NAME} SHARED ${FILES})
  SET_TARGET_PROPERTIES(${MODULE_NAME}
    PROPERTIES
    PREFIX ""
  )
ENDMACRO(ADD_LUA_MODULE)

MACRO(TOLUA_BINDING PKGFILE FILES)
  ADD_CUSTOM_COMMAND(
    OUTPUT ${PKGFILE}.c
    DEPENDS ${FILES} ${PKGFILE}
    COMMAND ${TOLUA_EXECUTABLE}
    ARGS -o ${CMAKE_CURRENT_BINARY_DIR}/${PKGFILE}.c ${PKGFILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
ENDMACRO(TOLUA_BINDING)

TOLUA_BINDING(process.pkg bind_process.h)
TOLUA_BINDING(eressea.pkg bind_eressea.h)
TOLUA_BINDING(settings.pkg bind_settings.h)

set (BINDINGS_SRC
	process.pkg.c
	eressea.pkg.c
	settings.pkg.c
	bind_process.c
	bind_eressea.c
	bind_settings.c
	bind_building.c
	bind_faction.c
	bind_gmtool.c
	bind_hashtable.c
	bindings.c
	bind_message.c
	bind_region.c
	bind_ship.c
	bind_sqlite.c
	bind_storage.c
	bind_unit.c
	helpers.c
)

add_library(${BINDINGS_LIBRARY} ${BINDINGS_SRC} ${BINDINGS_HDR})
target_link_libraries(${BINDINGS_LIBRARY} ${TOLUA_LIBRARIES})
target_link_libraries(${BINDINGS_LIBRARY} ${ERESSEA_LIBRARY})
