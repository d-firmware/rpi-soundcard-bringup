# Makefile — Generic ASoC Codec + Machine Driver
#
# Build against the running kernel (default):
#   make
#
# Build against a specific kernel source tree (cross-compile):
#   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
#        KDIR=/path/to/kernel/source
#
# Clean:
#   make clean

KDIR ?= /lib/modules/$(shell uname -r)/build

obj-m += my_codec.o
obj-m += my_machine.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod -A

load:
	@echo "Loading modules..."
	sudo insmod my_codec.ko
	sudo insmod my_machine.ko
	@echo "Check dmesg for bringup messages:"
	dmesg | tail -20

unload:
	@echo "Unloading modules..."
	-sudo rmmod my_machine
	-sudo rmmod my_codec

.PHONY: all clean install load unload
