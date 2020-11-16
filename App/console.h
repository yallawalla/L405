#ifndef		_CONSOLE_H
#define		_CONSOLE_H
#include	"stm32f4xx_hal.h"
#include	"ff.h"

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

extern		DAC_HandleTypeDef hdac;
extern		IWDG_HandleTypeDef hiwdg;
extern		TIM_HandleTypeDef htim7;
void			printVersion(void);
int32_t		USBH_Iap(void);
FRESULT		ff_format(char *);
FRESULT		LoadSettings(void),
					SaveSettings(void);
FRESULT		DecodeCom(char *);
int				ff_pack(int);
void			Parse(int);
void			*console(void *);
void 			JumpToBootloader(void);










#endif


