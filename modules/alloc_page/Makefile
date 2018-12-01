obj-m	:=al_page.o

KERNEL	:=/lib/modules/`uname -r`/build

all:
	make -C $(KERNEL) M=`pwd` modules

clean:
	make -C $(KERNEL) M=`pwd` clean






