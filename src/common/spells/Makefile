SUBDIRS =

LIBRARY = libe-spells.a
BINARY =

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

include $(ERESSEA)/Makefile.include

## more definitions

INCLUDES  += -I../util -I../kernel -I../gamecode -I..

# library:
$(BUILD_DIR)/$(LIBRARY):: $(BUILD_DIR) $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	$(INSTALL) $@ $(PUBLISH_DIR)/

$(PUBLISH_DIR)/$(LIBRARY): $(BUILD_DIR)/$(LIBRARY)
	$(INSTALL) $< $@

# executable:
#$(BUILD_DIR)/$(BINARY):: $(OBJECTS)
#	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)
#	$(INSTALL) $@ $(PUBLISH_DIR)/

