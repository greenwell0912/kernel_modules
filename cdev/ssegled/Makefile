HOME ?= /home/yourname
ARCH := arm
CROSS_COMPILE := $(HOME)/raspberry/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
KPATH := $(HOME)/raspberry/linux

obj-m := ssegled.o

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KPATH) M=$(PWD)  modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KPATH) M=$(PWD) clean
