SUBDIRS =

LIBRARY = libe-triggers.a
BINARY =

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

include ../Makefile.include

## more definitions

INCLUDES  += -I../util -I../kernel -I../gamecode

# library:
$(BUILD_DIR)/$(LIBRARY):: $(BUILD_DIR) $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	$(INSTALL) $@ $(PUBLISH_DIR)/

# executable:
#$(BUILD_DIR)/$(BINARY):: $(OBJECTS)
#	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
#	$(INSTALL) $@ $(PUBLISH_DIR)/

