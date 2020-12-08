#ifndef		LEDS_H
#define		LEDS_H
#include	"stm32f4xx_hal.h"
#include	"main.h"

typedef struct  {
	uint32_t	t[4];
	GPIO_TypeDef *port;
	uint16_t	pin[4];
} leds;


#define _RED(a)			Leds.t[0]=HAL_GetTick()+a
#define _GREEN(a)		Leds.t[1]=HAL_GetTick()+a
#define _BLUE(a)		Leds.t[2]=HAL_GetTick()+a
#define _YELLOW(a)	Leds.t[3]=HAL_GetTick()+a

		
extern leds	Leds;
void	*ledProc(void *);

#endif
