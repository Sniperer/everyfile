obj-m := vfs_kprobe.o 

CROSS_COMPILE=''

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.o *.mod.c .* *.cmd *.symvers modul*