SUBDIRS =
LIBNAME =
EXENAME = eressea

include ../Makefile.include

##
## cheating: we link all attributes/items/etc for all the games. TODO - don't.
##
#SOURCES = $(wildcard *.c)
SOURCES = main.c korrektur.c
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

##
## conversion code for old triggers - should only be used for free eressea,
## and only for old datafiles.
##
ifeq ($(CONVERT_TRIGGERS), 1)
 SUBDIRS += old
 LIBRARIES += $(PUBLISH_DIR)/libe-compat.a
 LIBS += -le-compat
endif

##
## more definitions
##

INCLUDES  += \
	-I../common \
	-I../common/util \
	-I../common/kernel \
	-I../common/gamecode

LIBNAMES      = \
	e-triggers \
	e-modules \
	e-gamecode \
	e-kernel \
	e-items \
	e-spells \
	e-races \
	e-attributes \
	e-util

LIBRARIES += $(LIBNAMES:%=$(PUBLISH_DIR)/lib%.a)

LIBS += $(LIBNAMES:%=-l%) -lm

# executable:

$(BUILD_DIR)/$(BINARY):: subdirs-$(CONFIG) $(BUILD_DIR) $(OBJECTS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS)
	$(INSTALL) $@ $(PUBLISH_DIR)/

$(PUBLISH_DIR)/$(BINARY): $(BUILD_DIR)/$(BINARY)
	$(INSTALL) $< $@
