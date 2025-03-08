MAKEFLAGS += --no-builtin-rules
CC?=cc
-include ccache.mk
OPT?=-O2
CFLAGS=-I./include/ -Wall -Wextra -Wno-missing-field-initializers -std=c99 -D_POSIX_C_SOURCE=200112L $(OPT) -g -ftree-vectorize
LDLIBS=-lpthread -lm -ldl

BIN=c-ray
BINDIR=bin

all: $(BINDIR)/$(BIN)

SRCS=$(shell find src/lib src/driver src/common -name '*.c') generated/gitsha1.c
OBJS=$(SRCS:%.c=$(BINDIR)/%.o)
DEPS=$(OBJS:%.o=%.d)

$(BINDIR)/$(BIN): $(OBJS)
	@mkdir -p $(@D)
	@echo "LD $@"
	@$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

-include $(DEPS)

$(BINDIR)/%.o: %.c
	@mkdir -p $(@D)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -MMD -c $< -o $@

generated/gitsha1.c: src/common/gitsha1.c.in
	@echo "Generating gitsha1.c"
	$(shell sed "s/@GIT_SHA1@/`git rev-parse --verify HEAD || echo "NoHash" | cut -c 1-8`/g" src/common/gitsha1.c.in > generated/gitsha1.c)

clean: clean_test clean_lib clean_cosmo
	rm -rf bin/c-ray bin/src bin/generated

enable-git-hooks:
	git config --local include.path ../.gitconfig

include cosmo.mk
include lib.mk
include tests/test.mk
