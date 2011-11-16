XCOMPILER_TARGET := i686
XCOMPILER_FORMAT := elf
XCOMPILER_PREFIX := ~/xcompiler/bin
XCOMPILER_TUPLE := $(XCOMPILER_TARGET)-$(XCOMPILER_FORMAT)

-include make.config

CC := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-gcc
LD := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-ld
AS := $(XCOMPILER_PREFIX)/$(XCOMPILER_TUPLE)-as

ARCH_DEFINE := -DX86
ARCH_TARGET := x86
ARCH_SUBTARGET := x86
MACH_TARGET := pc

DIRS := src src/arch/$(ARCH_TARGET) src/mach/$(MACH_TARGET) src/include

INCDIRS := -I src/include -I src/arch/$(ARCH_TARGET) -I src/mach/$(MACH_TARGET)
LIBDIRS :=

ASMFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.s")
SRCFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.c")
HDRFILES := $(shell find $(DIRS) -maxdepth 1 -type f -name "*.h")

OBJDIR := build

OBJFILES := $(patsubst %.s,$(OBJDIR)/%.o,$(ASMFILES)) $(patsubst %.c,$(OBJDIR)/%.o,$(SRCFILES))
DEPFILES := $(patsubst %.c,$(OBJDIR)/%.d,$(SRCFILES))

KERNEL := $(OBJDIR)/kernel

KERNEL_LSCRIPT := src/arch/$(ARCH_TARGET)/linker-$(ARCH_SUBTARGET).ld

CDIMAGE := $(OBJDIR)/mattise.iso

GRUB_ELTORITO := build-etc/stage2_eltorito-$(ARCH_TARGET)
GRUB_MENU := build-etc/menu.lst

MKISOFS := mkisofs

WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
			-Wwrite-strings -Wredundant-decls -Wnested-externs -Winline \
			-Wno-long-long -Wuninitialized -Wconversion
CFLAGS := $(ARCH_DEFINE) -DDEBUG -O3 -g -std=gnu99 -fno-builtin -nostdinc -nostdlib -ffreestanding $(WARNINGS)
LDFLAGS := -nostdlib -nostartfiles
ASFLAGS :=

LINT := splint
LINT_FLAGS := +nolib +charint -predboolint -paramuse -nullret -temptrans -usedef -branchstate -compdef -retvalint $(ARCH_DEFINE) $(INCDIRS)

all: objdirs analyse $(KERNEL) $(CDIMAGE)

objdirs:
	-@for d in $(DIRS); do \
		mkdir -p $(OBJDIR)/$$d; \
	done;

analyse:
	@echo
	@echo Static analysis...
	# @$(LINT) $(LINT_FLAGS) $(SRCFILES)
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
