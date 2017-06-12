#include "df.h"

/* #define DATAOUT 51//MOSI
#define DATAIN  50//MISO 
#define SPICLOCK  52//sck
#define SLAVESELECT 53//ss */

#define DATAOUT 11//MOSI
#define DATAIN  12//MISO 
#define SPICLOCK  13//sck
#define SLAVESELECT 10//ss

//opcodes
#define WREN  6
#define WRDI  4
#define RDSR  5
#define WRSR  1
#define READ  3
#define WRITE 2

unsigned char PageBits = 0;
unsigned int  PageSize = 0;

df::df(void)
{
}

void df::init(void)
{
  char clr;
  pinMode(DATAOUT, OUTPUT);
  pinMode(DATAIN, INPUT);
  pinMode(SPICLOCK,OUTPUT);
  pinMode(SLAVESELECT,OUTPUT);
  digitalWrite(SLAVESELECT,HIGH); //disable device
  // SPCR = 01010000
  //interrupt disabled,spi enabled,msb 1st,master,clk low when idle,
  //sample on leading edge of clk,system clock/4 rate (fastest)
 // SPCR = (1<<SPE)|(1<<MSTR);
 //SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPHA);
//   SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPOL); //nno
//	 SPCR = 0x5C; //(1<<SPE)|(1<<MSTR)|(1<<CPHA)|(1<<CPOL);
	 
  SPCR = (1<<SPE)|(1<<MSTR)|(1<<CPHA)|(1<<CPOL);	 
 // SPSR = (1<<SPI2X); //double speed??
  clr=SPSR;
  clr=SPDR;
  //delay(100); //FIXME
}
// *****************************[ Start Of df.C ]*************************
unsigned char df::DF_SPI_RW(unsigned char output)
{
  SPDR = output;                    // Start the transmission
  while (!(SPSR & (1<<SPIF)))     // Wait the end of the transmission
  {
  };
  return SPDR; 
}


void df::devid(void)
{
  unsigned char result,result2,result3,result4,index_copy;
  DF_CS_inactive();
  DF_CS_active();				//to reset df command decoder
  result = DF_SPI_RW(Devid);		//send status register read op-code
  result = DF_SPI_RW(0x00);			//dummy write to get result
  result2 = DF_SPI_RW(0x00); result3 = DF_SPI_RW(0x00); result4 = DF_SPI_RW(0x00);
 
  DF_CS_inactive();				//make sure to toggle CS signal in order
  
  Serial.println("DevID");
  
  Serial.print(result);Serial.print(":"); Serial.print(result2);Serial.print(":"); Serial.print(result3);Serial.print(":"); Serial.print(result4);
  //return result;				//return the read status register value
}



unsigned char df::Read_DF_status (void)
{
  unsigned char result,result2,index_copy;
  result=0x00;
  result2=0x00;
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
  result = DF_SPI_RW(StatusReg);		//send status register read op-code
  result = DF_SPI_RW(0x00);			//dummy write to get result
  result2 = DF_SPI_RW(0x00);	
  DF_CS_inactive();	
  
 //Serial.println(result,HEX);
 // Serial.println(result2,HEX);
 
  index_copy = ((result & 0x38) >> 3);	//get the size info from status register
  // mtA
  /// if (!PageBits) { // mt 200401
  // PageBits   = DF_pagebits[index_copy];	//get number of internal page address bits from look-up table
  // PageSize   = DF_pagesize[index_copy];   //get the size of the page (in bytes)
  //pagesize lijkt nergens gebruikt te worden
  PageBits   = pgm_read_byte(&DF_pagebits[index_copy]);	//get number of internal page address bits from look-up table
  PageSize   = pgm_read_word(&DF_pagesize[index_copy]);   //get the size of the page (in bytes)
  /// }
  // mtE
// DF_CS_inactive();				//make sure to toggle CS signal in order
//  DF_CS_active();
 // Serial.println(result); Serial.println(result2);
  return result;				//return the read status register value
}
/*****************************************************************************
 * 
 *	Function name : Page_To_Buffer
 *  
 *	Returns :		None
 *  
 *	Parameters :	BufferNo	->	Decides usage of either buffer 1 or 2
 * 
 *			PageAdr		->	Address of page to be transferred to buffer
 * 
 *	Purpose :	Transfers a page from flash to df SRAM buffer
 * 					
 * 
 ******************************************************************************/
void df::Page_To_Buffer (unsigned int PageAdr, unsigned char BufferNo)
{
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
  if (1 == BufferNo)				//transfer flash page to buffer 1
  {
    DF_SPI_RW(FlashToBuf1Transfer);				//transfer to buffer 1 op-code
    DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address
    DF_SPI_RW(0x00);						//don't cares
  }
#ifdef USE_BUFFER2
  else	
    if (2 == BufferNo)						//transfer flash page to buffer 2
  {
    DF_SPI_RW(FlashToBuf2Transfer);				//transfer to buffer 2 op-code
    DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address
    DF_SPI_RW(0x00);						//don't cares
  }
#endif
  DF_CS_inactive();						//initiate the transfer
  DF_CS_active();
  while(!(Read_DF_status() & 0x80));				//monitor the status register, wait until busy-flag is high
  
  DF_CS_inactive(); //asim
}
/*****************************************************************************
 *  
 *	Function name : Buffer_Read_Byte
 *  
 *	Returns :		One read byte (any value)
 *
 *	Parameters :	BufferNo	->	Decides usage of either buffer 1 or 2
 * 
 *					IntPageAdr	->	Internal page address
 *  
 *	Purpose :		Reads one byte from one of the df
 * 
 *					internal SRAM buffers
 * 
 ******************************************************************************/
unsigned char df::Buffer_Read_Byte (unsigned char BufferNo, unsigned int IntPageAdr)
{
  unsigned char data;
  data='0'; // mt 
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
  if (1 == BufferNo)				//read byte from buffer 1
  {
    DF_SPI_RW(Buf1Read);			//buffer 1 read op-code
    DF_SPI_RW(0x00);				//don't cares
    DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
    DF_SPI_RW(0x00);				//don't cares
    data = DF_SPI_RW(0x00);			//read byte
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)				//read byte from buffer 2
    {
      DF_SPI_RW(Buf2Read);					//buffer 2 read op-code
      DF_SPI_RW(0x00);						//don't cares
      DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
      DF_SPI_RW(0x00);						//don't cares
      data = DF_SPI_RW(0x00);					//read byte
    }
#endif
DF_CS_inactive(); //asim
  return data;							//return the read data byte
}
/*****************************************************************************
 *  
 *	Function name : Buffer_Read_Str
 * 
 *	Returns :		None
 *  
 *	Parameters :	BufferNo	->	Decides usage of either buffer 1 or 2
 * 
 *					IntPageAdr	->	Internal page address
 * 
 *					No_of_bytes	->	Number of bytes to be read
 * 
 *					*BufferPtr	->	address of buffer to be used for read bytes
 * 
 *	Purpose :		Reads one or more bytes from one of the df
 * 
 *					internal SRAM buffers, and puts read bytes into
 * 
 *					buffer pointed to by *BufferPtr
 * 
 * 
 ******************************************************************************/
void df::Buffer_Read_Str (unsigned char BufferNo, unsigned int IntPageAdr, unsigned int No_of_bytes, unsigned char *BufferPtr)
{
  unsigned int i;
  DF_CS_inactive();						//make sure to toggle CS signal in order
  DF_CS_active();							//to reset df command decoder
  if (1 == BufferNo)						//read byte(s) from buffer 1
  {
    DF_SPI_RW(Buf1Read);				//buffer 1 read op-code
    DF_SPI_RW(0x00);					//don't cares
    DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
    DF_SPI_RW(0x00);					//don't cares
    for( i=0; i<No_of_bytes; i++)
    {
      *(BufferPtr) = DF_SPI_RW(0x00);		//read byte and put it in AVR buffer pointed to by *BufferPtr
      BufferPtr++;					//point to next element in AVR buffer
    }
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)					//read byte(s) from buffer 2
    {
      DF_SPI_RW(Buf2Read);				//buffer 2 read op-code
      DF_SPI_RW(0x00);					//don't cares
      DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
      DF_SPI_RW(0x00);					//don't cares
      for( i=0; i<No_of_bytes; i++)
      {
        *(BufferPtr) = DF_SPI_RW(0x00);		//read byte and put it in AVR buffer pointed to by *BufferPtr
        BufferPtr++;					//point to next element in AVR buffer
      }
    }
#endif
DF_CS_inactive(); //asim
}
//NB : Sjekk at (IntAdr + No_of_bytes) < buffersize, hvis ikke blir det bare ball..
//mtA 
// translation of the Norwegian comments (thanks to Eirik Tveiten):
// NB : Check that (IntAdr + No_of_bytes) < buffersize, if not there will be problems
//mtE

/*****************************************************************************
 * 
 * 
 *	Function name : Buffer_Write_Enable
 * 
 *	Returns :		None
 *  
 *	Parameters :	IntPageAdr	->	Internal page address to start writing from
 * 
 *			BufferAdr	->	Decides usage of either buffer 1 or 2
 *  
 *	Purpose :	Enables continous write functionality to one of the df buffers
 * 
 *			buffers. NOTE : User must ensure that CS goes high to terminate
 * 
 *			this mode before accessing other df functionalities 
 * 
 ******************************************************************************/
void df::Buffer_Write_Enable (unsigned char BufferNo, unsigned int IntPageAdr)
{
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
  if (1 == BufferNo)				//write enable to buffer 1
  {
    DF_SPI_RW(Buf1Write);			//buffer 1 write op-code
    DF_SPI_RW(0x00);				//don't cares
    DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)				//write enable to buffer 2
    {
      DF_SPI_RW(Buf2Write);			//buffer 2 write op-code
      DF_SPI_RW(0x00);				//don't cares
      DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
    }
#endif
DF_CS_inactive(); //asim
}
/*****************************************************************************
 *  
 *	Function name : Buffer_Write_Byte
 * 
 *	Returns :		None
 *  
 *	Parameters :	IntPageAdr	->	Internal page address to write byte to
 * 
 *			BufferAdr	->	Decides usage of either buffer 1 or 2
 * 
 *			Data		->	Data byte to be written
 *  
 *	Purpose :		Writes one byte to one of the df
 * 
 *					internal SRAM buffers
 *
 ******************************************************************************/
void df::Buffer_Write_Byte (unsigned char BufferNo, unsigned int IntPageAdr, unsigned char Data)
{
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
  if (1 == BufferNo)				//write byte to buffer 1
  {
    DF_SPI_RW(Buf1Write);			//buffer 1 write op-code
    DF_SPI_RW(0x00);				//don't cares
    DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
    DF_SPI_RW(Data);				//write data byte
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)				//write byte to buffer 2
    {
      DF_SPI_RW(Buf2Write);			//buffer 2 write op-code
      DF_SPI_RW(0x00);				//don't cares
      DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
      DF_SPI_RW(Data);				//write data byte
    }		
#endif
DF_CS_inactive(); //asim
}
/*****************************************************************************
 * 
 * 
 *	Function name : Buffer_Write_Str
 *  
 *	Returns :		None
 * 
 *	Parameters :	BufferNo	->	Decides usage of either buffer 1 or 2
 * 
 *			IntPageAdr	->	Internal page address
 * 
 *			No_of_bytes	->	Number of bytes to be written
 * 
 *			*BufferPtr	->	address of buffer to be used for copy of bytes
 * 
 *			from AVR buffer to df buffer 1 (or 2)
 * 
 *	Purpose :		Copies one or more bytes to one of the df
 * 
 *				internal SRAM buffers from AVR SRAM buffer
 * 
 *			pointed to by *BufferPtr
 * 
 ******************************************************************************/
void df::Buffer_Write_Str (unsigned char BufferNo, unsigned int IntPageAdr, unsigned int No_of_bytes, unsigned char *BufferPtr)
{
  unsigned int i;
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
  if (1 == BufferNo)				//write byte(s) to buffer 1
  {
    DF_SPI_RW(Buf1Write);			//buffer 1 write op-code
    DF_SPI_RW(0x00);				//don't cares
    DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
    for( i=0; i<No_of_bytes; i++)
    {
      DF_SPI_RW(*(BufferPtr));			//write byte pointed at by *BufferPtr to df buffer 1 location
      BufferPtr++;				//point to next element in AVR buffer
    }
  }
#ifdef USE_BUFFER2
  else
    if (2 == BufferNo)				//write byte(s) to buffer 2
    {
      DF_SPI_RW(Buf2Write);			//buffer 2 write op-code
      DF_SPI_RW(0x00);				//don't cares
      DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
      DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
      for( i=0; i<No_of_bytes; i++)
      {
        DF_SPI_RW(*(BufferPtr));		//write byte pointed at by *BufferPtr to df buffer 2 location
        BufferPtr++;				//point to next element in AVR buffer
      }
    }
#endif
DF_CS_inactive(); //asim
}
//NB : Monitorer busy-flag i status-reg.
//NB : Sjekk at (IntAdr + No_of_bytes) < buffersize, hvis ikke blir det bare ball..
//mtA 
// translation of the Norwegian comments (thanks to Eirik Tveiten):
// NB : Monitors busy-flag in status-reg
// NB : Check that (IntAdr + No_of_bytes) < buffersize, if not there will be problems
//mtE
/*****************************************************************************
 * 
 * 
 *	Function name : Buffer_To_Page
 * 
 *	Returns :		None
 *  
 *	Parameters :	BufferAdr	->	Decides usage of either buffer 1 or 2
 * 
 *			PageAdr		->	Address of flash page to be programmed
 *  
 *	Purpose :	Transfers a page from df SRAM buffer to flash
 * 
 *			 
 ******************************************************************************/
void df::Buffer_To_Page (unsigned char BufferNo, unsigned int PageAdr)
{
  DF_CS_inactive();						//make sure to toggle CS signal in order
  DF_CS_active();						//to reset df command decoder
  if (1 == BufferNo)						//program flash page from buffer 1
  {
    DF_SPI_RW(Buf1ToFlashWE);					//buffer 1 to flash with erase op-code
    DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address
    DF_SPI_RW(0x00);										//don't cares
  }
#ifdef USE_BUFFER2
  else	
    if (2 == BufferNo)						//program flash page from buffer 2
  {
    DF_SPI_RW(Buf2ToFlashWE);					//buffer 2 to flash with erase op-code
    DF_SPI_RW((unsigned char)(PageAdr >> (16 - pPageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (pPageBits - 8)));	//lower part of page address
    DF_SPI_RW(0x00);						//don't cares
  }
#endif
  DF_CS_inactive();						//initiate flash page programming
  DF_CS_active();												
  while(!(Read_DF_status() & 0x80));				//monitor the status register, wait until busy-flag is high

  DF_CS_inactive(); //asim
  }
  
  //asim asim asim asim asim asim
  //*****************************************************************************************
  void df::Main_Write_Byte (unsigned int IntPageAdr, unsigned int PageAdr, unsigned char Data)
  {
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
 
    DF_SPI_RW(Mainwrite);			//buffer 1 write op-code
	DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address
    
    DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
    DF_SPI_RW(Data);				//write data byte

DF_CS_inactive(); //asim
}
  
  
  void df::Main_Write_Str (unsigned int IntPageAdr, unsigned int PageAdr, unsigned int No_of_bytes,  char *BufferPtr)
{
  unsigned int i;
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
 
    DF_SPI_RW(Mainwrite);			//Mainwrite op-code
    //DF_SPI_RW(0x00);				//don't cares
	DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address
    DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
    for( i=0; i<No_of_bytes; i++)
    {
      DF_SPI_RW(*(BufferPtr));			//write byte pointed at by *BufferPtr to df buffer 1 location
      BufferPtr++;				//point to next element in AVR buffer
    }
 
DF_CS_inactive(); //asim
}

 void df::Update_str (unsigned int IntPageAdr, unsigned int PageAdr, unsigned int No_of_bytes, char *BufferPtr) 
  {
  unsigned int i;
  DF_CS_inactive();				//make sure to toggle CS signal in order
  DF_CS_active();				//to reset df command decoder
 
    DF_SPI_RW(AutoPageReWrBuf1);			//AutoPageReWrBuf1 op-code is same as read-modify-write 
    //DF_SPI_RW(0x00);				//don't cares
	DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address
    DF_SPI_RW((unsigned char)(IntPageAdr>>8));//upper part of internal buffer address
    DF_SPI_RW((unsigned char)(IntPageAdr));	//lower part of internal buffer address
    for( i=0; i<No_of_bytes; i++)
    {
      DF_SPI_RW(*(BufferPtr));			//write byte pointed at by *BufferPtr to df buffer 1 location
      BufferPtr++;				//point to next element in AVR buffer
    }
 
DF_CS_inactive(); //asim
}
 //asim asim asim asim asim asim 
 //****************************************************************************** 
/*****************************************************************************
 * 
 * 
 *	Function name : Cont_Flash_Read_Enable
 * 
 *	Returns :		None
 * 
 *	Parameters :	PageAdr		->	Address of flash page where cont.read starts from
 * 
 *			IntPageAdr	->	Internal page address where cont.read starts from
 *
 *	Purpose :	Initiates a continuous read from a location in the df
 * 
 ******************************************************************************/
void df::Cont_Flash_Read_Enable (unsigned int PageAdr, unsigned int IntPageAdr)
{
  DF_CS_inactive();																//make sure to toggle CS signal in order
  DF_CS_active();																//to reset df command decoder
  DF_SPI_RW(ContArrayRead);													//Continuous Array Read op-code
  DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));			//upper part of page address
  DF_SPI_RW((unsigned char)((PageAdr << (PageBits - 8))+ (IntPageAdr>>8)));	//lower part of page address and MSB of int.page adr.
  DF_SPI_RW((unsigned char)(IntPageAdr));										//LSB byte of internal page address
  DF_SPI_RW(0x00);															//perform 4 dummy writes
  DF_SPI_RW(0x00);															//in order to intiate df
  DF_SPI_RW(0x00);															//address pointers
  DF_SPI_RW(0x00);
}

//#ifdef MTEXTRAS
/*****************************************************************************
 *  
 *	Function name : Page_Buffer_Compare
 *  
 *	Returns :		0 match, 1 if mismatch
 *  
 *	Parameters :	BufferAdr	->	Decides usage of either buffer 1 or 2
 * 
 *			PageAdr		->	Address of flash page to be compared with buffer
 * 
 *	Purpose :	comparte Buffer with Flash-Page
 *  
 *   added by Martin Thomas, Kaiserslautern, Germany. This routine was not 
 * 
 *   included by ATMEL
 * 
 ******************************************************************************/
unsigned char df::Page_Buffer_Compare(unsigned char BufferNo, unsigned int PageAdr)
{
  unsigned char stat;
  DF_CS_inactive();					//make sure to toggle CS signal in order
  DF_CS_active();					//to reset df command decoder
  if (1 == BufferNo)									
  {
    DF_SPI_RW(FlashToBuf1Compare);	
    DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address and MSB of int.page adr.
    DF_SPI_RW(0x00);	// "dont cares"
  }
#ifdef USE_BUFFER2
  else if (2 == BufferNo)											
  {
    DF_SPI_RW(FlashToBuf2Compare);						
    DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
    DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address
    DF_SPI_RW(0x00);						//don't cares
  }
#endif
  DF_CS_inactive();												
  DF_CS_active();		
  do {
    stat=Read_DF_status();
  } 
  while(!(stat & 0x80));					//monitor the status register, wait until busy-flag is high
  return (stat & 0x40);
}
/*****************************************************************************
 * 
 * 
 *	Function name : Page_Erase
 * 
 *	Returns :		None
 * 
 *	Parameters :	PageAdr		->	Address of flash page to be erased
 * 
 *	Purpose :		Sets all bits in the given page (all bytes are 0xff)
 * 
 *	function added by mthomas. 
 *
 ******************************************************************************/
void df::Page_Erase (unsigned int PageAdr)
{
  DF_CS_inactive();																//make sure to toggle CS signal in order
  DF_CS_active();																//to reset df command decoder
  DF_SPI_RW(PageErase);						//Page erase op-code
  DF_SPI_RW((unsigned char)(PageAdr >> (16 - PageBits)));	//upper part of page address
  DF_SPI_RW((unsigned char)(PageAdr << (PageBits - 8)));	//lower part of page address and MSB of int.page adr.
  DF_SPI_RW(0x00);	// "dont cares"
  DF_CS_inactive();						//initiate flash page erase
  DF_CS_active();
  while(!(Read_DF_status() & 0x80));				//monitor the status register, wait until busy-flag is high
DF_CS_inactive(); //asim
  }
 

 void df::Chip_Erase (void)
{
  DF_CS_inactive();																//make sure to toggle CS signal in order
  DF_CS_active();																//to reset df command decoder
  DF_SPI_RW(0xC7);						//Page erase op-code
 DF_SPI_RW(0x94);
 DF_SPI_RW(0x80);
 DF_SPI_RW(0x9A);
  DF_CS_inactive();						//initiate flash page erase
  DF_CS_active();
  while(!(Read_DF_status() & 0x80));				//monitor the status register, wait until busy-flag is high
DF_CS_inactive(); //asim
  }
  
  
  
  
// MTEXTRAS
//#endif
// *****************************[ End Of df.C ]*************************

void df::DF_CS_inactive()
{
  digitalWrite(SLAVESELECT,HIGH);
}
void df::DF_CS_active()
{
  digitalWrite(SLAVESELECT,LOW);
}
