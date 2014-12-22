PROJECT(attributes C)
SET(_TEST_FILES
stealth.test.c
)

SET(_FILES
attributes.c
fleechance.c
follow.c
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
stealth.c
targetregion.c
)
FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(ATTRIBUTES_SRC ${_SOURCES} PARENT_SCOPE)
FOREACH(_FILE ${_TEST_FILES})
    LIST(APPEND _TESTS ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(ATTRIBUTES_TESTS ${_TESTS} PARENT_SCOPE)
