obj-m :=demo_kobj.o

KERNEL :=/lib/modules/`uname -r`/build

all:
	make -C $(KERNEL) M=`pwd` modules

clean:
	make -C $(KERNEL) M=`pwd` clean




