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
int32_t		USBH_Iap(void);
void			printVersion(void);
void			ff_erase(void);
FRESULT		ff_format(char *);
FRESULT		LoadSettings(void),
					SaveSettings(void);
FRESULT		DecodeCom(char *);
FRESULT		iapRemote(void);

int				ff_pack(int);
void			Parse(int);
void			*console(void *);

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
extern uint32_t debug,error,errmask,pinV,ssd_timeout;
extern _io**		_DBG;
extern	bool		iapInproc;
enum dbg {    
	DBG_CAN_RX=0,
	DBG_CAN_TX,
	DBG_CRC,
	DBG_TIMING,
	DBG_LED,
	DBG_USEC,
	DBG_CONSOLE,
	DBG_STAT,
	DBG_VOLT,
	DBG_SYNC
};

enum err {    
	ERR_CAN_TX=0,
	ERR_V45,
	ERR_VM5,
	ERR_SYNC,
	ERR_PIN=8
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
	
#define _SETERR(n) 											\
	do {																	\
		error |= ((1<<(n)) & ~errmask);			\
	} while(0)

#define _CLRERR(n) 											\
	do {																	\
		error &= (~(1<<(n)));								\
	} while(0)
	

void	SectorQuery(void);
void	HAL_USBD_Setup(void);
void	UsbDevice_Init(void);
void	UsbDevice_DeInit(void);
bool	flashLock(bool);
void	*selftest(void);
	
#define _V45	((float)(pwr.V45*3.3/4095.0*(1.2+47)/1.2))
#define _VM5	((float)(3.3 - (4095-pwr.Vm5)*((1.2+6.8)/1.2*3.3/4095.0)))
#define _TEMP	((float)((pwr.T*3.3/4095.0 - 0.76)/2.5e-3+25.0))
	
extern uint32_t __Vectors[];
HAL_StatusTypeDef	FLASH_Program(uint32_t, uint32_t), FLASH_Erase(uint32_t, uint32_t);


