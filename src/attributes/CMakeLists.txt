PROJECT(attributes C)
SET(_FILES
alliance.c
attributes.c
fleechance.c
follow.c
gm.c
hate.c
iceberg.c
key.c
matmod.c
moved.c
movement.c
object.c
orcification.c
otherfaction.c
overrideroads.c
racename.c
raceprefix.c
reduceproduction.c
targetregion.c
)
FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(ATTRIBUTES_SRC ${_SOURCES} PARENT_SCOPE)
    
