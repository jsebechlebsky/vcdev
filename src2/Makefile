target=vcmod
vcmod-objs  = vcmodule.o vccontrol.o vcdevice.o vcioctl.o
obj-m = $(target).o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

$(target).o: $(vcmod-objs)
