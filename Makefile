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

# mkisofs can be overridden by the config file as needed (eg for genisoimage)
MKISOFS := mkisofs

# Override the serial tty for the 'kermit' target in ARM testing.
SERIAL_TTY := /dev/ttyUSB0

# Override the default shell to be 'bash' (and not 'dash' or similar)
SHELL := /bin/bash

# Should the build system use `clang' and LLVM for targets which support it?
USE_CLANG := no

# Path to LLVM binaries, including `llc' and `clang'.
LLVM_PATH :=

# Set to anything to enable building with -Werror.
ENABLE_WERROR :=

# END CONFIGURATION SECTION


# REPO_BASE_DIR is the base source directory, containing the top-level Makefile
REPO_BASE_DIR := $(PWD)

# BUILD_SRC is set on invocation of of make in the OUTPUT_DIR.
ifndef BUILD_SRC
# If BUILD_CONFIG was specified on the command line, read that file. It is an
# error if BUILD_CONFIG was specified and the file doesn't exist, but it is NOT
# an error if the default make.config file does not exist.
ifeq "$(origin BUILD_CONFIG)" "command line"
$(info "Reading configuration from $(BUILD_CONFIG)")
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
VALID_ARCHES := "x86 arm"

# List of valid sub-architectures.
VALID_x86_SUBARCHES := "x86"

# List of valid sub-arches for arm.
VALID_arm_SUBARCHES := "armv7"

# List of valid platforms
VALID_PLATFORMS := "pc omap3"

# List of valid build environments
VALID_BUILD_ENVS := debug release testing

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

  ifneq "$(LLVM_PATH)" ""
    LLC := $(LLVM_PATH)/llc
    CLANG := $(LLVM_PATH)/clang
    LLVMLD := $(LLVM_PATH)/llvm-link
    LLVMAS := $(LLVM_PATH)/llvm-mc
  else
    LLC := llc
    CLANG := clang
    LLVMLD := llvm-link
    LLVMAS := llvm-mc
  endif
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
export ARCH_TARGET ARCH_SUBTARGET PLATFORM_TARGET SHELL
export AR AS CC CPP CXX LD NM OBJCOPY OBJDUMP STRIP MKISOFS
export HOSTAR HOSTAS HOSTCC HOSTCPP HOSTCXX HOSTLD HOSTNM HOSTSTRIP
export OUTPUT_DIR BUILD_ENV BUILD_DIR OBJDIR INSTDIR SERIAL_TTY
export CLANG LLC LLVMLD LLVMAS USE_CLANG ENABLE_WERROR

# Don't perform the sub-make if we're running a clean or distclean target.
ifeq "$(findstring clean, $(MAKECMDGOALS))" ""

# At this point, we will create the BUILD_DIR if it does not exist, change to
# it, and then start the actual build.

MAKEFLAGS += --no-print-directory

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

ifeq "$(ARCH_TARGET)" "x86"
CDIMAGE := $(BUILD_DIR)/forge.iso
OUTIMAGE := $(CDIMAGE)
endif

ifeq "$(MKISOFS)" ""
MKISOFS := mkisofs
endif

ifeq "$(ARCH_TARGET)" "arm"
UIMAGE := $(BUILD_DIR)/uImage
OUTIMAGE := $(UIMAGE)
endif

# Using clang for static analysis, for the win.
LINT := $(CLANG) --analyze
export LINT

# Directories under src/ to visit
DIRS :=
ifeq "$(ARCH_TARGET)" "x86"
DIRS += kboot
endif
DIRS += kernel

.PHONY: analyse analyze clean cleanlog closelog kermit qemu $(DIRS)

all: $(DIRS) closelog $(OUTIMAGE)

analyze: analyse

analyse:
	@echo "Static Analysis Started at `date`"| tee $(BUILD_DIR)/analysis.log
	@echo "Note that this cannot be done as a parallel job so may take a long time to complete.\n" | tee -a $(BUILD_DIR)/analysis.log
	@for d in $(DIRS); do \
		$(MAKE) --no-print-directory -C $(BUILD_SRC)/src/$$d analyse 2>&1 | tee -a $(BUILD_DIR)/analysis.log; \
	done
	@echo "Static Analysis Complete at `date`" | tee -a $(BUILD_DIR)/analysis.log
	@echo "Results have been saved to: $(BUILD_DIR)/analysis.log\n"

ifneq "$(CDIMAGE)" ""
cdimage: $(CDIMAGE)

$(CDIMAGE): kernel kboot
	@echo "Building ISO image..."
	@mkdir -p $(INSTDIR)/System/Boot $(INSTDIR)/System/Modules
	@cat $(OBJDIR)/kboot/cdboot $(OBJDIR)/kboot/loader > $(INSTDIR)/System/Boot/cdboot.img
	@cp $(BUILD_SRC)/build-etc/loader.cfg $(INSTDIR)/System/Boot/loader.cfg
	@cp $(OBJDIR)/kernel/kernel $(INSTDIR)/System/Boot/kernel
	@$(MKISOFS)	-D -joliet -graft-points -quiet -input-charset iso8859-1 -R \
				-b System/Boot/cdboot.img -no-emul-boot -boot-load-size 4 \
				-boot-info-table -o $(CDIMAGE) -V 'FORGEOS' $(INSTDIR)/
	@echo "ISO image has been saved to: $(CDIMAGE)\n"

qemu: $(CDIMAGE)
	qemu-system-x86_64 -m 16 -cdrom $(CDIMAGE) -boot d -serial file:$(PWD)/build/serial.txt -monitor stdio -no-reboot
endif

ifneq "$(UIMAGE)" ""
uimage : $(UIMAGE)

$(UIMAGE): kernel
	@echo "Building uImage for u-boot..."
	@$(OBJCOPY) -O binary $(OBJDIR)/kernel/kernel $(OBJDIR)/kernel/kernel.flat
	@mkimage -A arm -O linux -T kernel -C none -a 0x80008000 -e 0x80008000 \
	-n forge -d $(OBJDIR)/kernel/kernel.flat $(UIMAGE) 2>&1 | tee -a $(BUILD_DIR)/build.log; exit $${PIPESTATUS[0]}

kermit: $(UIMAGE)
	@echo "Loading kernel via Kermit on a device..."
	@$(BUILD_SRC)/scripts/kermit.sh $(UIMAGE) $(SERIAL_TTY) 2>&1 | tee -a $(BUILD_DIR)/build.log; exit $${PIPESTATUS[0]}

endif

$(DIRS): cleanlog
	@mkdir -p $(OBJDIR)/$@
	@$(MAKE) REPO_BASE_DIR=$(REPO_BASE_DIR) OBJDIR=$(OBJDIR)/$@ -C $(BUILD_SRC)/src/$@ 2>&1 | tee -a $(BUILD_DIR)/build.log; exit $${PIPESTATUS[0]}

cleanlog:
	@echo "System build started at `date`\n" | tee $(BUILD_DIR)/build.log

closelog: $(DIRS)
	@echo "\nSystem build finished at `date`" | tee -a $(BUILD_DIR)/build.log
	@echo "Build log has been saved to: $(BUILD_DIR)/build.log\n"

endif # process-makefile

clean:
	-@$(RM) -r $(OBJDIR)
	-@$(RM) -r $(INSTDIR)

distclean:
	-@$(RM) -r $(OUTPUT_DIR)
