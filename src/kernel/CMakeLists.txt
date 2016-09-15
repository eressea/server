project(kernel C)

SET(_TEST_FILES
alliance.test.c
ally.test.c
build.test.c
building.test.c
command.test.c
config.test.c
# connection.test.c
curse.test.c
equipment.test.c
faction.test.c
group.test.c
item.test.c
messages.test.c
order.test.c
# pathdinder.test.c
plane.test.c
pool.test.c
race.test.c
# region.test.c
# resources.test.c
save.test.c
ship.test.c
# skills.test.c
spellbook.test.c
spell.test.c
# terrain.test.c
unit.test.c
jsonconf.test.c
# xmlreader.test.c
)

SET(_FILES
version.c
alliance.c
ally.c
build.c
building.c
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
