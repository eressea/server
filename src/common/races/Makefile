SUBDIRS =

LIBRARY = libe-races.a
BINARY =

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

include $(ERESSEA_SRC)/Makefile.include

## more definitions

INCLUDES  += -I../kernel -I../gamecode -I../util -I..

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

