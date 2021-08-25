DEFS := -D_FORGEOS=1

# Set ARCH_CFLAGS and ARCH_DEFINE automatically depending on how ARCH_TARGET is set
ifeq "$(ARCH_TARGET)" "x86"
  ARCH_CFLAGS :=
  ARCH_ASFLAGS :=
  ARCH_DEFINE := -DX86=1
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
  CLANG_ASFLAGS := -arch x86 -mcpu=i686

  CLANG_TRIPLE := -ccc-host-triple i686-unknown-none
  ARCH_LLCFLAGS += -mcpu=i686 -mattr=-sse,-sse2,-mmx,-3dnow
endif
ifeq "$(ARCH_SUBTARGET)" "armv7"
  ARCH_SUBTARGET_CFLAGS := -march=armv7-a
  ARCH_SUBTARGET_ASFLAGS := -march=armv7-a
  CLANG_ASFLAGS := -arch arm

  CLANG_TRIPLE := -ccc-host-triple armv7-unknown-gnueabi
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

  CLANG_ASFLAGS += -mcpu=cortex-a8
  ARCH_LLCFLAGS += -mcpu=cortex-a8 -mattr=-neon,+vfp3,+v7
endif

# Examples on how to set build flags depending on the build environment
ifeq "$(BUILD_ENV)" "debug"
  CFLAGS += -g -O3
  DEFS += -DDEBUG=1
endif

ifeq "$(BUILD_ENV)" "release"
  CFLAGS += -O3
endif

ifeq "$(BUILD_ENV)" "testing"
  CFLAGS += -g -O3
  DEFS += -D_TESTING=1
endif

OBJDIR := $(OBJDIR)/kernel

DIRS := . arch/$(ARCH_TARGET) arch/$(ARCH_TARGET)/$(ARCH_SUBTARGET) mach/$(PLATFORM_TARGET) include

INCLUDES := -Iinclude -Iarch/$(ARCH_TARGET)/include -Iarch/$(ARCH_TARGET)/$(ARCH_SUBTARGET)/include -Imach/$(PLATFORM_TARGET) -Imach/$(PLATFORM_TARGET)/include -I$(BUILD_DIR)
LIBDIRS :=

ifeq "$(PLATFORM_TARGET)" "pc"
  DIRS += mach/$(PLATFORM_TARGET)/acpica $(shell find mach/$(PLATFORM_TARGET)/acpica/components -type d)
  INCLUDES += -Imach/$(PLATFORM_TARGET)/acpica/include -Imach/$(PLATFORM_TARGET)/acpica/include/platform
endif

CFLAGS += -include $(BUILD_DIR)/kconfig.h

ifeq "$(USE_CLANG)" "yes"
  # compiler-rt automatically linked.
  # TODO: fix compiler-rt integration
  libgcc = $(shell $(CC) -print-file-name=libgcc.a)

  # Add in the CLANG-specific flags
  CFLAGS += $(CLANG_TRIPLE) -emit-llvm -integrated-as

  # compiler-rt does not build as easily on non-x86, however, so use libgcc for
  # now. Hopefully this won't be an issue for too much longer.
  ifneq "$(ARCH_TARGET)" "x86"
    libgcc = $(shell $(CC) -print-file-name=libgcc.a)

    # compiler-rt still doesn't give __builtin_memset or __builtin_memcpy, they
    # just forward straight on to memcpy/memset.
    DEFS += -DNO_BUILTIN_MEMFUNCS
  endif
else
  libgcc = $(shell $(CC) -print-file-name=libgcc.a)
endif
LIBS := $(libgcc)

ASMFILES := $(abspath $(shell find $(DIRS) -maxdepth 1 -type f -name "*.s"))
SRCFILES := $(abspath $(shell find $(DIRS) -maxdepth 1 -type f -name "*.c"))
HDRFILES := $(abspath $(shell find $(DIRS) -maxdepth 1 -type f -name "*.h"))

OBJFILES := $(patsubst %.s,$(OBJDIR)/%.gcc.o,$(ASMFILES))
ifeq "$(USE_CLANG)" "yes"
  ifeq "$(ARCH_TARGET)" "arm"
    ASMOBJS := $(patsubst %.s,$(OBJDIR)/%.gcc.o,$(ASMFILES))
  else
    # 16-bit codegen is only available on GCC, not llvm-mc. Nice.
    ASMOBJS := $(patsubst %.16bit.s,$(OBJDIR)/%.16bit.gcc.o,$(ASMFILES))
    ASMOBJS := $(patsubst %.s,$(OBJDIR)/%.clang.o,$(ASMOBJS))
  endif
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
			-Wno-long-long -Wuninitialized -Wconversion

CFLAGS += -std=gnu99 -nostdinc -nostdlib -ffreestanding

CFLAGS += $(ARCH_CFLAGS)
CFLAGS += $(ARCH_SUBTARGET_CFLAGS)
CFLAGS += $(PLATFORM_CFLAGS)
CFLAGS += $(WARNINGS)

DEFS += $(ARCH_DEFINE) $(ARCH_SUBTARGET_DEFINE) $(PLATFORM_DEFINES)

ifneq "$(ENABLE_WERROR)" ""
  CFLAGS += -Werror
endif

CFLAGS := $(strip $(CFLAGS))

LDFLAGS := -nostdlib -nostartfiles
ASFLAGS := $(ARCH_ASFLAGS) $(ARCH_SUBTARGET_ASFLAGS) $(PLATFORM_ASFLAGS)

LINT_FLAGS := $(CFLAGS) $(INCLUDES)
LINT_IGNORE := dlmalloc.c

ifeq "$(USE_CLANG)" "yes"
  WARNINGS += -Wno-empty-translation-unit
  LLVM_ASFLAGS := -filetype=obj -assemble

  ifeq "$(ARCH_TARGET)" "arm"
    LLVMAS := $(CC)
    CLANG_ASFLAGS := $(ASFLAGS)
    LLVM_ASFLAGS := -c
  endif

$(KERNEL): $(BUILD_DIR)/kconfig.h $(OBJFILES) $(ASMOBJS)
	@echo Linking kernel...
	@mkdir -p $(OBJDIR)/llbc
	@$(LLVMLD) -o $(OBJDIR)/llbc/kernel.bc $(OBJFILES)
	@$(LLC) -O3 $(ARCH_LLCFLAGS) -o $(OBJDIR)/llbc/kernel.s $(OBJDIR)/llbc/kernel.bc
	@$(LLVMAS) $(CLANG_ASFLAGS) $(CPPFLAGS) -o $(OBJDIR)/llbc/kernel.bo $(LLVM_ASFLAGS) $(OBJDIR)/llbc/kernel.s
	@$(LD) $(LDFLAGS) $(LIBDIRS) -o $@ -T $(KERNEL_LSCRIPT) $(OBJDIR)/llbc/kernel.bo $(ASMOBJS) $(LIBS)
	-@$(RM) -r $(OBJDIR)/llbc
else
$(KERNEL): $(BUILD_DIR)/kconfig.h $(OBJFILES) $(ASMOBJS)
	@echo Linking kernel...
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

$(BUILD_DIR)/kconfig.h: $(REPO_BASE_DIR)/scripts/genconfig.py Makefile
	@echo Generating kconfig.h...
	@python3 $(REPO_BASE_DIR)/scripts/genconfig.py $@ $(DEFS)
