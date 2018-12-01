MODULE_NAME := force_rmmod

XTRA_CFLAGS  += -g -std=gnu99  -Wfatal-errors 



ifneq ($(KERNELRELEASE),)  # kernelspace

obj-m := $(MODULE_NAME).o

else                        # userspace


CURRENT_PATH ?= $(shell pwd)
LINUX_KERNEL ?= $(shell uname -r)
LINUX_KERNEL_PATH ?= /lib/modules/$(LINUX_KERNEL)/build

CURRENT_PATH := $(shell pwd)

modules:
	    make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules

modules_install:
	    make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules_install


clean:
	    make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
		    rm -f modules.order Module.symvers Module.markers

.PHNOY:
	    modules modules_install clean



endif


