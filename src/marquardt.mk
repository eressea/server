
ifndef ERESSEA
	export ERESSEA=$(PWD)
endif

# CONVERT_TRIGGERS = 1

NCURSES = 1

# Hier definieren, damit nicht '@gcc'
CC      = gcc
AR      = ar
CTAGS   = ctags
CC      = gcc
LD      = gcc
INSTALL = cp

# CFLAGS += -Wshadow

MSG_COMPILE = "Compiling $@"
MSG_SUBDIR  = "Making $@ in $$subdir"


