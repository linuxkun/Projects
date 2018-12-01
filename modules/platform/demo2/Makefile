obj-m	:= plat_dev.o plat_drv.o plat_dev1.o

#KERNEL	:=/linux-3.5/ 
KERNEL	:=/lib/modules/`uname -r`/build

all:
	make -C $(KERNEL) M=`pwd` modules

clean:
	make -C $(KERNEL) M=`pwd` clean


