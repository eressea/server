SUBDIRS =

LIBRARY = libe-util.a
BINARY =

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

include ../Makefile.include

## more definitions

# library:
$(BUILD_DIR)/$(LIBRARY):: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	$(INSTALL) $@ $(PUBLISH_DIR)/
