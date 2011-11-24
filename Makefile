XCOMPILER_TARGET := i686
XCOMPILER_FORMAT := elf
XCOMPILER_PREFIX := ~/xcompiler/bin

ARCH_DEFINE := -DX86
ARCH_TARGET := x86
ARCH_SUBTARGET := x86
MACH_TARGET := pc

-include make.config

AS := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-as
CC := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-gcc
LD := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-ld
AS := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-as
OBJCOPY := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-objcopy

# This is needed for recursive make
export ARCH_DEFINE ARCH_TARGET ARCH_SUBTARGET MACH_TARGET
export CC LD AS OBJCOPY

DIRS := src/kernel src/kernel/arch/$(ARCH_TARGET) src/kernel/mach/$(MACH_TARGET) src/kernel/include

INCDIRS := -I src/kernel/include -I src/kernel/arch/$(ARCH_TARGET) -I src/kernel/mach/$(MACH_TARGET)
LIBDIRS :=

ASMFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.s")
SRCFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.c")
HDRFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.h")

OBJDIR := build
export OBJDIR

OBJFILES := $(patsubst %.s,$(OBJDIR)/%.o,$(ASMFILES)) $(patsubst %.c,$(OBJDIR)/%.o,$(SRCFILES))
DEPFILES := $(patsubst %.c,$(OBJDIR)/%.d,$(SRCFILES))

KBOOT := $(OBJDIR)/kboot
KERNEL := $(OBJDIR)/kernel

KERNEL_LSCRIPT := src/kernel/arch/$(ARCH_TARGET)/linker-$(ARCH_SUBTARGET).ld

CDIMAGE := $(BUILD_DIR)/mattise.iso

GRUB_ELTORITO := build-etc/stage2_eltorito-$(ARCH_TARGET)
GRUB_MENU := build-etc/menu.lst

MKISOFS := mkisofs

WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
			-Wwrite-strings -Wredundant-decls -Wnested-externs -Winline \
			-Wno-long-long -Wuninitialized -Wconversion

# TODO: arch-specific CFLAGS
CFLAGS := -m32 $(ARCH_DEFINE) -DDEBUG -O3 -g -std=gnu99 -fno-builtin -nostdinc -nostdlib -ffreestanding $(WARNINGS)
LDFLAGS := -nostdlib -nostartfiles
ASFLAGS :=

# Using clang for static analysis, for the win.
LINT := clang
LINT_FLAGS := --analyze $(CFLAGS) $(INCDIRS)
LINT_IGNORE := src/kernel/dlmalloc.c

# Phase 2 is merely for post-clang analysis.
# clang does nice things like check inline assembly, while cppcheck
# will pick up on less important issues such as style.
# (eg, checking whether an unsigned variable is < 0)
LINT_PHASE2 := cppcheck
LINT_PHASE2_FLAGS := --error-exitcode=1 -q --enable=style,performance,portability --std=c99 $(INCDIRS)

.PHONY: objdirs analyse clean

all: objdirs analyse $(KBOOT) $(KERNEL) $(CDIMAGE)

objdirs:
	-@for d in $(DIRS); do \
		mkdir -p $(OBJDIR)/$$d; \
	done;

analyze: analyse
	@:

analyse:
	@echo
	@for f in $(filter-out $(LINT_IGNORE), $(SRCFILES)); do \
		echo Analysing $$f...; \
		$(LINT) $(LINT_FLAGS) -o /dev/null $$f; \
		if [ $$? -ne 0 ]; then \
			exit 1; \
		fi; \
		$(LINT_PHASE2) $(LINT_PHASE2_FLAGS) $$f; \
	done
	@echo

clean:
	-@rm -f $(DEPFILES)
	-@rm -f $(OBJFILES)
	-@rm -f $(KERNEL)
	-@make -C src/kboot clean

$(CDIMAGE): $(KERNEL)
	@echo Building ISO image...
	@mkisofs	-D -joliet -graft-points -quiet -input-charset ascii -R \
				-b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 \
				-boot-info-table -o $(CDIMAGE) -V 'MATTISE' \
				boot/grub/stage2_eltorito=$(GRUB_ELTORITO) \
				boot/grub/menu.lst=$(GRUB_MENU) \
				boot/kernel=$(KERNEL)

$(KBOOT): objdirs
	$(MAKE) -C src/kboot all

$(KERNEL): $(OBJFILES)
	@echo Linking kernel...
	@$(LD) $(LDFLAGS) $(LIBDIRS) -o $@ -T $(KERNEL_LSCRIPT) $(OBJFILES)

-include $(DEPFILES)

$(OBJDIR)/%.o: %.c Makefile
	@echo Compiling $<...
	@$(CC) $(CFLAGS) $(INCDIRS) -MMD -MP -c $< -o $@

$(OBJDIR)/%.o: %.s Makefile
	@echo Assembling $<...
	@$(AS) $(ASFLAGS) $< -o $@
