# Rules to use when building

# Compile C code, with GCC.
$(OBJDIR)/%.gcc.o: %.c Makefile
	@echo '  [CC] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Compile C code, with CLANG/LLVM.
$(OBJDIR)/%.clang.o: %.c Makefile
	@echo '  [CC] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CLANG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Assemble code. This is currently done with GCC.
$(OBJDIR)/%.gcc.o: %.S Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.gcc.o: %.s Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) -v $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

# Generate a linker script from the given input, via processing.
$(OBJDIR)/%.ld: %.ld.in
	@echo '  [LDS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CPP) $(ASFLAGS) $(INCLUDES) -x c $< | grep -v '^\#' > $@

