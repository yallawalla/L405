#include "leds.h"

#ifdef	__DISCO__
		#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
		#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
#else
#ifdef	__NUCLEO__
	#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
	#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
#else
#ifdef	__IOC__
	#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
	#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
#else
	#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
	#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
#endif
#endif
#endif
led Leds = {{0,0},		LED_R_GPIO_Port,{LED_R_Pin,LED_G_Pin}};
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	*ledProc(void *v) {
	for(int i=0;i<2;++i) {
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

