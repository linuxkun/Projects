obj-m	:= jiffies.o

KERNEL	:=/lib/modules/`uname -r`/build
#KERNEL	:=/home/kun/linux-3.5


all:
	@make -C $(KERNEL) M=`pwd` modules
	@echo "make ok..."

clean:
	make -C $(KERNEL) M=`pwd` clean
