#
# Copyright (c) 2011 Rich Edelman <redelman at gmail dot com>
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

# NOTE: This whole thing currently assumes x86-pc. Will fix that up later.

PLATFORM = $(MACH_TARGET)
ARCH = $(ARCH_TARGET)

# Compiler specific.
libgcc = $(shell $(CC) -print-file-name=libgcc.a)
LIBS := $(libgcc)

stdinc = $(shell $(CC) -print-file-name=include)

INCLUDES := -Isource/include -Isource/arch/$(ARCH)/include -Isource/platform/$(PLATFORM)/include
INCLUDES += -isystem $(stdinc)

ARCH_SOURCES := $(shell find source/arch/$(ARCH) -name '*.c' -o -name '*.S')

# This is correct for the PC platform only!
PLATFORM_SOURCES := loaders/chainload.c loaders/enter.S bios.S console.c
PLATFORM_SOURCES += disk.c memory.c platform.c pxe.c start.S video.c
PLATFORM_SOURCES := $(addprefix source/platform/$(PLATFORM)/, $(PLATFORM_SOURCES))

# Common source files for the loader itself.
LOADER_SOURCES := fs/ext2.c fs/iso9660.c
LOADER_SOURCES += lib/printf.c lib/string.c lib/support.c
LOADER_SOURCES += loaders/kboot.c
LOADER_SOURCES += partitions/msdos.c
LOADER_SOURCES += config.c console.c device.c disk.c elf.c error.c fs.c main.c memory.c menu.c
LOADER_SOURCES += ui.c video.c
LOADER_SOURCES := $(addprefix source/, $(LOADER_SOURCES))

SOURCES := $(ARCH_SOURCES) $(PLATFORM_SOURCES) $(LOADER_SOURCES)

OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))

# Base flags to start with
CFLAGS   := -std=gnu99
CCFLAGS   := -gdwarf-2 -pipe -nostdlib -nostdinc -ffreestanding -fno-stack-protector
ASFLAGS  := -D __ASM__ -nostdinc

# Arch (x86) Flags
CFLAGS  += -m32 -mno-sse -mno-mmx -mno-sse2 -mno-3dnow
ASFLAGS += -m32

# Platform flags

# Include the platform configuration file (HACK)
CCFLAGS += -include 'source/platform/$(PLATFORM)/include/platform/config.h'
ASFLAGS += -include 'source/platform/$(PLATFORM)/include/platform/config.h'

# Final flags (override anything from earlier)
CFLAGS += -Os

# Platform (pc) Flags
DPADD := source/platform/$(PLATFORM)/link.ld

WARNINGS := -Wall -Wextra -Werror -Wno-variadic-macros -Wno-unused-parameter -Wwrite-strings
WARNINGS += -Wmissing-declarations -Wredundant-decls -Wno-format -Wno-unused-but-set-variable

CPPFLAGS := $(CCFLAGS) $(INCLUDES) $(WARNINGS)

all: cdboot pxeboot loader
	@:

cdboot.elf: LDFLAGS += -T source/platform/$(PLATFORM)/stage1/stage1.ld
cdboot.elf: source/platform/$(PLATFORM)/stage1/cdboot.o
	@$(LD) $(LDFLAGS) -o $@ $<

cdboot: cdboot.elf
	echo "  OBJCOPY     cdboot"
	$(OBJCOPY) -O binary --strip-all -R '.note.*' -R .comment $< $@

pxeboot.elf: LDFLAGS += -T source/platform/$(PLATFORM)/stage1/stage1.ld
pxeboot.elf: source/platform/$(PLATFORM)/stage1/pxeboot.o
	@$(LD) $(LDFLAGS) -o $@ $<


pxeboot: pxeboot.elf
	@echo "  OBJCOPY     pxeboot"
	@$(OBJCOPY) -O binary --strip-all -R '.note.*' -R .comment $< $@

loader.elf: LDFLAGS += -T source/platform/$(PLATFORM)/link.ld
loader.elf: $(OBJECTS) $(DPADD)
	@$(LD) $(LDFLAGS) -o $@  $(OBJECTS) $(LIBS)


loader: loader.elf
	@echo "  OBJCOPY     loader"
	@$(OBJCOPY) -O binary --strip-all $< $@

CLEANFILES = cdboot cdboot.elf pxeboot pxeboot.elf loader loader.elf

clean:
	@echo Clean $(CLEANDIR)
	rm -f $(OBJECTS) source/platform/$(PLATFORM)/stage1/*.o $(CLEANFILES)

%.ld: %.ld.in
	@echo "  Preprocessing linker script"
	@$(CPP) $(ASFLAGS) $(INCLUDES) -x c $< | grep -v '^\#' > $@
