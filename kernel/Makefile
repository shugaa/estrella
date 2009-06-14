EXTRA_CFLAGS = -Wall -Werror -O2

obj-m += usb2epp.o

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
		gcc -Wall -Werror -o drvtest -lgpif usb2epp_test.c

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
		rm drvtest*
		rm modules.order

