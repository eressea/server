
ifndef ERESSEA
	export ERESSEA=$(PWD)
endif

# CONVERT_TRIGGERS = 1

# Hier definieren, damit nicht '@gcc'
CC      = gcc -DNEW_RESOURCEGROWTH
AR      = ar
CTAGS   = ctags
CC      = gcc
LD      = gcc
INSTALL = cp

CFLAGS += -DNEW_RESOURCEGROWTH

# CFLAGS += -Wshadow

#		 Ps = 0  -> Normal (default)
#		 Ps = 1  -> Bold
#		 Ps = 4  -> Underlined
#		 Ps = 5  -> Blink (appears as Bold)
#		 Ps = 7  -> Inverse
#		 Ps = 8  -> Invisible (hidden)
#		 Ps = 2 2  -> Normal (neither bold nor faint)
#		 Ps = 2 4  -> Not underlined
#		 Ps = 2 5  -> Steady (not blinking)
#		 Ps = 2 7  -> Positive (not inverse)
#		 Ps = 2 8  -> Visible (not hidden)
#		 Ps = 3 0  -> Set foreground color to Black
#		 Ps = 3 1  -> Set foreground color to Red
#		 Ps = 3 2  -> Set foreground color to Green
#		 Ps = 3 3  -> Set foreground color to Yellow
#		 Ps = 3 4  -> Set foreground color to Blue
#		 Ps = 3 5  -> Set foreground color to Magenta
#		 Ps = 3 6  -> Set foreground color to Cyan
#		 Ps = 3 7  -> Set foreground color to White
#		 Ps = 3 9  -> Set foreground color to default (original)
#		 Ps = 4 0  -> Set background color to Black
#		 Ps = 4 1  -> Set background color to Red
#		 Ps = 4 2  -> Set background color to Green
#		 Ps = 4 3  -> Set background color to Yellow
#		 Ps = 4 4  -> Set background color to Blue
#		 Ps = 4 5  -> Set background color to Magenta
#		 Ps = 4 6  -> Set background color to Cyan
#		 Ps = 4 7  -> Set background color to White
#		 Ps = 4 9  -> Set background color to default (original).

MSG_COMPILE = "[1m[33m\#\#\#\#\# Compiling $@ \#\#\#\#\#[0m"
MSG_SUBDIR  = "[1m[36m\#\#\#\#\# Making $@ in $$subdir \#\#\#\#\#[0m"


