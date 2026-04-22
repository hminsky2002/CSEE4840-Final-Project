ifneq (${KERNELRELEASE},)

# Being compiled as part of the kernel build
    obj-m := wave_synth.o

else

# Being invoked as a top-level make: use the kernel build system
    KERNEL_SOURCE := /usr/src/linux-headers-$(shell uname -r)
    PWD := $(shell pwd)

default: module

module:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} SUBDIRS=${PWD} clean

.PHONY: default module clean

endif
