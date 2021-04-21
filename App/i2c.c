#include "stm32f4xx_hal.h"
#include "fonts.h"
#include "ssd1306.h"
#include "proc.h"
#include "console.h"

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;
_buffer *i2cBuf;

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
void	i2cInit() {
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
****************************f***************************************************/

void	*i2cProc(void *v) {
	if(v) {
		if(hi2c1.State == HAL_I2C_STATE_READY) {
			static uint8_t buf[130];
			if(_buffer_pull(v,buf,1)) {
				switch(*buf) {
					case 0x00:
						while(!_buffer_pull(v,buf+1,1)) 
							_wait(1);
						HAL_I2C_Master_Transmit_DMA(&hi2c1, SSD1306_I2C_ADDR, buf, 2 );
						break;
					case 0x40:
						while(_buffer_count(v) < sizeof(void *)) 
							_wait(1);
						_buffer_pull(v,buf+1,sizeof(void *));
						memcpy(buf+1,*(const void **)&buf[1],128);
						HAL_I2C_Master_Transmit_DMA(&hi2c1, SSD1306_I2C_ADDR, buf, 129 );
						break;
				}
			}
		}		
	} else {
		i2cBuf=_buffer_init(1024);
		_proc_add(i2cProc,i2cBuf,"i2c",0);
	}
	return i2cProc;
}
