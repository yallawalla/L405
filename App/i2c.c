#include "stm32f4xx_hal.h"
#include "fonts.h"
#include "ssd1306.h"
#include "proc.h"
#include "console.h"
#include	"bitmap.h"

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;

void Error_Handler(void);
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 240;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
static void MX_GPIO_Init(void)
{
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
extern uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
uint8_t cmdbuf[]={0x00,0xAE,0x20,0x10,0xB0,0xC8,0x00,0x10,0x40,
									0x81,0xFF,0xA1,0xA6,0xA8,0x3F,0xA4,0xD3,
									0x00,0xD5,0xF0,0xD9,0x22,0xDA,0x12,0xDB,
									0x20,0x8D,0x14,0xAF,
									0xB0,0x00,0x10};					
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	SSD1306.prefix=0x40;
	HAL_I2C_Master_Transmit_DMA(&hi2c1,SSD1306_I2C_ADDR,&SSD1306.prefix,SSD1306_WIDTH * SSD1306_HEIGHT / 8 + 1);
}
void	i2cInit() {
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
	SSD1306_DrawBitmap(0,0,logo, 128, 64, SSD1306_COLOR_WHITE);
	
	HAL_I2C_Master_Transmit_DMA(&hi2c1,SSD1306_I2C_ADDR,cmdbuf,sizeof(cmdbuf));
	_wait(1000);
	SSD1306_Fill(SSD1306_COLOR_BLACK);
}
