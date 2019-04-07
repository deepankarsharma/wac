#
# Per ESP-IF convention, the  Makefile must reside in the priject base directory
# but all c/h source code files are in 'component' directories, of whihc
# 'main' component has a special meaning.
#

PROJECT_NAME := wace

COMPONENT_DIRS := main

ifneq (,$(ESP_PLATFORM))
include $(IDF_PATH)/make/project.mk
endif

##########################################################
# User configurable build options

# libc or fooboot
PLATFORM = libc

# WARNING: GPL license implications from using READLINE
USE_READLINE ?=


#CFLAGS ?= -O2 -Wall -Werror -Wextra -MMD -MP
CFLAGS += -O2 -Wall -Werror -MMD -MP


##########################################################

ifeq (,$(ESP_PLATFORM))
CC = gcc $(CFLAGS) -std=gnu99 -m32 -g
endif

WA_DEPS = util.o thunk.o

ifeq (libc,$(PLATFORM))
  CFLAGS += -DPLATFORM=1
  ifeq (,$(strip $(USE_READLINE)))
    RL_LIBRARY ?= edit
  else
    RL_LIBRARY ?= readline
    CFLAGS += -DUSE_READLINE=1
  endif
  WAC_LIBS = m dl $(RL_LIBRARY)
  WACE_LIBS = m dl $(RL_LIBRARY)
  ifneq (,$(strip $(USE_SDL)))
    WACE_LIBS += SDL2 SDL2_image GL glut
  endif
else
ifeq (fooboot,$(PLATFORM))
  CFLAGS += -DPLATFORM=2
else
  $(error unknown PLATFORM: $(PLATFORM))
endif
endif


# Basic build rules
all: wac

out/%.a: main/%.o
	ar rcs $@ $^

out/%.o: main/%.c
	$(CC) -c $(filter %.c,$^) -o $@

# Additional dependencies
out/util.o: main/util.h
out/wa.o: main/wa.h main/util.h main/platform.h
out/thunk.o: main/wa.h main/thunk.h
out/wa.a: out/util.o out/thunk.o out/platform.o
out/wac: out/wa.a out/wac.o

#
# Platform
#
ifeq (libc,$(PLATFORM)) # libc Platform
wac:
	$(CC) -rdynamic -Wl,--no-as-needed -o $@ \
	    -Wl,--start-group $^ -Wl,--end-group $(foreach l,$(WAC_LIBS),-l$(l))
endif

ifeq (,$(ESP_PLATFORM))
clean ::
	rm -f out/* \
	    examples_wast/*.wasm
endif




