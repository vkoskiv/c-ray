MAKEFLAGS += --no-builtin-rules
CC?=cc
-include ccache.mk
OPT?=-O2
CFLAGS=-I./include/ -I./src/ -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -std=c99 -D_POSIX_C_SOURCE=200112L $(OPT) -g -ftree-vectorize
LDLIBS=-lpthread -lm -ldl

BINDIR=bin
OBJDIR=$(BINDIR)/obj
BIN=$(BINDIR)/c-ray

all: $(BIN)

SRCS_LIB=$(shell find src/lib -name '*.c')
OBJS_LIB=$(SRCS_LIB:%.c=$(OBJDIR)/%.o)
$(OBJS_LIB): CFLAGS += -I./src/lib/

SRCS_DRIVER=$(shell find src/driver -name '*.c')
OBJS_DRIVER=$(SRCS_DRIVER:%.c=$(OBJDIR)/%.o)
$(OBJS_DRIVER): CFLAGS += -I./src/driver/

SRCS_COMMON=$(shell find src/common -name '*.c') generated/gitsha1.c
OBJS_COMMON=$(SRCS_COMMON:%.c=$(OBJDIR)/%.o)
$(OBJS_COMMON): CFLAGS += -I./src/common/

OBJS := $(OBJS_LIB) $(OBJS_DRIVER) $(OBJS_COMMON)

DEPS=$(OBJS:%.o=%.d)

$(BIN): $(OBJS)
	@mkdir -p '$(@D)'
	@echo "LD $@"
	@$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

-include $(DEPS)

$(OBJDIR)/%.o: %.c
	@mkdir -p '$(@D)'
	@echo "CC $<"
	@$(CC) $(CFLAGS) -MMD -c $< -o $@

generated/gitsha1.c: src/common/gitsha1.c.in
	@echo "Generating gitsha1.c"
	$(shell sed "s/@GIT_SHA1@/`git rev-parse --verify HEAD || echo "NoHash" | cut -c 1-8`/g" src/common/gitsha1.c.in > generated/gitsha1.c)

cleanall: clean clean_test clean_lib clean_cosmo

clean:
	rm -rf $(BIN) $(OBJDIR)

enable-git-hooks:
	git config --local include.path ../.gitconfig

include cosmo.mk
include lib.mk
include tests/test.mk
