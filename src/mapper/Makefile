SUBDIRS =

LIBRARY = 
BINARY = mapper

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

include ../Makefile.include

## more definitions

INCLUDES  += \
	-I../util \
	-I../kernel \
	-I../spells \
	-I../items \
	-I../triggers \
	-I../modules \
	-I../races \
	-I../attributes

LIBS      += \
	-le-modules \
	-le-kernel \
	-le-items \
	-le-spells \
	-le-triggers \
	-le-races \
	-le-attributes \
	-le-util \
	-lcurses \
	-lm

# executable:
$(BUILD_DIR)/$(BINARY):: $(BUILD_DIR) $(OBJECTS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS)
	$(INSTALL) $@ $(PUBLISH_DIR)/

