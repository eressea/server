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

LIBNAMES      += \
	e-triggers \
	e-modules \
	e-kernel \
	e-items \
	e-spells \
	e-races \
	e-attributes \
	e-util \
	e-modules

LIBS      += \
	-L$(PUBLISH_DIR) \
	-lm

LIBDEPS = $(LIBNAMES:%=../$(BUILD_DIR)/lib%.a)
LIBS   += $(LIBNAMES:%=-l%)

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
$(BUILD_DIR)/$(BINARY):: $(BUILD_DIR) $(OBJECTS) $(LIBDEPS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS)
	$(INSTALL) $@ $(PUBLISH_DIR)/

$(PUBLISH_DIR)/$(BINARY): $(BUILD_DIR)/$(BINARY)
	$(INSTALL) $< $@
