#
## enable some new features in the source:
#

CONVERT_TRIGGERS = 1

#CFLAGS += -DUSE_GM_COMMANDS -DTEST_GM_COMMANDS

LD=gcc
AR=ar
CC=@colorgcc
INSTALL=install
MSG_COMPILE = "[1m[32m---> Compiling $@ [0m"
MSG_SUBDIR  = "[1m[37m--> Making $@ in $$subdir [0m"
