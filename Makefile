CC=cc
CFLAGS=-Wall -Wextra -Wno-missing-field-initializers -std=c99 -D_GNU_SOURCE -O2 -ftree-vectorize -DCRAY_TESTING
LDFLAGS=-lpthread -lm
BIN=bin/c-ray
OBJDIR=bin/obj
SRCS=$(shell find . -name '*.c')
OBJS=$(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))

all: $(BIN)

$(OBJDIR)/%.o: %.c $(OBJDIR)
	@mkdir -p '$(@D)'
	$(info CC $<)
	@$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR):
	mkdir -p $@
$(BIN): $(OBJS) $(OBJDIR)
	$(info LD $@)
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -rf bin/*
