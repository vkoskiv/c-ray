CC=cc
CFLAGS=-Wall -Wextra -Wno-missing-field-initializers -std=c99 -D_GNU_SOURCE -O2 -ftree-vectorize -DCRAY_TESTING
LDFLAGS=-lpthread -lm
BIN=bin/c-ray
OBJDIR=bin/obj
SRCS=$(shell find . -name '*.c' -not -path './CMakeFiles/*' )
OBJS=$(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))

all: $(BIN)

$(OBJDIR)/%.o: %.c $(OBJDIR)
	@mkdir -p '$(@D)'
	$(info CC $<)
	@$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR): dummy
	mkdir -p $@
$(BIN): $(OBJS) $(OBJDIR)
	$(info LD $@)
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# A sneaky target to run a bit of code generation
dummy:
	$(info Generating gitsha1.c)
	$(shell sed "s/@GIT_SHA1@/`git rev-parse --verify HEAD || echo "NoHash" | cut -c 1-8`/g" src/utils/gitsha1.c.in > generated/gitsha1.c)
clean:
	rm -rf bin/*
