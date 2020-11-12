#ifndef		_CAN_H
#define		_CAN_H

#include	"stm32f4xx_hal.h"
#include	"io.h"

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
														*_DBG;

extern		uint32_t					idDev,
														idPos,
														idCrc,
														debug;

#define 	_MAXDEV	4

void			*canRx(void *),
					*canTx(void *);

typedef struct  {
	uint32_t	t[4];
	GPIO_TypeDef *port;
	uint16_t	pin[4];
} led;

extern led Leds;

#define _RED(a)			Leds.t[0]=HAL_GetTick()+a
#define _GREEN(a)		Leds.t[1]=HAL_GetTick()+a
#define _BLUE(a)		Leds.t[2]=HAL_GetTick()+a
#define _YELLOW(a)	Leds.t[3]=HAL_GetTick()+a

typedef enum {    
	_ACK_LEFT_FRONT		=0x200,
	_ACK_RIGHT_FRONT	=0x201,
	_ACK_RIGHT_REAR		=0x202,
	_ACK_LEFT_REAR		=0x203,
	
	_THR_LEFT_FRONT		=0x210,
	_THR_RIGHT_FRONT	=0x211,
	_THR_RIGHT_REAR		=0x212,
	_THR_LEFT_REAR		=0x213,

	_ID_IAP_REQ				=0x214,
	
	_TEST_LEFT_FRONT	=0x220,
	_TEST_RIGHT_FRONT	=0x221,
	_TEST_RIGHT_REAR	=0x222,
	_TEST_LEFT_REAR		=0x223,

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
	DBG_CONSOLE
};

	enum tmode{    
	_DMA,
	_IT
};

#define	_REPEAT				0x6904BB59
#define	_VCP_CDC			0x5578913D
#define	_IAP_REQ			0x8407235F

#define	_LEFT_FRONT		0x2AA65689
#define	_RIGHT_FRONT	0xFBD9E4EB
#define	_RIGHT_REAR		0xDAF5192F
#define	_LEFT_REAR		0x0B8AAB4D

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
	uint32_t					timeout,to,crc,count;
	uint32_t					sum,scount;
} tim;

void				Send(int, payload *,int), iapRemote(void);
int					AckWait(int);
extern			uint32_t ackCount,ackMax;

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

