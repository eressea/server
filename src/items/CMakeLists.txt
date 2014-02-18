PROJECT(items C)
SET(_FILES
artrewards.c
demonseye.c
itemtypes.c
phoenixcompass.c
seed.c
speedsail.c
weapons.c
xerewards.c
)
FOREACH(_FILE ${_FILES})
    LIST(APPEND _SOURCES ${PROJECT_NAME}/${_FILE})
ENDFOREACH(_FILE)
SET(ITEMS_SRC ${_SOURCES} PARENT_SCOPE)
    
    