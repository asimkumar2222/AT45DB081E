#define SetBit(x,y)		(x |= (y))
#define ClrBit(x,y)		(x &=~(y))
#define ChkBit(x,y)		(x  & (y))
//df opcodes
#define FlashPageRead			0x52	// Main memory page read
#define FlashToBuf1Transfer 		0x53	// Main memory page to buffer 1 transfer
#define Buf1Read			0x54	// Buffer 1 read
#define FlashToBuf2Transfer 		0x55	// Main memory page to buffer 2 transfer
#define Buf2Read			0x56	// Buffer 2 read
#define StatusReg			0xD7   //0x57	// Status register
#define AutoPageReWrBuf1		0x58	// Auto page rewrite through buffer 1
#define AutoPageReWrBuf2		0x59	// Auto page rewrite through buffer 2
#define FlashToBuf1Compare    		0x60	// Main memory page to buffer 1 compare
#define FlashToBuf2Compare	    	0x61	// Main memory page to buffer 2 compare
#define ContArrayRead			0x68	// Continuous Array Read (Note : Only A/B-parts supported)
#define FlashProgBuf1			0x82	// Main memory page program through buffer 1
#define Buf1ToFlashWE   		0x83	// Buffer 1 to main memory page program with built-in erase
#define Buf1Write			0x84	// Buffer 1 write
#define FlashProgBuf2			0x85	// Main memory page program through buffer 2
#define Buf2ToFlashWE   		0x86	// Buffer 2 to main memory page program with built-in erase
#define Buf2Write			0x87	// Buffer 2 write
#define Buf1ToFlash     		0x88	// Buffer 1 to main memory page program without built-in erase
#define Buf2ToFlash		        0x89	// Buffer 2 to main memory page program without built-in erase
#define PageErase                   0x81	// Page erase, added by Martin Thomas
#define Devid                   0x9F
#define Mainwrite               0x02 


#include <inttypes.h>
//#include "WConstants.h"
#include <Arduino.h>
#include <avr/pgmspace.h>


const uint8_t DF_pagebits[] PROGMEM ={9,  9,  9,  9,  9,  10,  10,  11};
const uint16_t DF_pagesize[] PROGMEM ={264,264, 264, 264, 264, 528, 528,1056};
class df {
public:
df(void);
void init(void);
void Chip_Erase (void); //Added by Asim
void Main_Write_Byte (unsigned int IntPageAdr, unsigned int PageAdr, unsigned char Data);  //Added asim
void Main_Write_Str (unsigned int IntPageAdr, unsigned int PageAdr, unsigned int No_of_bytes, char *BufferPtr); //Added by asim
void Update_str (unsigned int IntPageAdr, unsigned int PageAdr, unsigned int No_of_bytes, char *BufferPtr); //added by asim(using buffer1)

void Page_To_Buffer (unsigned int PageAdr, unsigned char BufferNo);
unsigned char Buffer_Read_Byte (unsigned char BufferNo, unsigned int IntPageAdr);
void Buffer_Read_Str (unsigned char BufferNo, unsigned int IntPageAdr, unsigned int No_of_bytes, unsigned char *BufferPtr);
void Buffer_Write_Enable (unsigned char BufferNo, unsigned int IntPageAdr);
void Buffer_Write_Byte (unsigned char BufferNo, unsigned int IntPageAdr, unsigned char Data);
void Buffer_Write_Str (unsigned char BufferNo, unsigned int IntPageAdr, unsigned int No_of_bytes, unsigned char *BufferPtr);
void Buffer_To_Page (unsigned char BufferNo, unsigned int PageAdr);
void Cont_Flash_Read_Enable (unsigned int PageAdr, unsigned int IntPageAdr);
//#ifdef MTEXTRAS
void Page_Erase (unsigned int PageAdr); // added by mthomas
unsigned char Page_Buffer_Compare(unsigned char BufferNo, unsigned int PageAdr); // added by mthomas

unsigned char Read_DF_status (void);
void devid(void);

private:
unsigned char DF_SPI_RW (unsigned char output);
void DF_SPI_init (void);
//unsigned char Read_DF_status (void);
void DF_CS_inactive(void);
void DF_CS_active(void);

};
// *****************************[ End Of df.H ]*****************************
