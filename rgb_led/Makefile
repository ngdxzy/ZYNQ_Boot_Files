KERNELDIR:=/home/alfred/Work/linux-xlnx-master
PWD := $(shell pwd)
		
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
clean:
	rm -rf *.o *.mod.c *.mod.o *.ko

obj-m += rgbled.o
