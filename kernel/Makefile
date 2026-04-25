# linux kernel make file for altera de1 soc 
# to be run on ubuntu x86 machine with kernel
# version 6.17
# shamelessly ripped and modified from CSEE4840 lab3 
# https://www.cs.columbia.edu/~sedwards/classes/2026/4840-spring/lab3-hw.tar.gz

KERNEL_REPO = https://github.com/altera-opensource/linux-socfpga.git
KERNEL_BRANCH = rel_socfpga-4.19_19.04.01_pr
DEFAULT_CONFIG = socfpga_defconfig
CROSS = env CROSS_COMPILE=arm-linux-gnueabihf- ARCH=arm

KERNEL_DIR = linux-socfpga
KERNEL_CONFIG = $(KERNEL_DIR)/.config
ZIMAGE = $(KERNEL_DIR)/arch/arm/boot/zImage

# kernel-download
#
#   Clone the Linux kernel repository
#
# kernel-config
#
#   Set up the default kernel configuration
#
# kernel-menuconfig
#
#   (Optional) Access the kernel configuration menu to make further
#   adjustments about which modules are included
#
# zimage
#
#   Compile the kernel

.PHONY : kernel-download kernel-config kernel-menuconfig zimage
kernel-download : $(KERNEL_DIR)
kernel-config : $(KERNEL_CONFIG)
kernel-menuconfig :
	$(CROSS) $(MAKE) -C $(KERNEL_DIR) menuconfig
zimage : $(ZIMAGE)

$(KERNEL_DIR) :
	git clone --branch $(KERNEL_BRANCH) $(KERNEL_REPO) $(KERNEL_DIR)

# Configure the kernel.  Start from a provided default,
#
# Turn off version checking (makes it easier to compile kernel
# modules and not have them complain about version)
#
# Turn on large file (+2TB) support, which the ext4 filesystem
# requires by default (it will not be able to mount the root
# filesystem read/write otherwise)
# Order-only prereq so an existing .config is left alone — only regenerated
# when the file is missing or when `make kernel-config` is invoked directly.
$(KERNEL_CONFIG) : | $(KERNEL_DIR)
	$(CROSS) $(MAKE) -C $(KERNEL_DIR) $(DEFAULT_CONFIG)
	$(KERNEL_DIR)/scripts/config --file $(KERNEL_CONFIG) \
	  --disable CONFIG_LOCALVERSION_AUTO \
	  --enable CONFIG_LBDAF \
	  --disable CONFIG_XFS_FS \
	  --disable CONFIG_GFS2_FS \
	  --disable CONFIG_TEST_KMOD

# Compile the kernel for my ubuntu 24.04 kernel 6.17.0-20-generic.
#   HOSTCFLAGS=-fcommon  — host-side dtc fix (GCC 10+ -fno-common default
#                          clashes with duplicate yylloc in dtc-lexer/parser).
#   KCFLAGS=-march=armv7-a — Cortex-A9 target. Forces the kernel's
#                          arch-v7 cc-option fallback (which lands on
#                          -march=armv5t on gcc 13 hard-float toolchains)
#                          to be overridden.  Last -march wins.
$(ZIMAGE) : $(KERNEL_CONFIG)
	$(CROSS) $(MAKE) -C $(KERNEL_DIR) LOCALVERSION= HOSTCFLAGS=-fcommon KCFLAGS=-march=armv7-a zImage

# clean

.PHONY : kernel-clean config-clean
kernel-clean :
	rm -rf $(KERNEL_DIR)

config-clean :
	rm -rf $(KERNEL_CONFIG)
