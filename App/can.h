#ifndef		_CAN_H
#define		_CAN_H

#include	"stm32f4xx_hal.h"
#include	"io.h"

#define 	uS						84
#define		MIN_BURST			50					
#define		CRC_THRHOLD		300				
#define		MAX_BURST			120
#define		MAX__INT			20					
#define		FLUSH__INT		180

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

typedef struct {
	_buffer						*dma;
	TIM_HandleTypeDef *htim;
	uint32_t					Channel;
	uint8_t						sect,ch;
	enum {_DMA,_IT}		tmode;		
	GPIO_TypeDef*			gpio;
	uint16_t					pin;	
	uint32_t					timeout,to,tref,crc;
	uint32_t					cnt,longcnt,pw;
	uint32_t					hi,lo,shi,slo;
} tim;

void			*canRx(void *),
					*canTx(void *);

#define max(a,b) ({ __typeof__ (a) _a = (a);  __typeof__ (b) _b = (b);  _a > _b ? _a : _b; })
#define min(a,b) ({ __typeof__ (a) _a = (a);  __typeof__ (b) _b = (b);  _a < _b ? _a : _b; })

#define	_MAX_DEV		8
#define	_MAX_HEAD		4

typedef enum {    
	_ACK_LEFT_FRONT		=0x200,

	_TEST_LEFT_FRONT	=0x208,

	_TEST_REQ					=0x210,
	_REMOTE_REQ,
   idCOM2CAN,
	 idCAN2COM,
	_ID_IAP_REQ,
	_ID_SYNC_REQ,
	_ID_SYNC_ACK,
	_ID_RESET,

	_ID_IAP_GO				=0xA0,
	_ID_IAP_ERASE,
	_ID_IAP_ADDRESS,
	_ID_IAP_DWORD,
	_ID_IAP_ACK,
	_ID_IAP_SIGN,
	_ID_IAP_STRING,
	_ID_IAP_PING,
} _StdId;

typedef struct {
	unsigned mask:6;
	unsigned sect:2;
} test;


typedef union {
		__packed struct {
			__packed struct {
				uint8_t	ch:3, count:4, longpulse:1;
			} sect[4];
			uint32_t tref;
		} pulse;
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

void				Send(int, payload *,int);
int					AckWait(int);
uint16_t		sync(uint16_t);
int32_t			eval(int32_t);

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

					
extern		const char *			strPos[];
extern		CAN_HandleTypeDef hcan2;
extern		CRC_HandleTypeDef hcrc;
extern 		TIM_HandleTypeDef htim1,
														htim8,
														htim3,
														htim4,
														htim5,
														htim9;

extern		_io								*_CAN,
														*_VCP,
														*_ITM,
														*canConsole;


extern		uint32_t					nDev,
														idDev,
														idPos,
														idCrc,
														testMask,
														testReq,
														testRef;
extern	uint32_t						devices[];
extern	tim 								timStack[];

#endif

