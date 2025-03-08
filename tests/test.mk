BIN_test=tests/testrunner
OBJDIR_test=tests/obj
SRCS_test=$(shell find src/lib src/driver src/common tests/ -name '*.c' -not -name 'main.c')
TEST_HDRS=$(shell find tests/ -name '*.h')
OBJS_test=$(patsubst %.c, $(OBJDIR_test)/%.o, $(SRCS_test))
# suite=vector

test: $(BIN_test)
	tests/runner.sh $(suite)

$(OBJDIR_test)/%.o: %.c $(TEST_HDRS) $(OBJDIR_test)
	@mkdir -p '$(@D)'
	@echo "CC $<"
	@$(CC) $(CFLAGS) -fsanitize=address,undefined -DCRAY_TESTING -c $< -o $@
$(OBJDIR_test):
	mkdir -p $@
$(BIN_test): $(OBJS_test) $(OBJDIR_test)
	@echo "LD $@"
	@$(CC) $(LDFLAGS) -fsanitize=address,undefined $(OBJS_test) -o $@ $(LDLIBS)

clean_test:
	rm -rf tests/obj tests/testrunner
