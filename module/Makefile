#
# Makefile for the i2c bus drivers.
#

obj-m	+= i2c-mock.o
obj-m += sample-i2c-client.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install
