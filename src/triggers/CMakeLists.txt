PROJECT(triggers C)
SET(_FILES
changerace.c
clonedied.c
createcurse.c
createunit.c
gate.c
giveitem.c
killunit.c
shock.c
timeout.c
triggers.c
)
FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(TRIGGERS_SRC ${_SOURCES} PARENT_SCOPE)
