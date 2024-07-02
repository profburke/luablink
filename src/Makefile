SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
ifeq ($(origin .RECIPEPREFIX), undefined)
  $(error This Make does not support .RECIPEPREFIX. Please use GNU Make 4.0 or later)
endif
.RECIPEPREFIX = >

UNAME := $(shell uname -s)

ifeq "$(UNAME)" "Darwin"
        OS = macos
endif

ifeq "$(OS)" "Windows_NT"
        OS = windows
endif

ifeq "$(UNAME)" "Linux"
        OS = linux
endif

ifeq "$(UNAME)" "FreeBSD"
        OS = freebsd
endif

ifeq "$(UNAME)" "OpenBSD"
        OS = openbsd
endif

ifeq "$(UNAME)" "NetBSD"
       OS = netbsd
endif

INSTALL = install
PREFIX = /usr/local
LUAVERSION = 5.4
LIBLOCATION = $(PREFIX)/lib/lua/$(LUAVERSION)
LIBFLAG = -fPIC -shared
LIBS = -lBlink1

# We're not going to use the .dylib extension on macOS because the default Lua
# cpath doesn't look for .dylib files.
LIBTARGET = blink.so

ifeq "$(OS)" "macos"
LIBFLAGS = -dynamiclib -undefined dynamic_lookup
CFLAGS = -Wall -DUSE_HIDAPI
endif


# Targets:

.PHONY: install clean

blink: blink.c
> $(CC) $(LIBFLAGS) $(CFLAGS) -o $(LIBTARGET) $(LIBS) blink.c
ifeq "$(OS)" "macos"
> install_name_tool -change libBlink1.dylib /usr/local/lib/libBlink1.dylib $(LIBTARGET)
endif

install: blink
> $(INSTALL) $(LIBTARGET) $(LIBLOCATION)/$(LIBTARGET)

clean:
> rm -f *.o *.so *~

