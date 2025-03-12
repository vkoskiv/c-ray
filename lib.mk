
# `make lib` -- Build c-ray as a .so + driver instead of a monolith

CFLAGS_lib=-DCR_BUILDING_LIB -fvisibility=hidden -fPIC
BINDIR_lib=lib
LIB=$(BINDIR_lib)/libc-ray.so
BIN_lib=$(BINDIR_lib)/c-ray
OBJDIR_lib=$(BINDIR_lib)/obj
OBJS_LIB_lib=$(patsubst %.c, $(OBJDIR_lib)/%.o, $(SRCS_LIB))
OBJS_DRIVER_lib=$(patsubst %.c, $(OBJDIR_lib)/%.o, $(SRCS_DRIVER))
OBJS_COMMON_lib=$(patsubst %.c, $(OBJDIR_lib)/%.o, $(SRCS_COMMON))

SRCS_PYLIB=$(shell find bindings/python/lib -name '*.py')

$(OBJS_LIB_lib): CFLAGS += -I./src/lib/ $(CFLAGS_lib)
$(OBJS_DRIVER_lib): CFLAGS += -I./src/driver/
$(OBJS_COMMON_lib): CFLAGS += -I./src/common/ $(CFLAGS_lib)

lib: $(BIN_lib)

pylib: bindings/python/lib/cray_wrap.so

BLENDER_VERSION=$(shell blender --version | head -n1 | cut -d ' ' -f 2 | cut -c 1-3)
BLENDER_ROOT=$(HOME)/.config/blender/$(BLENDER_VERSION)

# Only copy .py files, which Blender seems to handle okay.
blsync: $(SRCS_PYLIB)
	mkdir -p $(BLENDER_ROOT)/scripts/addons/c_ray
	cp $(SRCS_PYLIB) $(BLENDER_ROOT)/scripts/addons/c_ray/
	cp -r integrations/blender/* $(BLENDER_ROOT)/scripts/addons/c_ray/

# Blender crashes if I swap out the .so, no idea why, so updating the library
# requires a restart of blender and the use of this target.
fullblsync: pylib blsync
	# todo: use var for this path VV
	cp bindings/python/lib/cray_wrap.so $(BLENDER_ROOT)/scripts/addons/c_ray/

$(OBJDIR_lib)/%.o: %.c
	@mkdir -p '$(@D)'
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# FIXME: We're linking LDLIBS to the binary instead of .so because
# transforms.o uses libm and lives under common/ because json_loader.c
# depends on it.

$(LIB): $(OBJS_LIB_lib) $(OBJS_COMMON_lib)
	@mkdir -p '$(@D)'
	@echo "LD -fPIC $@"
	@$(CC) $(LDFLAGS) $^ -shared -o $@
# TODO: rpath?
$(BIN_lib): $(LIB) $(OBJS_DRIVER_lib) $(OBJS_COMMON_lib)
	@mkdir -p '$(@D)'
	@echo "LD $@"
	@$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)
bindings/python/lib/cray_wrap.so: $(OBJS_LIB_lib) $(OBJS_COMMON_lib) bindings/python/cray_wrap.c bindings/python/py_types.c
	@mkdir -p '$(@D)'
	@echo "Building CPython module"
	@$(CC) -shared $(CFLAGS) -fPIC `pkg-config --cflags python3` bindings/python/cray_wrap.c bindings/python/py_types.c $(OBJS_LIB_lib) $(OBJS_COMMON_lib) -o bindings/python/lib/cray_wrap.so $(LDLIBS)

clean_lib:
	rm -rf $(BIN_lib) $(LIB) $(OBJDIR_lib)
	rm -rf bindings/python/lib/cray_wrap.so
