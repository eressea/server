TARGETS = subdirs

SUBDIRS = \
  common \
  eressea \
  mapper

#  askalon \
#  tools \

ifeq ($(HOST),amber)
  ifeq ($(USER),eressea)
    include Makefile.corwin
  endif
endif

include Makefile.include


## more definitions
