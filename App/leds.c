#include "leds.h"

#ifdef	__DISCO__
		#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
		#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
		led Leds = {{0,0,0,0},GPIOD,{GPIO_PIN_14,GPIO_PIN_12,GPIO_PIN_15,GPIO_PIN_13}};
#else 
	#ifdef	__NUCLEO__
		#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
		#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
		led Leds = {{0,0,0,0},GPIOB,{GPIO_PIN_14,GPIO_PIN_0,GPIO_PIN_7,GPIO_PIN_7}};
	#else
		#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
		#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
		leds Leds = {{0,0,0,0},LED_R_GPIO_Port,{LED_R_Pin,LED_G_Pin,LED_R_Pin,LED_G_Pin}};
	#endif
#endif
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	*ledProc(void *v) {
	for(int i=0;i<4;++i) {
		if(!Leds.t[i])
			continue;
		if(HAL_GetTick() > Leds.t[i]) {
			ledOff(Leds.port, Leds.pin[i]);
			Leds.t[i]=0;
		}
		else
			ledOn(Leds.port, Leds.pin[i]);
	}
	return ledProc;
}

