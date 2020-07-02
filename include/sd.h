#ifndef __SDI_H__
#define __SDI_H__
 
#define u32 unsigned int
#define u16 unsigned short
#define u8  unsigned char
#define PCLK  50000000

#define SDICON     (*(volatile unsigned *)0x5a000000)	//SDI control
#define SDIPRE     (*(volatile unsigned *)0x5a000004)	//SDI baud rate prescaler

#define SDICARG    (*(volatile unsigned *)0x5a000008)	//SDI command argument
#define SDICCON    (*(volatile unsigned *)0x5a00000c)	//SDI command control
#define SDICSTA    (*(volatile unsigned *)0x5a000010)	//SDI command status

#define SDIRSP0    (*(volatile unsigned *)0x5a000014)	//SDI response 0
#define SDIRSP1    (*(volatile unsigned *)0x5a000018)	//SDI response 1
#define SDIRSP2    (*(volatile unsigned *)0x5a00001c)	//SDI response 2
#define SDIRSP3    (*(volatile unsigned *)0x5a000020)	//SDI response 3
#define SDIDTIMER  (*(volatile unsigned *)0x5a000024)	//SDI data/busy timer
#define SDIBSIZE   (*(volatile unsigned *)0x5a000028)	//SDI block size
#define SDIDCON    (*(volatile unsigned *)0x5a00002c)	//SDI data control
#define SDIDCNT    (*(volatile unsigned *)0x5a000030)	//SDI data remain counter
#define SDIDSTA    (*(volatile unsigned *)0x5a000034)	//SDI data status
#define SDIFSTA    (*(volatile unsigned *)0x5a000038)	//SDI FIFO status
#define SDIIMSK    (*(volatile unsigned *)0x5a00003c)	//SDI interrupt mask. edited for 2440A

#ifdef __BIG_ENDIAN  /* edited for 2440A */
#define rSDIDAT    (*(volatile unsigned *)0x5a00004c)	//SDI data
#define SDIDAT     (*(volatile unsigned *)0x5a00004c)
#else  // Little Endian
#define rSDIDAT    (*(volatile unsigned *)0x5a000040)	//SDI data
#define SDIDAT     (*(volatile unsigned *)0x5a000040)
#endif


//������Ϣ�ṹ��
struct sd_info
{
	u8 sdiWide; // 0:1bit, 1:4bit
	u8 sdiType;  // 0:SD  , 1:MMC
	u16 sdiRCA;
	u8 cCardCID[16]; // ����CID��Ϣ
	u32 lCardCSD[4]; // ����CSD��Ϣ
	u32 lSectorSize; /* һ�οɲ����Ŀ���� */ 
	u32 lCardSize; //������(��λ:�ֽ�)
}; 
 
int sd_test();
u8 sd_init();
u32 SDI_Check_CMD_End(int cmd, int be_resp);
void CMD0(void);
u8 CMD1(void);
u8 CMD2(u8 *cCID_Info);
u8 CMD3(u16 iCardType,u16 *iRCA);
u8 CMD7(u8 cSorD,u16 iRCA);
u8 CMD9(u16 iRCA,u32 *lCSD);
u8 CMD12(void);
u16 CMD13(u16 iRCA);
u8 CMD17(u32 Addr);
u8 CMD18(u32 Addr);
u8 CMD24(u32 Addr);
u8 CMD25(u32 Addr);
u8 CMD55(u16 iRCA);
u8 ACMD6(u8 BusWidth,u16 iRCA);
u8 ACMD41(u16 iRCA);
u32 SDI_MMC_OCR(void);
u32 SDI_SD_OCR(void);
u8 select_or_deselect(u8 cSelDesel,u16 iCardRCA);
u8 Set_bus_Width(u8 cCardType,u8 cBusWidth,u16 iRCA);
u8 sd_read_sector(u32 *buf, u32 addr, u32 block_num);
u8 sd_write_sector(u32 *buf, u32 addr, u32 block_num);
void file_delay(u32 i);
#endif

