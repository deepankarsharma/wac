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

# libc or fooboot
PLATFORM = libc

# WARNING: GPL license implications from using READLINE
USE_READLINE ?=

#CFLAGS ?= -O2 -Wall -Werror -Wextra -MMD -MP
CFLAGS += -O2 -Wall -Werror -MMD -MP -DPLATFORM=1


ifeq (,$(ESP_PLATFORM))
  CC = gcc $(CFLAGS) -std=gnu99 -m32 -g
  OBJ = out/wa.o out/util.o out/platform.o out/thunk.o out/wac.o
endif

#  dependencies
out/util.o: main/util.h
out/wa.o: main/wa.h main/util.h main/platform.h
out/thunk.o: main/wa.h main/thunk.h
out/wa.a: out/util.o out/thunk.o out/platform.o
out/wac: out/wa.a out/wac.o

ifeq (,$(ESP_PLATFORM))

all: out/wac

out/%.a: main/%.o
	ar rcs $@ $^

out/%.o: main/%.c
	$(CC) -c $(filter %.c,$^) -o $@

out/wac : ${(OBJ)}
	$(CC) -rdynamic -Wl,--no-as-needed -o $@ -Wl,--start-group $^ -Wl,--end-group -lm -ldl

clean ::
	rm -f out/* examples_wast/*.wasm

endif




