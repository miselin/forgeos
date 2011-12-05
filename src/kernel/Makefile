# Set ARCH_CFLAGS and ARCH_DEFINE automatically depending on how ARCH_TARGET is set
ifeq "$(ARCH_TARGET)" "x86"
  ARCH_CFLAGS :=
  ARCH_DEFINE := -DX86
endif

# Set ARCH_SUBTARGET_CFLAGS and ARCH_SUBTARGET_DEFINE automatically depending on how ARCH_SUBTARGET is set
ifeq "$(ARCH_SUBTARGET)" "x86"
  ARCH_SUBTARGET_CFLAGS := -m32
  ARCH_SUBTARGET_DEFINE :=
endif

# And do the same for PLATFORM_CFLAGS and PLATFORM_TARGET
ifeq "$(PLATFORM_TARGET)" "pc"
  PLATFORM_CFLAGS :=
  PLATFORM_DEFINES :=
endif

# Examples on how to set build flags depending on the build environment
ifeq "$(BUILD_ENV)" "debug"
  CFLAGS += -g -O3 -DDEBUG
endif

ifeq "$(BUILD_ENV)" "release"
  CFLAGS += -O3
endif

OBJDIR := $(OBJDIR)/kernel

DIRS := . arch/$(ARCH_TARGET) mach/$(PLATFORM_TARGET) include

INCLUDES := -Iinclude -Iarch/$(ARCH_TARGET)/include -Imach/$(PLATFORM_TARGET)
LIBDIRS :=

ASMFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.s")
SRCFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.c")
HDRFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.h")

OBJFILES := $(abspath $(patsubst %.s,$(OBJDIR)/%.o,$(ASMFILES)) $(patsubst %.c,$(OBJDIR)/%.o,$(SRCFILES)))
DEPFILES := $(abspath $(patsubst %.c,$(OBJDIR)/%.d,$(SRCFILES)))

KERNEL := $(OBJDIR)/kernel

KERNEL_LSCRIPT := arch/$(ARCH_TARGET)/linker-$(ARCH_SUBTARGET).ld

WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
			-Wwrite-strings -Wredundant-decls -Wnested-externs -Winline \
			-Wno-long-long -Wuninitialized -Wconversion

CFLAGS := -std=gnu99 -fno-builtin -nostdinc -nostdlib -ffreestanding

CFLAGS += $(ARCH_CFLAGS) $(ARCH_DEFINE)
CFLAGS += $(ARCH_SUBTARGET_CFLAGS) $(ARCH_SUBTARGET_DEFINE)
CFLAGS += $(PLATFORM_CFLAGS) $(PLATFORM_DEFINES)
CFLAGS += $(WARNINGS)

CFLAGS := $(strip $(CFLAGS))

LDFLAGS := -nostdlib -nostartfiles
ASFLAGS :=

LINT_FLAGS := $(CFLAGS) $(INCLUDES)
LINT_IGNORE := dlmalloc.c

$(KERNEL): $(OBJFILES)
	@echo Linking kernel...
	@$(LD) $(LDFLAGS) $(LIBDIRS) -o $@ -T $(KERNEL_LSCRIPT) $(OBJFILES)

-include $(DEPFILES)

analyse:
	@echo "Kernel Static Analysis:"
	@for f in $(filter-out $(LINT_IGNORE), $(subst ./,,$(SRCFILES))); do \
		$(LINT) $(LINT_FLAGS) -o /dev/null $$f; \
	done
	@echo "End Kernel Static Analysis\n"


$(OBJDIR)/%.o: %.c Makefile
	@echo '  [CC] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

$(OBJDIR)/%.o: %.s Makefile
	@echo '  [AS] $<...'
	@[ ! -d $(dir $@) ] && mkdir -p $(dir $@); \
	$(AS) $(ASFLAGS) $< -o $@