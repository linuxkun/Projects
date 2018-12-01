
obj-m	:= tasklet1.o

#KERNEL	:=/lib/modules/`uname -r`/build
KERNEL	:=/home/kun/linux-3.5/

all:
	make -C $(KERNEL) M=`pwd` modules

clean:
	make -C $(KERNEL) M=`pwd` clean




