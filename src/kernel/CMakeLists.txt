cmake_minimum_required(VERSION 2.6)
project(kernel C)

SET(_TEST_FILES
alliance.test.c
ally.test.c
build.test.c
building.test.c
# callbacks.test.c
command.test.c
config.test.c
# connection.test.c
curse.test.c
database.test.c
equipment.test.c
faction.test.c
group.test.c
item.test.c
messages.test.c
order.test.c
orderdb.test.c
# pathfinder.test.c
plane.test.c
pool.test.c
race.test.c
region.test.c
# resources.test.c
save.test.c
ship.test.c
skills.test.c
spellbook.test.c
spell.test.c
# terrain.test.c
unit.test.c
)

SET(_DBFILES db/critbit.c)

IF(DB_FOUND)
SET(_DBFILES db/berkeley.c)
ENDIF(DB_FOUND)

IF(SQLITE3_FOUND)
SET(_DBFILES db/sqlite.c)
ENDIF(SQLITE3_FOUND)

SET(_FILES
${_DBFILES}
alliance.c
ally.c
build.c
building.c
callbacks.c
command.c
config.c
connection.c
curse.c
database.c
equipment.c
faction.c
group.c
item.c
messages.c
order.c
orderdb.c
pathfinder.c
plane.c
pool.c
race.c
region.c
resources.c
rules.c
save.c
ship.c
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
