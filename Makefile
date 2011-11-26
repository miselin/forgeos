# TODO: Put some comments here about how all this works.
# Further document the entire thing under doc/build-system


# CONFIGURATION SECTION
# You can set the below variables to control how you want the system to be
# built. Alternatively, instead of altering this Makefile, you can specify
# the same options in 'make.config' and those options will be used instead.
# To support multiple build options, you can pass the BUILD_CONFIG parameter
# to make via the command line, specifying the name of the config file to be
# read. Example:
# make BUILD_CONFIG=make-x86-64.config
# With the above specified command line, make will use the file
# 'make-x86-64.config' for the build configuration.

# Cross compiler prefix. Don't set this for native builds.
XCOMPILER_PREFIX := i586-elf
# Path to the cross compiler's bin directory. Set this only if
# the cross compiler tools are not in your PATH.
XCOMPILER_PATH :=

# Architecture to build for
ARCH_TARGET := x86
# Sub-Architecture to build for.
# For example, if your ARCH_TARGET is x86, you would set ARCH_SUBTARGET to
# x86-64 to build a 64-bit system.
ARCH_SUBTARGET := x86
# Platform to build for
PLATFORM_TARGET := pc

# override OUTPUT_DIR to place the build files somewhere other than ./build/
OUTPUT_DIR := build

# BUILD_ENV may be 'debug' or 'release'
BUILD_ENV := debug

# END CONFIGURATION SECTION


# BUILD_SRC is set on invocation of of make in the OUTPUT_DIR.
ifndef BUILD_SRC
# If BUILD_CONFIG was specified on the command line, read that file. It is an
# error if BUILD_CONFIG was specified and the file doesn't exist, but it is NOT
# an error if the default make.config file does not exist.
ifeq "$(origin $(BUILD_CONFIG))" "command line"
  # Make sure $(CONFIG) exists before reading it.
  ifeq "$(wildcard $(BUILD_CONFIG))" ""
    $(error Config file '$(BUILD_CONFIG)' does not exist)
  endif

  # Include the config file.
  include $(BUILD_CONFIG)
else
  # Attempt to read the default configuration file, but do not error if it
  # does not exist.
  -include make.config
endif

# Sanity check the build configuration.

# List of valid architectures
VALID_ARCHES := "x86"

# List of valid sub-architectures.
VALID_x86_SUBARCHES := "x86"

# List of valid platforms
VALID_PLATFORMS := "pc"

# List of valid build environments
VALID_BUILD_ENVS := debug release

# Ensure that ARCH_TARGET, ARCH_SUBTARGET, and PLATFORM_TARGET are all valid.
ifeq "$(findstring $(ARCH_TARGET), $(VALID_ARCHES))" ""
  $(error Specified architecture '$(ARCH_TARGET)' is not valid. Valid architectures are: $(VALID_ARCHES))
endif

ifeq "$(findstring $(ARCH_SUBTARGET), $(VALID_$(ARCH_TARGET)_SUBARCHES))" ""
  $(error Specified architecture '$(ARCH_SUBTARGET)' is not valid. Valid sub-architectures are: $(VALID_$(ARCH_TARGET)_SUBARCHES)))
endif

ifeq "$(findstring $(PLATFORM_TARGET), $(VALID_PLATFORMS))" ""
  $(error Specified platform '$(PLATFORM_TARGET)' is not valid. Valid platforms are: $(VALID_PLATFORMS))
endif

# Ensure that BUILD_ENV is valid.
ifeq "$(findstring $(BUILD_ENV), $(VALID_BUILD_ENVS))" ""
  $(error Specified BUILD_ENV '$(BUILD_ENV)' is not valid. Valid BUILD_ENV options are: $(VALID_BUILD_ENVS))
endif

# Make sure we can use the host's compiler tools if needed
HOSTAR := $(AR)
HOSTAS := $(AS)
HOSTCC := $(CC)
HOSTCPP := $(CPP)
HOSTCXX := $(CXX)
HOSTLD := $(LD)
HOSTNM := $(NM)
HOSTSTRIP := $(STRIP)

# If we're using a cross compiler, set compiler tools as needed
ifneq "$(XCOMPILER_PREFIX)" ""
  # Set XCOMPILER to be the either the full path to the cross compiler tools, or just the prefix.
  ifneq "$(XCOMPILER_PATH)" ""
    XCOMPILER := $(XCOMPILER_PATH)/$(XCOMPILER_PREFIX)-
  else
    XCOMPILER := $(XCOMPILER_PREFIX)-
  endif

  AR := $(XCOMPILER)ar
  AS := $(XCOMPILER)as
  CC := $(XCOMPILER)gcc
  CPP := $(XCOMPILER)cpp
  CXX := $(XCOMPILER)g++
  LD := $(XCOMPILER)ld
  NM := $(XCOMPILER)nm
  OBJCOPY := $(XCOMPILER)objcopy
  OBJDUMP := $(XCOMPILER)objdump
  STRIP := $(XCOMPILER)strip
endif

# BUILD_DIR is the base build directory, set up like build/x86-pc/debug/
BUILD_DIR := $(abspath $(OUTPUT_DIR)/$(ARCH_TARGET)-$(PLATFORM_TARGET)/$(BUILD_ENV))

# The directory for object files under the BUILD_DIR
OBJDIR := $(BUILD_DIR)/obj

# The directory for 'install' targets under the BUILD_DIR.
# INSTDIR should be suitable to pass to mkisofs as the base directory to use
# when creating a live CD.
INSTDIR := $(BUILD_DIR)/inst

# Export everything we have so far for the sub-make.
export ARCH_TARGET ARCH_SUBTARGET PLATFORM_TARGET
export AR AS CC CPP CXX LD NM OBJCOPY OBJDUMP STRIP
export HOSTAR HOSTAS HOSTCC HOSTCPP HOSTCXX HOSTLD HOSTNM HOSTSTRIP
export OUTPUT_DIR BUILD_ENV BUILD_DIR OBJDIR INSTDIR

# Don't perform the sub-make if we're running a clean or distclean target.
ifeq "$(findstring clean, $(MAKECMDGOALS))" ""

# At this point, we will create the BUILD_DIR if it does not exist, change to
# it, and then start the actual build.

.PHONY: _all sub-make $(MAKECMDGOALS)
_all:

#$(CURDIR)/Makefile Makefile: ;

$(filter-out _all sub-make $(BUILD_DIR), $(MAKECMDGOALS)) _all: sub-make
	@:

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# The "-e" below is VERY IMPORTANT so that the default configuration variables above
# DO NOT OVERRIDE what was entered from a build configuration.
sub-make: $(BUILD_DIR)
	@$(MAKE) -e -C $(BUILD_DIR) -f $(CURDIR)/Makefile BUILD_SRC=$(CURDIR) \
	$(filter-out _all sub-make $(BUILD_DIR), $(MAKECMDGOALS))

process-makefile := 1
endif #findstring
endif # BUILD_SRC

ifeq "$(process-makefile)" ""

CDIMAGE := $(BUILD_DIR)/mattise.iso

GRUB_ELTORITO := $(BUILD_SRC)/build-etc/stage2_eltorito-$(ARCH_TARGET)
GRUB_MENU := $(BUILD_SRC)/build-etc/menu.lst

MKISOFS := mkisofs

WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
			-Wwrite-strings -Wredundant-decls -Wnested-externs -Winline \
			-Wno-long-long -Wuninitialized -Wconversion

CFLAGS := -std=gnu99 -fno-builtin -nostdinc -nostdlib -ffreestanding

# TODO: Only add the following _CFLAGS and _DEFINES if they are set to avoid a lot of spaces in the command line
CFLAGS += $(ARCH_CFLAGS) $(ARCH_DEFINE)
CFLAGS += $(ARCH_SUBTARGET_CFLAGS) $(ARCH_SUBTARGET_DEFINE)
CFLAGS += $(PLATFORM_CFLAGS) $(PLATFORM_DEFINES)
CFLAGS += $(WARNINGS)

LDFLAGS := -nostdlib -nostartfiles
ASFLAGS :=

# Using clang for static analysis, for the win.
LINT := clang
LINT_FLAGS := --analyze $(CFLAGS) $(INCLUDES)
LINT_IGNORE := src/kernel/dlmalloc.c

# Directories to visit
DIRS := kboot kernel

.PHONY: analyse analyze clean $(DIRS)

all: analyse $(DIRS) $(CDIMAGE)

analyze: analyse
	@:

analyse:
	@echo
	@for f in $(filter-out $(LINT_IGNORE), $(SRCFILES)); do \
		echo Analysing $$f...; \
		$(LINT) $(LINT_FLAGS) -o /dev/null $(BUILD_SRC)/$$f; \
		if [ $$? -ne 0 ]; then \
			exit 1; \
		fi; \
		$(LINT_PHASE2) $(LINT_PHASE2_FLAGS) $$f; \
	done
	@echo

cdimage: $(CDIMAGE)

$(CDIMAGE): kernel kboot
	@echo Building ISO image...
	@mkdir -p $(INSTDIR)/boot/grub
	cp $(GRUB_ELTORITO) $(INSTDIR)/boot/grub/stage2_eltorito
	cp $(GRUB_MENU) $(INSTDIR)/boot/grub
	cp $(OBJDIR)/kernel/kernel $(INSTDIR)/boot
	mkisofs	-D -joliet -graft-points -quiet -input-charset ascii -R \
				-b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 \
				-boot-info-table -o $(CDIMAGE) -V 'MATTISE' $(INSTDIR)/

$(DIRS):
	@$(MAKE) -C $(BUILD_SRC)/src/$@


endif # process-makefile

clean:
	-@$(RM) -r $(OBJDIR)
	-@$(RM) -r $(INSTDIR)

distclean:
	-@$(RM) -r $(OUTPUT_DIR)
