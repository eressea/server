PROJECT(spells C)
SET(_FILES
borders.c
buildingcurse.c
combatspells.c
regioncurse.c
shipcurse.c
unitcurse.c
magicresistance.c
flyingship.c
)
FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(SPELLS_SRC ${_SOURCES} PARENT_SCOPE)
