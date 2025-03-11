
# `make lib` -- Build c-ray as a .so + driver instead of a monolith

LIB=lib/libc-ray.so
BIN_lib=lib/c-ray
OBJDIR_LIB=lib/obj_lib
OBJDIR_DRIVER=lib/obj_driver
OBJDIR_COMMON=lib/obj_common
OBJS_LIB=$(patsubst %.c, $(OBJDIR_LIB)/%.o, $(SRCS_LIB))
OBJS_DRIVER=$(patsubst %.c, $(OBJDIR_DRIVER)/%.o, $(SRCS_DRIVER))
OBJS_COMMON=$(patsubst %.c, $(OBJDIR_COMMON)/%.o, $(SRCS_COMMON))
SRCS_PYLIB=$(shell find bindings/python/lib -name '*.py')

$(OBJS_LIB): CFLAGS += -I./src/lib/
$(OBJS_DRIVER): CFLAGS += -I./src/driver/
$(OBJS_COMMON): CFLAGS += -I./src/common/

lib: $(BIN_lib)

pylib: bindings/python/lib/cray_wrap.so $(SRCS_PYLIB)

BLENDER_VERSION=$(shell blender --version | head -n1 | cut -d ' ' -f 2 | cut -c 1-3)
BLENDER_ROOT=$(HOME)/.config/blender/$(BLENDER_VERSION)

# Only copy .py files, which Blender seems to handle okay.
blsync: $(SRCS_PYLIB)
	mkdir -p $(BLENDER_ROOT)/scripts/addons/c_ray
	cp $(SRCS_PYLIB) $(BLENDER_ROOT)/scripts/addons/c_ray/
	cp -r integrations/blender/* $(BLENDER_ROOT)/scripts/addons/c_ray/

# Blender crashes if I swap out the .so, no idea why, so updating the library
# requires a restart of blender and the use of this target.
fullblsync: pylib
	mkdir -p $(BLENDER_ROOT)/scripts/addons/c_ray
	cp -r bindings/python/lib/* $(BLENDER_ROOT)/scripts/addons/c_ray/
	cp -r integrations/blender/* $(BLENDER_ROOT)/scripts/addons/c_ray/

$(OBJDIR_DRIVER)/%.o: %.c $(OBJDIR_DRIVER)
	@mkdir -p '$(@D)'
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR_LIB)/%.o: %.c $(OBJDIR_LIB)
	@mkdir -p '$(@D)'
	@echo "CC -fPIC $<"
	@$(CC) -DCR_BUILDING_LIB $(CFLAGS) -fvisibility=hidden -c -fPIC $< -o $@
$(OBJDIR_COMMON)/%.o: %.c $(OBJDIR_COMMON)
	@mkdir -p '$(@D)'
	@echo "CC -fPIC $<"
	@$(CC) -DCR_BUILDING_LIB $(CFLAGS) -fvisibility=hidden -c -fPIC $< -o $@
$(OBJDIR_LIB):
	mkdir -p $@
$(OBJDIR_DRIVER):
	mkdir -p $@
$(OBJDIR_COMMON):
	mkdir -p $@

# FIXME: We're linking LDLIBS to the binary instead of .so because
# transforms.o uses libm and lives under common/ because json_loader.c
# depends on it.

$(LIB): $(OBJS_LIB) ${OBJS_COMMON}
	@echo "LD -fPIC $@"
	@$(CC) $(LDFLAGS) $(OBJS_LIB) $(OBJS_COMMON) -shared -o $@
$(BIN_lib): $(LIB) $(OBJS_DRIVER) $(OBJDIR_DRIVER) $(OBJS_COMMON)
	@echo "LD $@"
	@$(CC) $(LDFLAGS) $(OBJS_DRIVER) $(OBJS_COMMON) $(LIB) -o $@ $(LDLIBS)
bindings/python/lib/cray_wrap.so: $(OBJS_LIB) $(OBJS_COMMON) bindings/python/cray_wrap.c bindings/python/py_types.c
	@echo "Building CPython module"
	@$(CC) -shared $(CFLAGS) -fPIC `pkg-config --cflags python3` bindings/python/cray_wrap.c bindings/python/py_types.c $(OBJS_LIB) $(OBJS_COMMON) -o bindings/python/lib/cray_wrap.so $(LDLIBS)

clean_lib:
	rm -rf lib/*
	rm -rf bindings/python/lib/cray_wrap.so
