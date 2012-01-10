# Rules to use when building

$(OBJDIR)/%.o: %.c Makefile
	@echo '  [CC] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(OBJDIR)/%.o: %.S Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.s Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(ASFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.ld: %.ld.in
	@echo '  [LDS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CPP) $(ASFLAGS) $(INCLUDES) -x c $< | grep -v '^\#' > $@

