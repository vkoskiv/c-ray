BINDIR_test=tests
BIN_test=testrunner
OBJDIR_test=tests/obj
SRCS_test=$(shell find tests/ -name '*.c')

# You can specify a suite by running 'make test suite=vector', for instance

OBJS_LIB_test=$(SRCS_LIB:%.c=$(OBJDIR_test)/%.o)
SRCS_DRIVER_test=$(shell find src/driver -name '*.c' -not -name 'main.c')
OBJS_DRIVER_test=$(SRCS_DRIVER_test:%.c=$(OBJDIR_test)/%.o)
OBJS_COMMON_test=$(SRCS_COMMON:%.c=$(OBJDIR_test)/%.o)

OBJS_test=$(patsubst %.c, $(OBJDIR_test)/%.o, $(SRCS_test)) $(OBJS_LIB_test) $(OBJS_DRIVER_test) $(OBJS_COMMON_test)
CFLAGS_TESTING=-O0 -fsanitize=address,undefined -DCRAY_TESTING
$(OBJS_test): CFLAGS += -I./src/lib/ -I./src/driver/ -I./src/common/ $(CFLAGS_TESTING)
DEPS_test=$(OBJS_test:%.o=%.d)

test: $(BINDIR_test)/$(BIN_test)
	tests/runner.sh $(suite)

$(BINDIR_test)/$(BIN_test): $(OBJS_test)
	@mkdir -p $(@D)
	@echo "LD $@"
	@$(CC) $(LDFLAGS) -fsanitize=address,undefined $(OBJS_test) -o $@ $(LDLIBS)

-include $(DEPS_test)

$(OBJDIR_test)/%.o: %.c
	@mkdir -p $(@D)
	@echo "CC $(CFLAGS_TESTING) $<"
	@$(CC) $(CFLAGS) -MMD -c $< -o $@
$(OBJDIR_test):
	mkdir -p $@

clean_test:
	rm -rf tests/obj tests/testrunner
