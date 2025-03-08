CC_cosmo=cosmocc
BIN_cosmo=bin/c-ray.com
OBJDIR_cosmo=bin/obj_cosmo
OBJS_cosmo=$(patsubst %.c, $(OBJDIR_cosmo)/%.o, $(SRCS))

cosmo: $(BIN_cosmo)

$(OBJDIR_cosmo)/%.o: %.c $(OBJDIR_cosmo)
	@mkdir -p '$(@D)'
	@echo "cosmocc $<"
	@$(CC_cosmo) $(CFLAGS) -c $< -o $@
$(OBJDIR_cosmo):
	mkdir -p $@
$(BIN_cosmo): $(OBJS_cosmo) $(OBJDIR_cosmo)
	@echo "LD $@"
	@$(CC_cosmo) $(LDFLAGS) $(OBJS_cosmo) -o $@ $(LDLIBS)

clean_cosmo:
	rm -rf bin/c-ray.* bin/obj_cosmo
