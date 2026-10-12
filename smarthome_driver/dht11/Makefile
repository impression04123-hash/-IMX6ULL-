ifeq ($(KERNELRELEASE),)
X86_KERNEL_BUILD = /lib/modules/$(shell uname -r)/build
ARM_KERNEL_BUILD = /home/dxg/alientek/kernel/linux-imx-rel_imx_4.1.15_2.1.0_ga_smarthome
MODULE_PATH = $(shell pwd)

x86_module:
	make -C $(X86_KERNEL_BUILD) M=$(MODULE_PATH) modules

arm_module:
	make -C $(ARM_KERNEL_BUILD) M=$(MODULE_PATH) modules ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
	cp dht11_driver.ko /home/dxg/alientek/nfs/rootfs

clean:
	make -C $(X86_KERNEL_BUILD) M=$(MODULE_PATH) clean

else
	obj-m = dht11_driver.o
endif