#ifndef		_CAN_H
#define		_CAN_H

#include	"stm32f4xx_hal.h"
#include	"io.h"

#define 	uS						84
#define		MIN_BURST			50					
#define		CRC_THRHOLD		300				
#define		MAX_BURST			100
#define		MAX__INT			20					
#define		MAX_FLUSH			90

#define 	N_CH0					8
#define 	N_CH1					8

#define		_REPEAT				0x6DC5A6EE
#define		_VCP_CDC			0x784183D7

#define		_LEFT_FRONT		0xD6E0F601
#define		_RIGHT_FRONT	0x079F4463
#define		_RIGHT_REAR		0x26B3B9A7
#define		_LEFT_REAR		0xF7CC0BC5

void			VCP_USB_DEVICE_Init(void),
					VCP_USB_DEVICE_DeInit(void),
					MSC_USB_DEVICE_Init(void),
					MSC_USB_DEVICE_DeInit(void),
					MX_USB_HOST_Init(void),
					MX_USB_HOST_DeInit(void),
					canFilterCfg(int, int, int, int);
					
extern		CAN_HandleTypeDef hcan2;
extern		CRC_HandleTypeDef hcrc;
extern 		TIM_HandleTypeDef htim1,
														htim8,
														htim3,
														htim4,
														htim5;

extern		_io								*_CAN,
														*_VCP,
														*_ITM,
														*_DBG,
														*canConsole;


extern		uint32_t					nDev,
														idDev,
														idPos,
														idCrc,
														debug,
														testmode;

void			*canRx(void *),
					*canTx(void *);

typedef struct  {
	uint32_t	t[4];
	GPIO_TypeDef *port;
	uint16_t	pin[4];
} led;
extern led	Leds;

typedef enum { false, true } bool;



#define _RED(a)			Leds.t[0]=HAL_GetTick()+a
#define _GREEN(a)		Leds.t[1]=HAL_GetTick()+a
#define _BLUE(a)		Leds.t[2]=HAL_GetTick()+a
#define _YELLOW(a)	Leds.t[3]=HAL_GetTick()+a

typedef enum {    
	_ACK_LEFT_FRONT		=0x200,
	_ACK_RIGHT_FRONT	=0x201,
	_ACK_RIGHT_REAR		=0x202,
	_ACK_LEFT_REAR		=0x203,
	
	_TEST_LEFT_FRONT	=0x210,
	_TEST_RIGHT_FRONT	=0x211,
	_TEST_RIGHT_REAR	=0x212,
	_TEST_LEFT_REAR		=0x213,

	_ID_IAP_REQ				=0x214,
	_TEST_DAC					=0x215,
	idCOM2CAN					=0x20C,
	idCAN2COM					=0x24C,

	_ID_IAP_GO				=0xA0,
	_ID_IAP_ERASE			=0xA1,
	_ID_IAP_ADDRESS		=0xA2,
	_ID_IAP_DWORD			=0xA3,
	_ID_IAP_ACK				=0xA4,
	_ID_IAP_SIGN			=0xA5,
	_ID_IAP_STRING		=0xA6,
	_ID_IAP_PING			=0xA7
} _StdId;


	enum dbg {    
	DBG_CAN_RX=0,
	DBG_CAN_TX,
	DBG_CRC,
	DBG_TIMING,
	DBG_LED,
	DBG_USEC,
	DBG_CONSOLE,
	DBG_STAT,
	DBG_VOLT
};

	enum tmode{    
	_DMA,
	_IT
};

typedef struct {
	unsigned mask:6;
	unsigned sect:2;
} test;

typedef union {
		uint8_t		byte[8];
		uint8_t		bytes[8];
		uint16_t	hword[4];
		uint32_t	word[2];
	} payload;

typedef struct {
	CAN_RxHeaderTypeDef hdr;
	payload buf;
} CanRxMsg;

typedef struct {
	CAN_TxHeaderTypeDef hdr;
	payload buf;
} CanTxMsg;

typedef struct {
	_buffer						*dma;
	TIM_HandleTypeDef *htim;
	uint32_t					Channel;
	uint8_t						sect,ch;
	enum tmode				tmode;				
	uint32_t					timeout,N,to,crc;
	uint32_t					cnt,longcnt,pw;
	uint32_t					hi,lo,shi,slo;
} tim;

void				Send(int, payload *,int);
int					AckWait(int);

#define			_SIGN_PAGE			FLASH_Sector_1
#define			_FLASH_TOP			0x08008000
#define			_BOOT_SECTOR		0x08000000
#define			FATFS_ADDRESS 	0x08040000
						                        
#define			_FW_START				((int *)(_FLASH_TOP-16))
#define			_FW_CRC					((int *)(_FLASH_TOP-20))
#define			_FW_SIZE				((int *)(_FLASH_TOP-24))
#define			_SIGN_CRC				((int *)(_FLASH_TOP-28))
#define			_FLASH_BLANK		((int)-1)
	
#define			SW_version			100
#endif

