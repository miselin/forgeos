# Rules to use when building

$(OBJDIR)/%.gcc.o: %.c Makefile
	@echo '  [CC] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(OBJDIR)/%.clang.o: %.c Makefile
	@echo '  [CC] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CLANG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(OBJDIR)/%.gcc.o: %.S Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.gcc.o: %.s Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) -v $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.ld: %.ld.in
	@echo '  [LDS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CPP) $(ASFLAGS) $(INCLUDES) -x c $< | grep -v '^\#' > $@

