/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbh_platform.c

  * @brief          : This file implements the USB platform
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbh_platform.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/**
  * @brief  Drive VBUS.
  * @param  state : VBUS state
  *          This parameter can be one of the these values:
  *           - 0 : VBUS Active
  *           - 1 : VBUS Inactive
  */
void MX_DriverVbusFS(uint8_t state)
{ 
  uint8_t data = state; 
  /* USER CODE BEGIN PREPARE_GPIO_DATA_VBUS_FS */
  if(state != 0)
  {
    /* Drive high Charge pumhp */ 	     
    data = GPIO_PIN_RESET;
#ifdef	__NUCLEO__
		HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_SET);
#endif
#ifdef	__DISCO__
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
#endif
  }
  else
  {
    /* Drive low Charge pump */
    data = GPIO_PIN_SET;
#ifdef	__NUCLEO__
		HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_RESET);
#endif
#ifdef	__DISCO__
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
#endif
  }
  /* USER CODE END PREPARE_GPIO_DATA_VBUS_FS */
  HAL_GPIO_WritePin(GPIOC,GPIO_PIN_12,(GPIO_PinState)data);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
