MODULES := common eressea common/triggers common/items common/spells common/races common/attributes common/gamecode common/kernel common/util common/modules

CC = gcc
CFLAGS += -Wall -Wwrite-strings -Wstrict-prototypes \
           -Werror-implicit-function-declaration \
           -Wno-char-subscripts 

# look for include files in
# each of the modules
CFLAGS += -I/cygwin/usr/include
CFLAGS += -I.
CFLAGS += $(patsubst %,-I%,$(MODULES))
# extra libraries if required
LIBS :=
# each module will add to this
SRC :=

# include the description for
# each module
include $(patsubst %,%/module.mk,$(MODULES))
# determine the object files
OBJ := $(patsubst %.c,%.o, $(filter %.c,$(SRC))) \
       $(patsubst %.y,%.o, $(filter %.y,$(SRC)))
# link the program
eressea: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LIBS)

%.o: %.c
	@echo Compiling $@
	@$(CC) $(CFLAGS) -c -o $@ $<

# include the C include
# dependencies
include $(OBJ:.o=.d)
# calculate C include
# dependencies
%.d: %.c
	gcc -MM -MG $(CFLAGS) $< > $@
