SUBDIRS =

LIBRARY = 
BINARY = mapper

include ../Makefile.include

SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

## more definitions

INCLUDES  += \
	-I../common/util \
	-I../common/kernel \
	-I../common/spells \
	-I../common \
	-I..

LIBS      += \
	-L$(PUBLISH_DIR) \
	-le-triggers \
	-le-modules \
	-le-kernel \
	-le-items \
	-le-spells \
	-le-races \
	-le-attributes \
	-le-util \
	-le-modules \
	-lm

ifeq ($(NCURSES), 1)
 LIBS += -lncurses
else
 LIBS += -lcurses
endif

##
## conversion code for old triggers - should only be used for free eressea,
## and only for old datafiles.
##
ifeq ($(CONVERT_TRIGGERS), 1)
 SUBDIRS += old
 LIBRARIES += $(PUBLISH_DIR)/libe-compat.a
 LIBS += -le-compat
endif

# executable:
$(BUILD_DIR)/$(BINARY):: $(BUILD_DIR) $(OBJECTS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS)
	$(INSTALL) $@ $(PUBLISH_DIR)/

$(PUBLISH_DIR)/$(BINARY): $(BUILD_DIR)/$(BINARY)
	$(INSTALL) $< $@
