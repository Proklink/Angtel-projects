obj-m := hello_world.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

all: modules
install: modules_install

modules modules_install help clean:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) $@
