obj-m := ibmvsm.o

# enable for debug logging to /var/log/kern.log
# CFLAGS_ibmvsm.o := -DDEBUG

KDIR  := /lib/modules/$(shell uname -r)/build

default: modules
install: modules_install

modules modules_install clean:
	$(MAKE) -C $(KDIR) M=$(PWD) $@
