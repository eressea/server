SUBDIRS =

LIBRARY = 
BINARY = eressea

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

include ../Makefile.include

## more definitions

INCLUDES  += \
	-I../util \
	-I../kernel \
	-I../items \
	-I../spells \
	-I../triggers \
	-I../gamecode \
	-I../races \
	-I../attributes

LIBS      += \
	-le-triggers \
	-le-modules \
	-le-gamecode \
	-le-kernel \
	-le-items \
	-le-spells \
	-le-races \
	-le-attributes \
	-le-util \
	-lm

# executable:
$(BUILD_DIR)/$(BINARY):: $(BUILD_DIR) $(OBJECTS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS)
	$(INSTALL) $@ $(PUBLISH_DIR)/

