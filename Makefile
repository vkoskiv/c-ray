CC=cc
CFLAGS=-I./include/ -Wall -Wextra -Wno-missing-field-initializers -std=c99 -D_GNU_SOURCE -O2 -ftree-vectorize
LDFLAGS=-lpthread -lm -ldl
BIN=bin/c-ray
OBJDIR=bin/obj
SRCS=$(shell find src/lib src/driver src/common generated/ -name '*.c')
OBJS=$(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))

all: $(BIN)

$(OBJDIR)/%.o: %.c $(OBJDIR)
	@mkdir -p '$(@D)'
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR): dummy
	mkdir -p $@
$(BIN): $(OBJS) $(OBJDIR)
	@echo "LD $@"
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# A sneaky target to run a bit of code generation
dummy:
	@echo "Generating gitsha1.c"
	$(shell sed "s/@GIT_SHA1@/`git rev-parse --verify HEAD || echo "NoHash" | cut -c 1-8`/g" src/common/gitsha1.c.in > generated/gitsha1.c)
clean:
	rm -rf bin/* lib/* wrappers/*o wrappers/*.so

include cosmo.mk
include lib.mk
