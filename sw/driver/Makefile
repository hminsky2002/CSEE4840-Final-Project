ifneq (${KERNELRELEASE},)
	obj-m := wave_synth.o
else 

	KERNEL_SOURCE := /usr/src/linux-headers-$(shell uname -r)
	PWD := $(shell pwd)

default: module 
module:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules
clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean

.PHONY: default module clean 

endif 

