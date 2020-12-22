#ifndef		_CONSOLE_H
#define		_CONSOLE_H
#include	"stm32f4xx_hal.h"
#include	"ff.h"
#include	"io.h"

int				Escape(void);
enum			err_parse	{
					_PARSE_OK=0,
					_PARSE_ERR_SYNTAX,
					_PARSE_ERR_ILLEGAL,
					_PARSE_ERR_MISSING,
					_PARSE_ERR_NORESP,
					_PARSE_ERR_OPENFILE,
					_PARSE_ERR_MEM
};
extern		IWDG_HandleTypeDef hiwdg;
void			printVersion(void);
int32_t		USBH_Iap(void);
FRESULT		ff_format(char *);
FRESULT		LoadSettings(void),
					SaveSettings(void);
FRESULT		DecodeCom(char *);
FRESULT		iapRemote(void);

int				ff_pack(int);
void			Parse(int);
void			*console(void *);
void 			JumpToBootloader(void);

typedef 	struct {
	uint16_t	V45;
	uint16_t	Vm5;
	uint16_t	T;
} dma;
	
typedef		struct {
	dma				dma[128];	
	uint32_t	V45;
	uint32_t	Vm5;
	uint32_t	T;
}	adc;

extern adc			pwr;
extern uint32_t debug;
extern _io**		_DBG;

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

#define _DEBUG(n,f, ...) 				\
	do {													\
		if(debug & (1<<(n))) {			\
			_io **io=_stdio(_DBG);		\
			_print(f,__VA_ARGS__);		\
			_stdio(io);								\
		} 													\
	} while(0)	
#endif

void	SectorQuery(void);
void	HAL_USBD_Setup(void);
void	UsbDevice_Init(void);
void	UsbDevice_DeInit(void);
