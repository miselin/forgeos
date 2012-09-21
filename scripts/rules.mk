# Rules to use when building

# Compile C code, with GCC.
$(OBJDIR)/%.gcc.o: %.c Makefile
	@echo '  [CC] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Compile C code, with CLANG/LLVM.
$(OBJDIR)/%.clang.o: %.c Makefile
	@echo '  [CLANG] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CLANG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Assemble code with LLVM/clang
$(OBJDIR)/%.clang.o: %.s Makefile
	@echo '  [LLVMAS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(LLVMAS) $(CLANG_ASFLAGS) -filetype=obj -assemble $< -o $@

$(OBJDIR)/%.clang.o: %.S Makefile
	@echo '  [LLVMAS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CPP) $(ASFLAGS) $(CPPFLAGS) $< -o $@.s && \
	$(LLVMAS) $(CLANG_ASFLAGS) -filetype=obj -assemble $@.s -o $@

# Assemble code with GCC
$(OBJDIR)/%.gcc.o: %.S Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.gcc.o: %.s Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

# Generate a linker script from the given input, via processing.
$(OBJDIR)/%.ld: %.ld.in
	@echo '  [LDS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CPP) $(ASFLAGS) $(INCLUDES) -x c $< | grep -v '^\#' > $@
