SUBDIRS =

LIBRARY = libe-modules.a
BINARY =

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

include $(ERESSEA_SRC)/Makefile.include

## more definitions

INCLUDES  += -I../gamecode -I../kernel -I../util -I..

# library:
$(BUILD_DIR)/$(LIBRARY):: $(BUILD_DIR) $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(PUBLISH_DIR)/$(LIBRARY): $(BUILD_DIR)/$(LIBRARY)
	$(INSTALL) $< $@
