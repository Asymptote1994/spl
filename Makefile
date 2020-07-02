# CROSS_COMPILE = arm-none-linux-gnueabi-
CROSS_COMPILE = arm-linux-gnueabi-

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJJUMP = $(CROSS_COMPILE)objdump

CFLAGS = -g -Wall -O2 -nostdlib -fno-builtin -march=armv4t -marm -I$(shell pwd)/include

.PHONY:

spl.bin: start.o main.o nand.o uart.o sd.o mylibc/mylibc.a
	$(LD) -T spl.lds $^ -o spl.elf
	$(OBJCOPY) -S spl.elf -O binary $@
	$(OBJJUMP) -D -m arm spl.elf > spl.dis

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:%.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *.elf *.dis *.bin

























