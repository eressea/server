
CONVERT_TRIGGERS = 1

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

