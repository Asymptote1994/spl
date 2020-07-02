#include "vsprintf.h"
#include "string.h"

extern void putc(char c);

#define	OUTBUFSIZE	1024

static char g_pcOutBuf[OUTBUFSIZE];

int printk(const char *fmt, ...)
{
	int i;
	int len;
	va_list args;

	va_start(args, fmt);
	len = vsprintf(g_pcOutBuf,fmt,args);
	va_end(args);
	for (i = 0; i < strlen(g_pcOutBuf); i++) {
		putc(g_pcOutBuf[i]);
	}

	return len;
}
