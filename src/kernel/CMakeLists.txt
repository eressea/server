project(kernel C)

SET(_TEST_FILES
alliance.test.c
ally.test.c
attrib.test.c
build.test.c
building.test.c
# callbacks.test.c
calendar.test.c
command.test.c
config.test.c
# connection.test.c
curse.test.c
database.test.c
direction.test.c
equipment.test.c
# event.test.c
faction.test.c
gamedata.test.c
group.test.c
item.test.c
messages.test.c
order.test.c
# pathfinder.test.c
plane.test.c
pool.test.c
race.test.c
region.test.c
# resources.test.c
save.test.c
ship.test.c
skill.test.c
skills.test.c
spellbook.test.c
spell.test.c
# terrain.test.c
unit.test.c
)

SET(_FILES
db/sqlite.c
alliance.c
ally.c
attrib.c
build.c
building.c
callbacks.c
calendar.c
command.c
config.c
connection.c
curse.c
database.c
direction.c
equipment.c
event.c
faction.c
gamedata.c
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
skill.c
skills.c
spellbook.c
spell.c
terrain.c
unit.c
)

SET(VERSION_SRC ${PROJECT_NAME}/version.c PARENT_SCOPE)

FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(KERNEL_SRC ${_SOURCES} PARENT_SCOPE)
FOREACH(_FILE ${_TEST_FILES})
LIST(APPEND _TESTS ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(KERNEL_TESTS ${_TESTS} PARENT_SCOPE)
