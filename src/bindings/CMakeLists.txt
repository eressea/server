cmake_minimum_required(VERSION 2.6)
project (bindings C)

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

set (_FILES
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
bind_monsters.c
bind_region.c
bind_ship.c
bind_sqlite.c
bind_storage.c
bind_unit.c
helpers.c
)

FOREACH(_FILE ${_FILES})
    if(EXISTS ${_FILE})
        LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
    else()
    	message(WARNING "File \"${_FILE}\" does not exist but is listed as source.")
    endif()
ENDFOREACH(_FILE)
SET(BINDINGS_SRC ${_SOURCES} PARENT_SCOPE)
