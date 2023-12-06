
# `make lib` -- Build c-ray as a .so + driver instead of a monolith

LIB=lib/libc-ray.so
BIN_lib=lib/c-ray
OBJDIR_lib=lib/obj_lib
OBJDIR_driver=lib/obj_driver
SRCS_lib=$(shell find src/lib src/common generated/ -name '*.c')
OBJS_lib=$(patsubst %.c, $(OBJDIR_lib)/%.o, $(SRCS_lib))
SRCS_driver=$(shell find src/driver src/common -name '*.c')
OBJS_driver=$(patsubst %.c, $(OBJDIR_driver)/%.o, $(SRCS_driver))

lib: $(BIN_lib)

pylib: wrappers/cray.so

$(OBJDIR_driver)/%.o: %.c $(OBJDIR_driver)
	@mkdir -p '$(@D)'
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR_lib)/%.o: %.c $(OBJDIR_lib)
	@mkdir -p '$(@D)'
	@echo "CC -fPIC $<"
	@$(CC) -DCR_BUILDING_LIB $(CFLAGS) -fvisibility=hidden -c -fPIC $< -o $@
$(OBJDIR_lib): dummy
	mkdir -p $@
$(OBJDIR_driver):
	mkdir -p $@
$(LIB): $(OBJS_lib) $(OBJDIR_lib)
	@echo "LD -fPIC $@"
	@$(CC) -DCR_BUILDING_LIB $(CFLAGS) -fvisibility=hidden $(OBJS_lib) -shared -fPIC -o $@
$(BIN_lib): $(LIB) $(OBJS_driver) $(OBJDIR_driver)
	@echo "LD $@"
	@$(CC) $(CFLAGS) $(OBJS_driver) $(LIB) -o $@ $(LDFLAGS)
wrappers/cray.o: wrappers/cray.c
	@echo "CC -fPIC $@"
	@$(CC) `pkg-config --cflags python3` $(CFLAGS) -o wrappers/cray.o -shared -fPIC wrappers/cray.c
wrappers/cray.so: $(LIB) wrappers/cray.o
	@echo "Building Python module"
	@$(CC) -shared -fPIC wrappers/cray.o $(LIB) -o $@
