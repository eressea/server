project(kernel C)

SET(_TEST_FILES
alliance.test.c
build.test.c
config.test.c
group.test.c
faction.test.c
unit.test.c
save.test.c
ship.test.c
spell.test.c
ally.test.c
building.test.c
equipment.test.c
curse.test.c
item.test.c
order.test.c
pool.test.c
race.test.c
spellbook.test.c
curse.test.c
jsonconf.test.c
spy.test.c
)

SET(_FILES
alliance.c
ally.c
build.c
building.c
calendar.c
command.c
config.c
connection.c
curse.c
equipment.c
faction.c
group.c
item.c
messages.c
order.c
pathfinder.c
plane.c
pool.c
race.c
region.c
resources.c
save.c
ship.c
skills.c
spellbook.c
spell.c
teleport.c
terrain.c
unit.c
xmlreader.c
jsonconf.c
)

FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(KERNEL_SRC ${_SOURCES} PARENT_SCOPE)
FOREACH(_FILE ${_TEST_FILES})
    LIST(APPEND _TESTS ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(KERNEL_TESTS ${_TESTS} PARENT_SCOPE)
