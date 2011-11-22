XCOMPILER_TARGET := i686
XCOMPILER_FORMAT := elf
XCOMPILER_PREFIX := ~/xcompiler/bin

ARCH_DEFINE := -DX86
ARCH_TARGET := x86
ARCH_SUBTARGET := x86
MACH_TARGET := pc

-include make.config

XCOMPILER_TUPLE := $(XCOMPILER_TARGET)-$(XCOMPILER_FORMAT)

CC := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-gcc
LD := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-ld
AS := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-as

DIRS := src/kernel src/kernel/arch/$(ARCH_TARGET) src/kernel/mach/$(MACH_TARGET) src/kernel/include

INCDIRS := -I src/kernel/include -I src/kernel/arch/$(ARCH_TARGET) -I src/kernel/mach/$(MACH_TARGET)
LIBDIRS :=

ASMFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.s")
SRCFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.c")
HDRFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.h")

OBJDIR := build

OBJFILES := $(patsubst %.s,$(OBJDIR)/%.o,$(ASMFILES)) $(patsubst %.c,$(OBJDIR)/%.o,$(SRCFILES))
DEPFILES := $(patsubst %.c,$(OBJDIR)/%.d,$(SRCFILES))

KERNEL := $(OBJDIR)/kernel

KERNEL_LSCRIPT := src/kernel/arch/$(ARCH_TARGET)/linker-$(ARCH_SUBTARGET).ld

CDIMAGE := $(OBJDIR)/mattise.iso

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
LINT_FLAGS := $(CFLAGS) $(INCDIRS) -o /dev/null -c
LINT_IGNORE := src/kernel/dlmalloc.c

# Phase 2 is merely for post-clang analysis.
# clang does nice things like check inline assembly, while cppcheck
# will pick up on less important issues such as style.
# (eg, checking whether an unsigned variable is < 0)
LINT_PHASE2 := cppcheck
LINT_PHASE2_FLAGS := --error-exitcode=1 -q --enable=style,performance,portability --std=c99 $(INCDIRS)

.PHONY: objdirs analyse clean


.PHONY: objdirs analyse clean

all: objdirs analyse $(KERNEL) $(CDIMAGE)

objdirs:
	-@for d in $(DIRS); do \
		mkdir -p $(OBJDIR)/$$d; \
	done;

analyse: $(OBJFILES)
	@echo
	@for f in $(SRCFILES); do \
		if [[ "$$f" =~ "$(LINT_IGNORE)" ]]; then \
			continue; \
		fi; \
		echo Analysing $$f...; \
		$(LINT) $(LINT_FLAGS) $$f; \
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

$(CDIMAGE): $(KERNEL)
	@echo Building ISO image...
	@mkisofs	-D -joliet -graft-points -quiet -input-charset ascii -R \
				-b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 \
				-boot-info-table -o $(CDIMAGE) -V 'MATTISE' \
				boot/grub/stage2_eltorito=$(GRUB_ELTORITO) \
				boot/grub/menu.lst=$(GRUB_MENU) \
				boot/kernel=$(KERNEL)

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
