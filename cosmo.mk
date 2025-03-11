CC_cosmo=cosmocc
BIN_cosmo=c-ray.com
OBJDIR_cosmo=bin/obj_cosmo

OBJS_LIB_cosmo=$(SRCS_LIB:%.c=$(OBJDIR_cosmo)/%.o)
$(OBJS_LIB_cosmo): CFLAGS += -I./src/lib/
OBJS_DRIVER_cosmo=$(SRCS_DRIVER:%.c=$(OBJDIR_cosmo)/%.o)
$(OBJS_DRIVER_cosmo): CFLAGS += -I./src/driver/
OBJS_COMMON_cosmo=$(SRCS_COMMON:%.c=$(OBJDIR_cosmo)/%.o)
$(OBJS_COMMON_cosmo): CFLAGS += -I./src/common/

OBJS_cosmo := $(OBJS_LIB_cosmo) $(OBJS_DRIVER_cosmo) $(OBJS_COMMON_cosmo)

cosmo: $(BINDIR)/$(BIN_cosmo)

$(OBJDIR_cosmo)/%.o: %.c $(OBJDIR_cosmo)
	@mkdir -p '$(@D)'
	@echo "cosmocc $<"
	@$(CC_cosmo) $(CFLAGS) -c $< -o $@
$(OBJDIR_cosmo):
	mkdir -p $@
$(BINDIR)/$(BIN_cosmo): $(OBJS_cosmo) $(OBJDIR_cosmo)
	@echo "LD $@"
	@$(CC_cosmo) $(LDFLAGS) $(OBJS_cosmo) -o $@ $(LDLIBS)

clean_cosmo:
	rm -rf bin/c-ray.* bin/obj_cosmo
