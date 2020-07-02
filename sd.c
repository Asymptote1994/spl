/* 
SD 操作

串行时钟线同步 5 根数据线上信息的采样和移位。传输频率是通过设置 SDIPRE 寄存器相应位来控制。可以更
改其频率来调整波特率数据寄存器值。

编程过程（通用）
编程 SDI 模块需要以下几个基本步骤：

初始化
1. 设置 SDICON 配制适当的时钟和中断使能。
2. 设置 SDIPRE 配制为适当值。
3. 等待为初始化卡的 74 个 SDCLK 时钟周期。

CMD 通路编程
1. 写 32 位命令参数给 SDICmdArg。
2. 决定命令类型并设置 SDICmdCon 来启动命令发送。
3. 当 SDICmdSta 的特定标志置位时确认 SDICMD 通路操作的结束。
4. 如果命令类型为无响应时标志为 CmdSent。
5. 如果命令类型带响应时标志为 RspFin。
6. 通过写'1'到相应位来清除 SDICmdSta 的标志。

DAT 通路编程
1. 写数据超时时间到 SDIDTimer。
2. 写块大小（块长度）到 SDIBSize（通常为 0x80 个字）
3. 确定块方式，宽总线或 DMA 等并通过设置 SDIDatCon 启动数据传输。
4. Tx 数据→写数据到数据寄存器（SDIDAT），其中 Tx FIFO 为可用（TFDET 为置位），或一半（TFHalf 为置位），
	或空（TFEmpty 为置位）。
5. Rx 数据→从数据寄存器（SDIDAT）读数据，其中 Rx FIFO 为可用（RFDET 为置位），或满（RFFull 为置位），
	或一半（RFHalf 为置位），再或最后数据就绪（RFLast 为置位）。
6. 当 SDIDatSta 的 DatFin 标志置位时确认 SDI DAT 通路操作的结束。
7. 通过写'1'到相应位来清除 SDIDatSta 的标志。 

*/


/*
// SD卡指令表  	   
#define CMD0    0       //卡复位
#define CMD1    1
#define CMD8    8       //命令8 ，SEND_IF_COND
#define CMD9    9       //命令9 ，读CSD数据
#define CMD10   10      //命令10，读CID数据
#define CMD12   12      //命令12，停止数据传输
#define CMD16   16      //命令16，设置SectorSize 应返回0x00
#define CMD17   17      //命令17，读sector
#define CMD18   18      //命令18，读Multi sector
#define CMD23   23      //命令23，设置多sector写入前预先擦除N个block
#define CMD24   24      //命令24，写sector
#define CMD25   25      //命令25，写Multi sector
#define CMD41   41      //命令41，应返回0x00
#define CMD55   55      //命令55，应返回0x01
#define CMD58   58      //命令58，读OCR信息
#define CMD59   59      //命令59，使能/禁止CRC，应返回0x00
*/

#include <sd.h>
#include <s3c2440_regs.h>

#define _SD_printk_
#define SDCARD_BUFF_SIZE 512
 
u8 send_buf[SDCARD_BUFF_SIZE];
u8 recv_buf[SDCARD_BUFF_SIZE];
struct sd_info SDCard;
 
int sd_test()  //测试函数
{
	u32 i;

	/* 初始化sd */
	if (sd_init()) {
		printk("SDI初始化结束！\r\n");
	} else {
		printk("SDI初始化出错，终止！\r\n");
		return -1;
	}

	/* 发送/接收缓冲区初始化 */
	for (i = 0; i < 512; i++) {
		send_buf[i] = i + 1;
		recv_buf[i] = 0;
	}

	/* 写数据到sd */
	if (sd_write_sector((u32 *)send_buf, 0, 1)) {
		printk("\r\n写入SD卡0地址数据成功！");
	} else {
		printk("\r\n写入SD卡0地址数据出错，终止！");
		return -1;
	}
	
	/* 从sd读数据 */
	if (sd_read_sector((u32 *)recv_buf, 0, 1)) {
		printk("\r\n读出SD卡0地址数据成功！");
		printk("\r\n读出的数据:\r\n");

		for (i = 0; i < 512; i += 4) {
			if (i % 32)
				;printk("\r\n");
			
			printk("%d ",recv_buf[i]);
			printk("%d ",recv_buf[i+1]);
			printk("%d ",recv_buf[i+2]);
			printk("%d ",recv_buf[i+3]);
		}
	} else {
		printk("\r\n读出SD卡0地址数据出错，终止！");
		return -1;
	}

	/* 取消片选 */
//	select_or_deselect(0,SDCard.sdiRCA);

	return 0;
}

/*
 *
 *
 *
 *
 *
 *
 */
u8 sd_init()
{
	int i;
	
	printk("\r\nSDI初始化开始！");
	
	GPEUP  = 0xf83f;   			// The pull up  1111 1000 0011 1111  必须上拉
    GPECON = 0xaaaaaaaa;  		// 1010 1010 1010 1010 1010 1010 1010 1010
    SDICSTA = 0xffff;   		//SDI指令状态
    SDIDSTA = 0xffff;			//SDI数据状态
 
	SDIPRE = 124; 				// 400KHz  波特率设置 频率 PCLK/400K -1
    SDICON = 1; 			// Type A,  clk enable SDI控制
    SDIFSTA |= 1 << 16; 			//FIFO reset
	SDIBSIZE = 0x200; 			// 512byte(128word)  SDI块大小
	SDIDTIMER = 0x7fffff; 		// Set timeout count 数据传输超时时间
	
	//等待74个CLK
    for(i=0;i<0x1000;i++);
	
	//先执行CMD0,复位
	CMD0(); 
	
	//判断卡的类型
	if(SDI_MMC_OCR()) 
		SDCard.sdiType = 1;  //卡为MMC
	else
		SDCard.sdiType = 0;  //卡为SD
		
	//检测SD卡
	if (SDI_SD_OCR()) {
		printk("SD is ready\r\n");
	} else{
		printk("Initialize fail\r\nNo Card assertion\r\n");
        return 0;
    }  
	
	//读CID
	if (CMD2(SDCard.cCardCID)) {
		printk("CID\r\n");
		printk("MID = %d\r\n",SDCard.cCardCID[0]);
		printk("OLD = %d\r\n",(SDCard.cCardCID[1]*0X100)+SDCard.cCardCID[2]);
		printk("生产厂家:%s\r\n",(SDCard.cCardCID+3));
		printk("生产日期:20%d,%d\r\n",((SDCard.cCardCID[13]&0x0f)<<4)+((SDCard.cCardCID[14]&0xf0)>>4),(SDCard.cCardCID[14]&0x0f));
	} else {
		printk("Read Card CID is fail!\r\n");
		return 0;
	}
	
	//设置RCA       MMC的RCA=1   SD的RCA=0
	//MMC
	if (SDCard.sdiType==1) {
		if (CMD3(1,&SDCard.sdiRCA)) {
			SDCard.sdiRCA = 1;
			SDIPRE = 2;  //16MHZ
			printk("MMC Card RCA = 0x%x\r\n",SDCard.sdiRCA);
			// printk("MMC Frequency is %dHz\r\n",(PCLK/(SDIPRE+1)));
		} else {
			printk("Read MMC RCA is fail!\r\n");
			return 0;
		}
	//SD
	} else {
		if (CMD3(0,&SDCard.sdiRCA)) {
			SDIPRE = 1; // Normal clock=25MHz
			printk("SD Card RCA = 0x%x\r\n",SDCard.sdiRCA);
			// printk("SD Frequency is %dHz\r\n",(PCLK/(SDIPRE+1)));
		} else {
			printk("Read SD RCA is fail!\r\n");
			return 0;
		}
	}
	
	//读CSD
	if (CMD9(SDCard.sdiRCA,SDCard.lCardCSD)) {
		SDCard.lCardSize = (((SDCard.lCardCSD[1]&0x0000003f)<<16)+((SDCard.lCardCSD[2]&0xffff0000)>>16)+1)*512;
		SDCard.lSectorSize = ((SDCard.lCardCSD[2]>>6)&0x0000007f)+1;
		printk("Read Card CSD OK!\r\n");
		printk("0x%08x\r\n",SDCard.lCardCSD[0]);
		printk("0x%08x\r\n",SDCard.lCardCSD[1]);
		printk("0x%08x\r\n",SDCard.lCardCSD[2]);
		printk("0x%08x\r\n",SDCard.lCardCSD[3]);
		printk("卡容量为:%dKB,%dMB\r\n",SDCard.lCardSize,SDCard.lCardSize/1024);
	} else {
		printk("Read Card CSD Fail!\r\n");
		return 0;
	}
	
	//选中卡  CMD7  进入传输状态
	//1表示选中卡 
	if (select_or_deselect(1,SDCard.sdiRCA)) {
		printk("Card sel desel OK!\r\n");
	} else {
		printk("Card sel desel fail!\r\n");
		return 0;
	}
	
	//CMD13 查询是否为传输状态
	while ((CMD13(SDCard.sdiRCA) & 0x1e00) != 0x800);

	//设置总线带宽 ACMD6
	if (Set_bus_Width(SDCard.sdiType,1,SDCard.sdiRCA)) {
		SDCard.sdiWide = 1;
		printk("Bus Width is 4bit\r\n");
	} else {
		SDCard.sdiWide = 0;
		printk("Bus Width is 1bit\r\n");
	}
	
	return 1;
}
 
//检查CMD是否结束
u32 SDI_Check_CMD_End(int cmd, int be_resp) 
{
    int finish0;

	// No response   
    if (!be_resp) {
     	finish0 = SDICSTA;		

		while ((finish0 & 0x800) != 0x800) // Check cmd end		
   			finish0 = SDICSTA;
 
		SDICSTA = finish0;		// Clear cmd end state
	 
		printk("%x\r\n", finish0);
		return 1;
		
	// With response
	} else {
	    finish0 = SDICSTA;

		while (!( ((finish0 & 0x200) == 0x200) | ((finish0 & 0x400) == 0x400) ))    // Check cmd/rsp end
	        finish0=SDICSTA;
	 
//		printk("CMD%d:SDICSTA=0x%x, SDIRSP0=0x%x\r\n", cmd, SDICSTA, SDIRSP0);   

		// CRC no check
		if (cmd==1 | cmd==9 | cmd==41) {
		   	// Check error
		   	if ( (finish0&0xf00) != 0xa00 ) {
				SDICSTA=finish0;   // Clear error state 
			 
				if (((finish0&0x400)==0x400)) {
					printk("CMD%d Time out!\r\n", cmd);
				    return 0; // Timeout error     
				}
			}
			
			SDICSTA=finish0; // Clear cmd & rsp end state
			// printk("%x\r\n", finish0);

		// CRC check
		} else {
			// Check error
			if ( (finish0&0x1f00) != 0xa00 ) { 
				SDICSTA=finish0;   // Clear error state
			 
				if (((finish0&0x400)==0x400)) {
					printk("CMD%d Time out!\r\n", cmd);
				    return 0; // Timeout error
				}
			}
			SDICSTA=finish0;
		}
		
		return 1;
	}

	return 1;
}
 
//复位，使卡进入IDEL状态
void CMD0(void)
{
	SDICARG = 0x0; 
	SDICCON = (1<<8)|0x40; // No_resp, start
 
	SDI_Check_CMD_End(0, 0);
	SDICSTA = 0x800; // Clear cmd_end(no rsp)
}
 
//设置工作电压是根据SD的OCR寄存器来设置
u8 CMD1(void)
{
	SDICARG = 0xff8000; //(SD OCR:2.7V~3.6V)
	SDICCON = (0x1<<9)|(0x1<<8)|0x41; //sht_resp, wait_resp, start, 
	 
	if(SDI_Check_CMD_End(1, 1)) //[31]:Card Power up status bit (busy)
	{
	if((SDIRSP0>>16)==0x80ff)
	{
	SDICSTA = 0xa00; // Clear cmd_end(with rsp)
	return 1; // Success
	}
	else
	return 0;
	}
	return 0;
}

//请求设备在CMD上传送CID
u8 CMD2(u8 *cCID_Info) 
{
	SDICARG = 0x0;
	SDICCON = (0x1<<10)|(0x1<<9)|(0x1<<8)|0x42; //lng_resp, wait_resp, start
	 
	if(!SDI_Check_CMD_End(2, 1)) 
	return 0;
	*(cCID_Info+0) = SDIRSP0>>24;
	*(cCID_Info+1) = SDIRSP0>>16;
	*(cCID_Info+2) = SDIRSP0>>8;
	*(cCID_Info+3) = SDIRSP0;
	*(cCID_Info+4) = SDIRSP1>>24;
	*(cCID_Info+5) = SDIRSP1>>16;
	*(cCID_Info+6) = SDIRSP1>>8;
	*(cCID_Info+7) = SDIRSP1;
	*(cCID_Info+8) = SDIRSP2>>24;
	*(cCID_Info+9) = SDIRSP2>>16;
	*(cCID_Info+10) = SDIRSP2>>8;
	*(cCID_Info+11) = SDIRSP2;
	*(cCID_Info+12) = SDIRSP3>>24;
	*(cCID_Info+13) = SDIRSP3>>16;
	*(cCID_Info+14) = SDIRSP3>>8;
	*(cCID_Info+15) = SDIRSP3;
	SDICSTA = 0xa00; // Clear cmd_end(with rsp)
	return 1;
}

//给SD卡设定一个相对地址，也就是寻址的地址 = 0:SD卡，=1:MMC卡 =0 失败 =1 成功
u8 CMD3(u16 iCardType,u16 *iRCA) 
{
	SDICARG = iCardType<<16;     // (MMC:Set RCA, SD:Ask RCA-->SBZ)
	SDICCON = (0x1<<9)|(0x1<<8)|0x43; // sht_resp, wait_resp, start
	 
	if(!SDI_Check_CMD_End(3, 1)) 
	return 0;
	SDICSTA=0xa00; // Clear cmd_end(with rsp)
	 
	if(iCardType)
	{
	*iRCA = 1;
	}
	else 
	    {
	*iRCA =( SDIRSP0 & 0xffff0000 )>>16;
	}
	if( SDIRSP0 & 0x1e00!=0x600 )   // CURRENT_STATE check
	return 0;
	else
	return 1;
}
 
//选中卡或者解除选中 cSorD=1为选中 为0则解除选中
u8 CMD7(u8 cSorD,u16 iRCA) 
{
	if(cSorD)
	{
		SDICARG = iRCA<<16; // (RCA,stuff bit)
		SDICCON = (0x1<<9)|(0x1<<8)|0x47;   // sht_resp, wait_resp, start
		if(!SDI_Check_CMD_End(7, 1))
		return 0;
		SDICSTA = 0xa00; // Clear cmd_end(with rsp)
		//--State(transfer) check
		if( SDIRSP0 & 0x1e00!=0x800 )
		return 0;
		else
		return 1;
	}
	else
	{
		SDICARG = 0<<16; //(RCA,stuff bit)
		SDICCON = (0x1<<8)|0x47; //no_resp, start
		
		if(!SDI_Check_CMD_End(7, 0))
		return 0;
		SDICSTA = 0x800; //Clear cmd_end(no rsp)
		return 1;
	}
}

//获取卡的CSD寄存器的值
u8 CMD9(u16 iRCA,u32 *lCSD) 
{
	SDICARG = iRCA<<16; // (RCA,stuff bit)
	SDICCON = (0x1<<10)|(0x1<<9)|(0x1<<8)|0x49; // long_resp, wait_resp, start
	 
	if(!SDI_Check_CMD_End(9, 1)) 
	return 0;
	 
	*(lCSD+0) = SDIRSP0;
	*(lCSD+1) = SDIRSP1;
	*(lCSD+2) = SDIRSP2;
	*(lCSD+3) = SDIRSP3;
	return 1;
}
 
//停止数据传输
u8 CMD12(void) 
{
	SDICARG = 0x0;    
	SDICCON = (0x1<<9)|(0x1<<8)|0x4c; //sht_resp, wait_resp, start,
	 
	if(!SDI_Check_CMD_End(12, 1)) 
	return 0;
	else
	SDICSTA = 0xa00; //Clear cmd_end(with rsp)
	return 1;
}

//获取卡内状态
u16 CMD13(u16 iRCA) 
{
	SDICARG = iRCA<<16; // (RCA,stuff bit)
	SDICCON = (0x1<<9)|(0x1<<8)|0x4d; // sht_resp, wait_resp, start
	 
	if(!SDI_Check_CMD_End(13, 1)) 
	return 0;
	 
	SDICSTA=0xa00; // Clear cmd_end(with rsp)
	return SDIRSP0;
}
 
//读取一个数据块
u8 CMD17(u32 addr) 
{
    //STEP1:发送指令 
    SDICARG = addr; //设定指令参数 
    SDICCON = (1<<9)|(1<<8)|0x51; //发送CMD17指令
    
    if(SDI_Check_CMD_End(17,1))
     return 1;
    else
     return 0;
}
 
//读取多个数据块
u8 CMD18(u32 addr) 
{
    //STEP1:发送指令 
    SDICARG = addr; //设定指令参数 
    SDICCON = (1<<9)|(1<<8)|0x52; //发送CMD18指令
    
    if(SDI_Check_CMD_End(18,1))
     	return 1;
    else
     	return 0;
}
 
//写入一个数据块
u8 CMD24(u32 addr) 
{
    //STEP1:发送指令 
    SDICARG = addr; //设定指令参数 
    SDICCON = (1 << 9) | (1 << 8) | 0x58; //发送CMD24指令
    
    if (SDI_Check_CMD_End(24, 1))
     	return 1;
    else
     	return 0;
}

//写入多个数据块
u8 CMD25(u32 addr) 
{
    //STEP1:发送指令 
    SDICARG = addr; //设定指令参数 
    SDICCON = (1<<9)|(1<<8)|0x59; //发送CMD25指令
    
    if(SDI_Check_CMD_End(25,1))
     return 1;
    else
     return 0;
}

//检测是否有卡，执行ACMD必须先执行CMD55
u8 CMD55(u16 iRCA) 
{
	SDICARG = iRCA<<16;
	SDICCON = (0x1<<9)|(0x1<<8)|0x77; //sht_resp, wait_resp, start
	 
	if(!SDI_Check_CMD_End(55, 1)) 
	return 0;
	SDICSTA = 0xa00; // Clear cmd_end(with rsp)
	return 1;
}

// ACMD6命令为设置总线带宽 [1:0] 00为1bit  10为4bit
u8 ACMD6(u8 BusWidth,u16 iRCA) 
{
	if(!CMD55(iRCA))
	return 0;
	 
	SDICARG = BusWidth<<1;     //Wide 0: 1bit, 1: 4bit
	SDICCON = (0x1<<9)|(0x1<<8)|0x46; //sht_resp, wait_resp, start
	 
	if(!SDI_Check_CMD_End(6, 1)) 
	return 0;
	SDICSTA=0xa00;     // Clear cmd_end(with rsp)
	return 1;
}
 
//检测是否为SD卡及类型 =0应答错误或者卡正忙  =1标准SD卡 =2SDHC V2.0
u8 ACMD41(u16 iRCA) 
{
	u8 cReturn;
	if(!CMD55(iRCA)) 
	return 0;
	SDICARG=0x40ff8000; //ACMD41(SD OCR:2.7V~3.6V)
	SDICCON=(0x1<<9)|(0x1<<8)|0x69;//sht_resp, wait_resp, start, ACMD41
	 
	if(SDI_Check_CMD_End(41, 1)) 
	{
	if(SDIRSP0==0xc0ff8000)
	cReturn = 2; //SDHC
	else if(SDIRSP0==0x80ff8000)
	cReturn = 1; //标准SD
	else
	cReturn = 0; //应答错误
	SDICSTA = 0xa00; // Clear cmd_end(with rsp)
	return cReturn; // Success    
	}
	SDICSTA = 0xa00; // Clear cmd_end(with rsp)
	return 0;
}

//检测MMC卡
u32 SDI_MMC_OCR(void)
{
    int i;
    //-- Negotiate operating condition for MMC, it makes card ready state
    for(i=0;i<10;i++)
    {
		if(CMD1())
		return 1;
    }
    return 0; // Fail
}

//检测SD卡
u32 SDI_SD_OCR(void)
{
    int i;
 
    SDCard.sdiRCA = 0;
    for(i=0;i<50;i++)
    {
		if(ACMD41(SDCard.sdiRCA))
		return 1;
		file_delay(1000);
	 }
	return 0;  //fail
}

//运用CMD7来选中或解除选中卡，返回1则成功，0失败
u8 select_or_deselect(u8 cSelDesel,u16 iCardRCA)
{
	if (CMD7(cSelDesel,iCardRCA))
		return 1;
	else
		return 0;
}

//设置总线带宽
u8 Set_bus_Width(u8 cCardType,u8 cBusWidth,u16 iRCA)
{
	if(cCardType==1) //MMC，返回0不需要设置 默认为1bit总线带宽
		return 0;
	return ACMD6(cBusWidth,iRCA);
}
 
#define be32_to_le32(x) \
	((unsigned int)( \
		(((unsigned int)(x) & (unsigned int)0x000000ffUL) << 24) | \
		(((unsigned int)(x) & (unsigned int)0x0000ff00UL) <<  8) | \
		(((unsigned int)(x) & (unsigned int)0x00ff0000UL) >>  8) | \
		(((unsigned int)(x) & (unsigned int)0xff000000UL) >> 24) ))
 
/*
 * sd_read_sector - 从SD卡中读出指定块起始地址的单个或多个数据块
 *
 * @addr  被读块的起始地址
 * @buffer 用于接收读出数据的缓冲区
 * @block_num 读的块数
 *
 * @return 
 *		0 读块操作不成功
 *		1 读块操作成功
 *
 */
u8 sd_read_sector(u32 *buf, u32 addr, u32 block_num)
{
	u32 i=0;
	u32 status=0;
	 
	SDIDTIMER = 0x7fffff; 				// Set timeout count
	SDIBSIZE = 0x200; 					// 512byte(128word)
	SDIFSTA = SDIFSTA | (1 << 16); 		// FIFO reset
	SDIDCON = (block_num << 0) | (2 << 12) | (1 << 14) | (SDCard.sdiWide << 16) | (1 << 17) | (1 << 19) | (2 << 22);

//	printk("enter sd_read_sector(): src_block = %d, block_num = %d\r\n", addr, block_num);
	
	//发送读多个块指令
	while (CMD18(addr) != 1)		
		SDICSTA = 0xF << 9;

	//开始接收数据到缓冲区
	while (i < block_num * 128) { 
		//检查是否超时和CRC校验是否出错
		if (SDIDSTA & 0x60) { 
			//清除超时标志和CRC错误标志
			SDIDSTA = (0x3 << 0x5); 
			return -1;
		}
		
		status = SDIFSTA;
		//如果接收FIFO中有数据
		if ((status & 0x1000) == 0x1000) { 
			*buf = SDIDAT;
			// *buf = be32_to_le32(*buf);
			buf++;
			i++;
		}
	}
	 
	SDIDCON = SDIDCON & ~(7 << 12);
	SDIFSTA = SDIFSTA & 0x200;			//Clear Rx FIFO Last data Ready 
	SDIDSTA = 0x10;						//Clear data Tx/Rx end detect 

	//发送结束指令 
	while (CMD12() != 1)				
		SDICSTA = 0xF << 9;
//	printk("leave sd_read_sector(): src_block = %d, block_num = %d\r\n", addr, block_num);
	 
	return 0;
}

 
/*
 * sd_write_sector - 向SD卡的一个或多个数据块写入数据
 *
 * @addr  被写块的起始地址
 * @buffer 用于发送数据的缓冲区
 * @block_num 块数
 *
 * @return 
 *		0 数据写入操作失败
 *		1 数据写入操作成功
 *
 */
u8 sd_write_sector(u32 *buf, u32 addr, u32 block_num)
{
	u16 i = 0;
	u32 status = 0;
	 
	SDIDTIMER = 0x7fffff; 				// Set timeout count
	SDIBSIZE = 0x200; 					// 512byte(128word)
	SDIFSTA = SDIFSTA | (1 << 16); 		// FIFO reset
	SDIDCON = (block_num << 0) | (3 << 12) | (1 << 14) | (1 << 16) | (1 << 17) | (1 << 20) | (2 << 22);

	//发送写多个块指令
	while (CMD25(addr) != 1)
		SDICSTA = 0xF << 9;

	//开始传递数据到缓冲区
	while (i < block_num * 128) { 
		status = SDIFSTA;
		
		//如果发送FIFO可用，即FIFO未满
		if ((status & 0x2000) == 0x2000) {
			SDIDAT = *buf;
			buf++;
			i++;
		}
	}
	
	SDIDCON = SDIDCON & ~(7 << 12);

	//发送结束指令 
	while (CMD12() != 1)
		SDICSTA=0xF<<9;

	//等待数据发送结束
	do { 
		status = SDIDSTA;
	} while ((status & 0x2) == 0x2);

	SDIDSTA = status; 
	SDIDSTA = 0xf4;

	return 0;
}
 
void file_delay(u32 i)
{
	while (i--);
}

