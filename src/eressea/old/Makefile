SUBDIRS =
LIBNAME = e-compat
EXENAME =

## cheating: we link all attributes/items/etc for all the games. TODO - don't.
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

include $(ERESSEA)/Makefile.include

##
## more definitions
##

INCLUDES  += \
	-I../../common \
	-I../../common/util \
	-I../../common/kernel

LIBRARIES = $(LIBNAMES:%=$(PUBLISH_DIR)/lib%.a)

LIBS += $(LIBNAMES:%=-l%) -lm

# library:
$(BUILD_DIR)/$(LIBRARY):: $(BUILD_DIR) $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	$(INSTALL) $@ $(PUBLISH_DIR)/

