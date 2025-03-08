
# `make lib` -- Build c-ray as a .so + driver instead of a monolith

LIB=lib/libc-ray.so
BIN_lib=lib/c-ray
OBJDIR_lib=lib/obj_lib
OBJDIR_driver=lib/obj_driver
SRCS_lib=$(shell find src/lib src/common -name '*.c') generated/gitsha1.c
OBJS_lib=$(patsubst %.c, $(OBJDIR_lib)/%.o, $(SRCS_lib))
SRCS_driver=$(shell find src/driver src/common -name '*.c')
OBJS_driver=$(patsubst %.c, $(OBJDIR_driver)/%.o, $(SRCS_driver))
SRCS_pylib=$(shell find bindings/python/lib -name '*.py')

lib: $(BIN_lib)

pylib: bindings/python/lib/cray_wrap.so $(SRCS_pylib)

BLENDER_VERSION=$(shell blender --version | head -n1 | cut -d ' ' -f 2 | cut -c 1-3)
BLENDER_ROOT=$(HOME)/.config/blender/$(BLENDER_VERSION)

# Only copy .py files, which Blender seems to handle okay.
blsync: $(SRCS_pylib)
	mkdir -p $(BLENDER_ROOT)/scripts/addons/c_ray
	cp $(SRCS_pylib) $(BLENDER_ROOT)/scripts/addons/c_ray/
	cp -r integrations/blender/* $(BLENDER_ROOT)/scripts/addons/c_ray/

# Blender crashes if I swap out the .so, no idea why, so updating the library
# requires a restart of blender and the use of this target.
fullblsync: pylib
	mkdir -p $(BLENDER_ROOT)/scripts/addons/c_ray
	cp -r bindings/python/lib/* $(BLENDER_ROOT)/scripts/addons/c_ray/
	cp -r integrations/blender/* $(BLENDER_ROOT)/scripts/addons/c_ray/

$(OBJDIR_driver)/%.o: %.c $(OBJDIR_driver)
	@mkdir -p '$(@D)'
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@
$(OBJDIR_lib)/%.o: %.c $(OBJDIR_lib)
	@mkdir -p '$(@D)'
	@echo "CC -fPIC $<"
	@$(CC) -DCR_BUILDING_LIB $(CFLAGS) -fvisibility=hidden -c -fPIC $< -o $@
$(OBJDIR_lib):
	mkdir -p $@
$(OBJDIR_driver):
	mkdir -p $@
$(LIB): $(OBJS_lib)
	@echo "LD -fPIC $@"
	@$(CC) $(LDFLAGS) $(OBJS_lib) -shared -o $@
$(BIN_lib): $(LIB) $(OBJS_driver) $(OBJDIR_driver)
	@echo "LD $@"
	@$(CC) $(LDFLAGS) $(OBJS_driver) $(LIB) -o $@ $(LDLIBS)
bindings/python/lib/cray_wrap.so: $(OBJS_lib) bindings/python/cray_wrap.c bindings/python/py_types.c
	@echo "Building CPython module"
	@$(CC) -shared $(CFLAGS) -fPIC `pkg-config --cflags python3` bindings/python/cray_wrap.c bindings/python/py_types.c $(OBJS_lib) -o bindings/python/lib/cray_wrap.so $(LDLIBS)
