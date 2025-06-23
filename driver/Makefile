# driver/Makefile

# Módulo del kernel
obj-m := robotDriver.o
CC = gcc-12

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Programa en espacio de usuario
USERPROG = uartSender
USERSRC = uartSender.c

all: kernel user

kernel:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

user:
	$(CC) $(USERSRC) -o $(USERPROG)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(USERPROG)

