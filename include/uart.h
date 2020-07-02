#ifndef __UART_H__
#define __UART_H__

#define PCLK            50000000    // clock_init函数设置PCLK为50MHz
#define UART_CLK        PCLK        //  UART0的时钟源设为PCLK
#define UART_BAUD_RATE  115200      // 波特率
#define UART_BRD        ((UART_CLK  / (UART_BAUD_RATE * 16)) - 1)

void uart0_init(void);
void putc(unsigned char c);
unsigned char getc(void);
void puts(unsigned char *s);
unsigned char *gets(unsigned char *s);

#endif

