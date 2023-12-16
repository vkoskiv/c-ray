
# `make lib` -- Build c-ray as a .so + driver instead of a monolith

LIB=lib/libc-ray.so
BIN_lib=lib/c-ray
OBJDIR_lib=lib/obj_lib
OBJDIR_driver=lib/obj_driver
SRCS_lib=$(shell find src/lib src/common generated/ -name '*.c')
OBJS_lib=$(patsubst %.c, $(OBJDIR_lib)/%.o, $(SRCS_lib))
SRCS_driver=$(shell find src/driver src/common -name '*.c')
OBJS_driver=$(patsubst %.c, $(OBJDIR_driver)/%.o, $(SRCS_driver))
SRCS_python=$(shell find bindings -name '*.py')

lib: $(BIN_lib)

pylib: bindings/cray_wrap.so

BLENDER_ROOT=$(HOME)/.config/blender/4.0

blsync: bindings/cray_wrap.so $(SRCS_python)
	mkdir -p $(BLENDER_ROOT)/scripts/addons/c_ray
	cp bindings/c_ray.py $(BLENDER_ROOT)/scripts/addons/c_ray/
	cp bindings/blender_init.py $(BLENDER_ROOT)/scripts/addons/c_ray/__init__.py
	cp bindings/blender_properties.py $(BLENDER_ROOT)/scripts/addons/c_ray/properties.py
	cp bindings/blender_ui.py $(BLENDER_ROOT)/scripts/addons/c_ray/ui.py
	cp -r bindings/nodes $(BLENDER_ROOT)/scripts/addons/c_ray/

# Blender crashes if I swap out the .so, no idea why, so updating the library
# requires a restart of blender and the use of this target.
fullblsync: bindings/cray_wrap.so $(SRCS_python)
	mkdir -p $(BLENDER_ROOT)/scripts/addons/c_ray
	cp bindings/cray_wrap.so $(BLENDER_ROOT)/scripts/addons/c_ray/
	cp bindings/c_ray.py $(BLENDER_ROOT)/scripts/addons/c_ray/
	cp bindings/blender_init.py $(BLENDER_ROOT)/scripts/addons/c_ray/__init__.py
	cp bindings/blender_properties.py $(BLENDER_ROOT)/scripts/addons/c_ray/properties.py
	cp bindings/blender_ui.py $(BLENDER_ROOT)/scripts/addons/c_ray/ui.py
	cp -r bindings/nodes $(BLENDER_ROOT)/scripts/addons/c_ray/

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
$(LIB): $(OBJS_lib)
	@echo "LD -fPIC $@"
	@$(CC) $(CFLAGS) $(OBJS_lib) -shared -o $@
$(BIN_lib): $(LIB) $(OBJS_driver) $(OBJDIR_driver)
	@echo "LD $@"
	@$(CC) $(CFLAGS) $(OBJS_driver) $(LIB) -o $@ $(LDFLAGS)
bindings/cray_wrap.so: $(OBJS_lib) bindings/cray_wrap.c
	@echo "Building Python module"
	@$(CC) -shared $(CFLAGS) -fPIC `pkg-config --cflags python3` bindings/cray_wrap.c $(OBJS_lib) -o $@
