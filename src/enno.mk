#
## enable some new features in the source:
#

#CONVERT_TRIGGERS = 1

CFLAGS += -DNEW_MESSAGES

LD=gcc
AR=ar
CC=@colorgcc
INSTALL=install
MSG_COMPILE = "[1m[32m---> Compiling $@ [0m"
MSG_SUBDIR  = "[1m[37m--> Making $@ in $$subdir [0m"
