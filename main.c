#include <uart.h>
#include <sd.h>
#include <s3c2440_regs.h>

#define SD_BLOCK_SIZE			(512)

void delay(void)
{
	/* 由于使用了-O2优化编译选项，volatile的作用在于
	   告诉编译器空循环不要优化，否则会被优化为空语句 */
	volatile unsigned long i, j;

	for (i = 0; i < 300; ++i)
		for (j = 0; j < 300; ++j);
}

void main(void)
{	
	unsigned int *uboot_addr_on_sdram = (unsigned int *)0x32000000;		// 将u-boot读取到sdram的0x33f00000地址处
	unsigned int uboot_addr_on_sdcard = 2 * 1024 / SD_BLOCK_SIZE;		// u-boot存放在sdcard的2KB地址处
	unsigned int block_num = (512 * 1024) / SD_BLOCK_SIZE;				// u-boot存放在sdcard的大小为512KB
	// void (*theKernel)(void);

	// CLKCON = 0;
	// CLKCON |= 1 << 10;
	// CLKCON |= 1 << 13;

	uart0_init();
	// printk("\n\rearly disable all clocks\n\r");
	delay();
	sd_init();

	printk("\n\rBegin to copy uboot from sdcard(%dKB) to sdram(0x%x)...",
			uboot_addr_on_sdcard * 512 / 1024, uboot_addr_on_sdram);

	if (!sd_read_sector(uboot_addr_on_sdram, uboot_addr_on_sdcard, block_num)) {
		printk("   [done]\n\r");
	} else {
		printk("   [failed]\n\r");
		printk("Please check your sdcard!\n\r");
		while (1);
	}

	printk("\n\rbooting uboot...");

	// printk("\n\rdisable all clocks\n\r");
	// CLKCON = 0;
	// theKernel = (void (*)(void))uboot_addr_on_sdram;
	// theKernel();
}
