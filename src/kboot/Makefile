#
# Copyright (c) 2011-2012 Rich Edelman <redelman at gmail dot com>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

PLATFORM = $(PLATFORM_TARGET)
ARCH = $(ARCH_TARGET)

# Compiler specific.
libgcc = $(shell $(CC) -print-file-name=libgcc.a) 
LIBS := $(libgcc)

stdinc = $(shell $(CC) -print-file-name=include)

INCLUDES := -Isource/include -Isource/arch/$(ARCH)/include -Isource/platform/$(PLATFORM)/include
INCLUDES += -isystem $(stdinc)

ARCH_SOURCES := $(shell cd source && find arch/$(ARCH) -name '*.c' -o -name '*.S')

# If we're building x86-pc then filter out stage1 files from PLATFORM_SOURCES
ifeq "$(PLATFORM_TARGET)" "pc"
	PLATFORM_SOURCES := $(shell cd source && find platform/$(PLATFORM_TARGET) -name '*.c' -o -name '*.S' | grep -v stage1)
else
	PLATFORM_SOURCES := $(shell cd source && find platform/$(PLATFORM_TARGET) -name '*.c' -o -name '*.S')
endif


# Common source files for the loader itself.
LOADER_SOURCES := fs/ext2.c fs/iso9660.c
LOADER_SOURCES += lib/printf.c lib/string.c lib/support.c
LOADER_SOURCES += loaders/kboot.c
LOADER_SOURCES += partitions/msdos.c
LOADER_SOURCES += config.c console.c device.c disk.c elf.c error.c fs.c main.c memory.c menu.c
LOADER_SOURCES += ui.c video.c

SOURCES := $(ARCH_SOURCES) $(PLATFORM_SOURCES) $(LOADER_SOURCES)

OBJECTS := $(addsuffix .gcc.o, $(basename $(SOURCES)))

OBJDIR := $(OBJDIR)/kboot

VPATH = source/
OBJECTS := $(addprefix $(OBJDIR)/, $(OBJECTS))

# Base flags to start with
CFLAGS   := -std=gnu99
CCFLAGS   := -gdwarf-2 -pipe -nostdlib -nostdinc -ffreestanding -fno-stack-protector
ASFLAGS  := -D __ASM__ -nostdinc

# Arch (x86) Flags
ifeq "$(ARCH_TARGET)" "x86"
CFLAGS  += -m32 -mno-sse -mno-mmx -mno-sse2 -mno-3dnow
ASFLAGS += -m32
endif

ifeq "$(ARCH_TARGET)" "arm"
CFLAGS +=
CCFLAGS += -mno-thumb-interwork
endif

# Platform flags
ifeq "$(PLATFORM_TARGET)" "omap3"
CCFLAGS += -mcpu=cortex-a8 -mfpu=vfp
endif

# Include the platform configuration file (HACK)
CCFLAGS += -include 'source/platform/$(PLATFORM)/include/platform/config.h'
ASFLAGS += -include 'source/platform/$(PLATFORM)/include/platform/config.h'

# Final flags (override anything from earlier)
CFLAGS += -Os

# DPADD is the platform linker script, as it needs some preprocessing. 
DPADD := $(OBJDIR)/source/platform/$(PLATFORM)/link.ld

WARNINGS := -Wall -Wextra -Werror -Wno-variadic-macros -Wno-unused-parameter -Wwrite-strings
WARNINGS += -Wmissing-declarations -Wredundant-decls -Wno-format -Wno-unused-but-set-variable

CPPFLAGS := $(CCFLAGS) $(INCLUDES) $(WARNINGS)

LINT_FLAGS := $(CFLAGS) $(CCFLAGS) $(INCLUDES)
LINT_IGNORE :=

.PHONY: all
all: $(OBJDIR)/loader
	@:

# PC platform specific targets
ifeq "$(PLATFORM_TARGET)" "pc"
all: $(OBJDIR)/cdboot $(OBJDIR)/pxeboot

$(OBJDIR)/cdboot.elf: LDFLAGS += -T source/platform/$(PLATFORM)/stage1/stage1.ld
$(OBJDIR)/cdboot.elf: $(OBJDIR)/source/platform/$(PLATFORM)/stage1/cdboot.gcc.o
	@echo "  LINKING     cdboot.elf"
	@$(LD) $(LDFLAGS) -o $@ $<

$(OBJDIR)/cdboot: $(OBJDIR)/cdboot.elf
	@echo "  OBJCOPY     cdboot"
	@$(OBJCOPY) -O binary --strip-all -R '.note.*' -R .comment $< $@

$(OBJDIR)/pxeboot.elf: LDFLAGS += -T source/platform/$(PLATFORM)/stage1/stage1.ld
$(OBJDIR)/pxeboot.elf: $(OBJDIR)/source/platform/$(PLATFORM)/stage1/pxeboot.gcc.o
	@echo "  LINKING     pxeboot.elf"
	@$(LD) $(LDFLAGS) -o $@ $<


$(OBJDIR)/pxeboot: $(OBJDIR)/pxeboot.elf
	@echo "  OBJCOPY     pxeboot"
	@$(OBJCOPY) -O binary --strip-all -R '.note.*' -R .comment $< $@

endif # PC platform specific targets

$(OBJDIR)/loader.elf: LDFLAGS += -T $(OBJDIR)/source/platform/$(PLATFORM)/link.ld
$(OBJDIR)/loader.elf: $(OBJECTS) $(DPADD)
	@echo "  LINKING     loader.elf"
	@$(LD) $(LDFLAGS) -o $@  $(OBJECTS) $(LIBS)

$(OBJDIR)/loader: $(OBJDIR)/loader.elf
	@echo "  OBJCOPY     loader"
	@$(OBJCOPY) -O binary --strip-all $< $@

analyse:
	@echo "KBoot Static Analysis:"
	@for f in $(filter-out $(LINT_IGNORE), $(filter-out %.S, $(addprefix source/, $(SOURCES)))); do \
		$(LINT) $(LINT_FLAGS) -o /dev/null $$f; \
	done
	@echo "End KBoot Static Analysis\n"

include $(BUILD_SRC)/scripts/rules.mk

