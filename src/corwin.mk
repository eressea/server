
ifndef ERESSEA
	export ERESSEA=$(PWD)
endif

# Hier definieren, damit nicht '@gcc'
CC      = gcc
AR      = ar
CTAGS   = ctags
CC      = gcc
LD      = gcc
INSTALL = cp

CFLAGS += -DHAVE_GETCURX

# ifdef DMALLOC
# 	LIBS   += -ldmalloc
# 	CFLAGS += -DDMALLOC
# endif

