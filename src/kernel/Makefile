# Set ARCH_CFLAGS and ARCH_DEFINE automatically depending on how ARCH_TARGET is set
ifeq "$(ARCH_TARGET)" "x86"
  ARCH_CFLAGS :=
  ARCH_ASFLAGS :=
  ARCH_DEFINE := -DX86
  ARCH_LLCFLAGS := -march=x86
endif
ifeq "$(ARCH_TARGET)" "arm"
  ARCH_CFLAGS :=
  ARCH_ASFLAGS :=
  ARCH_DEFINE := -DARM
  ARCH_LLCFLAGS := -march=arm
endif

# Set ARCH_SUBTARGET_CFLAGS and ARCH_SUBTARGET_DEFINE automatically depending on how ARCH_SUBTARGET is set
ifeq "$(ARCH_SUBTARGET)" "x86"
  ARCH_SUBTARGET_CFLAGS := -mtune=i686 -m32  -mno-sse -mno-mmx -mno-sse2 -mno-3dnow
  ARCH_SUBTARGET_ASFLAGS := -m32  -mno-sse -mno-mmx -mno-sse2 -mno-3dnow
  ARCH_SUBTARGET_DEFINE :=
  ARCH_LLCFLAGS += -mcpu=i686 -mattr=-sse,-sse2,-mmx,-3dnow
endif
ifeq "$(ARCH_SUBTARGET)" "armv7"
  ARCH_SUBTARGET_CFLAGS := -march=armv7-a
  ARCH_SUBTARGET_ASFLAGS := -march=armv7-a
  ARCH_SUBTARGET_DEFINE := -DARMV7
endif

# And do the same for PLATFORM_CFLAGS and PLATFORM_TARGET
ifeq "$(PLATFORM_TARGET)" "pc"
  PLATFORM_CFLAGS :=
  PLATFORM_ASFLAGS :=
  PLATFORM_DEFINES :=
endif
ifeq "$(PLATFORM_TARGET)" "omap3"
  PLATFORM_CFLAGS := -mtune=cortex-a8 -mfpu=vfp
  PLATFORM_ASFLAGS := -mtune=cortex-a8 -mfpu=vfp
  PLATFORM_DEFINES := -DOMAP3 -DMACH_REQUIRES_EARLY_DEVINIT
  ARCH_LLCFLAGS += -mcpu=cortex-a8
endif

# Examples on how to set build flags depending on the build environment
ifeq "$(BUILD_ENV)" "debug"
  CFLAGS += -g -O3 -DDEBUG
endif

ifeq "$(BUILD_ENV)" "release"
  CFLAGS += -O3
endif

ifeq "$(BUILD_ENV)" "testing"
  CFLAGS += -g -O3 -D_TESTING
endif

OBJDIR := $(OBJDIR)/kernel

DIRS := . arch/$(ARCH_TARGET) arch/$(ARCH_TARGET)/$(ARCH_SUBTARGET) mach/$(PLATFORM_TARGET) include

INCLUDES := -Iinclude -Iarch/$(ARCH_TARGET)/include -Iarch/$(ARCH_TARGET)/$(ARCH_SUBTARGET)/include -Imach/$(PLATFORM_TARGET) -Imach/$(PLATFORM_TARGET)/include
LIBDIRS :=

ifeq "$(USE_CLANG)" "yes"
  # compiler-rt automatically linked.
  libgcc =
else
  libgcc = $(shell $(CC) -print-file-name=libgcc.a)
endif
LIBS := $(libgcc)

ASMFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.s")
SRCFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.c")
HDRFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.h")

OBJFILES := $(abspath $(patsubst %.s,$(OBJDIR)/%.gcc.o,$(ASMFILES)))
ifeq "$(USE_CLANG)" "yes"
  ASMOBJS := $(OBJFILES)
  OBJFILES := $(patsubst %.c,$(OBJDIR)/%.clang.o,$(SRCFILES))
  DEPFILES := $(abspath $(patsubst %.c,$(OBJDIR)/%.clang.d,$(SRCFILES)))
else
  OBJFILES += $(patsubst %.c,$(OBJDIR)/%.gcc.o,$(SRCFILES))
  DEPFILES := $(abspath $(patsubst %.c,$(OBJDIR)/%.d,$(SRCFILES)))
endif

KERNEL := $(OBJDIR)/kernel

ifeq "$(PLATFORM_TARGET)" "pc"
KERNEL_LSCRIPT := arch/$(ARCH_TARGET)/linker-$(ARCH_SUBTARGET).ld
else
KERNEL_LSCRIPT := mach/$(PLATFORM_TARGET)/linker-$(PLATFORM_TARGET).ld
endif

WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
			-Wwrite-strings -Wredundant-decls -Wnested-externs -Winline \
			-Wno-long-long -Wuninitialized -Wconversion \
			-Wno-empty-translation-unit

CFLAGS += -std=gnu99 -nostdinc -nostdlib -ffreestanding

CFLAGS += $(ARCH_CFLAGS) $(ARCH_DEFINE)
CFLAGS += $(ARCH_SUBTARGET_CFLAGS) $(ARCH_SUBTARGET_DEFINE)
CFLAGS += $(PLATFORM_CFLAGS) $(PLATFORM_DEFINES)
CFLAGS += $(WARNINGS)

ifneq "$(ENABLE_WERROR)" ""
  CFLAGS += -Werror
endif

CFLAGS := $(strip $(CFLAGS))

LDFLAGS := -nostdlib -nostartfiles
ASFLAGS := $(ARCH_ASFLAGS) $(ARCH_SUBTARGET_ASFLAGS) $(PLATFORM_ASFLAGS)

LINT_FLAGS := $(CFLAGS) $(INCLUDES)
LINT_IGNORE := dlmalloc.c

ifeq "$(USE_CLANG)" "yes"
  CFLAGS += -emit-llvm

$(KERNEL): $(OBJFILES) $(ASMOBJS)

	@echo Linking kernel...
	@mkdir -p $(OBJDIR)/llbc
	@$(LLVMLD) -o $(OBJDIR)/llbc/kernel.bc $(OBJFILES)
	@$(LLC) $(ARCH_LLCFLAGS) -o $(OBJDIR)/llbc/kernel.s $(OBJDIR)/llbc/kernel.bc
	@$(CC) $(ASFLAGS) $(CPPFLAGS) -o $(OBJDIR)/llbc/kernel.bo -c $(OBJDIR)/llbc/kernel.s
	@$(LD) $(LDFLAGS) $(LIBDIRS) -o $@ -T $(KERNEL_LSCRIPT) $(OBJDIR)/llbc/kernel.bo $(ASMOBJS) $(LIBS)
	-@$(RM) -r $(OBJDIR)/llbc
else
$(KERNEL): $(OBJFILES) $(ASMOBJS)
	@$(LD) $(LDFLAGS) $(LIBDIRS) -o $@ -T $(KERNEL_LSCRIPT) $(OBJFILES) $(ASMOBJS) $(LIBS)
endif

-include $(DEPFILES)

analyse:
	@echo "Kernel Static Analysis:"
	@for f in $(filter-out $(LINT_IGNORE), $(subst ./,,$(SRCFILES))); do \
		$(LINT) $(LINT_FLAGS) -o /dev/null $$f; \
	done
	@echo "End Kernel Static Analysis\n"

include $(BUILD_SRC)/scripts/rules.mk

