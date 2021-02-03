#ifndef		LEDS_H
#define		LEDS_H
#include	"stm32f4xx_hal.h"
#include	"main.h"

typedef struct  {
	uint32_t	t[2];
	GPIO_TypeDef *port;
	uint16_t	pin[2];
} led;

#define _RED(a)			Leds.t[0]=HAL_GetTick()+a
#define _GREEN(a)		Leds.t[1]=HAL_GetTick()+a

extern led	Leds;
void	*ledProc(void *);

#endif
