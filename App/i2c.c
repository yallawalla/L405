#include "stm32f4xx_hal.h"
#include <math.h>
#include "proc.h"
#include "console.h"
#include "ssd1306.h"
#include "i2c.h"
#include "bitmap.h"

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;

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

static uint32_t	timeout;
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if(timeout && HAL_GetTick() > timeout) {
		memset(SSD1306.Buffer, 0, sizeof(SSD1306.Buffer));
		timeout=0;
	}
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
	
	SSD1306.Inverted=false;
	SSD1306.color=SSD1306_COLOR_WHITE;
	
	ssdHome();
	ssdClear();
	ssdFont(&Font_7x10);
	
	SSD1306.Initialized=true;
}

//______________________________________________________________________________
void	ssdClear() {
	SSD1306_Fill(SSD1306_COLOR_BLACK);
}
//______________________________________________________________________________
void	ssdXY(uint8_t x, uint8_t y) {
	SSD1306_GotoXY(x,y);
}
//______________________________________________________________________________
void	ssdHome() {
	SSD1306_GotoXY(0,0);
}
//______________________________________________________________________________
void	ssdFont(FontDef_t *f) {
	SSD1306.Font=f;
}
//______________________________________________________________________________
void	ssdColor(SSD1306_COLOR_t c) {
	SSD1306.color=c;
}
//______________________________________________________________________________
void	ssdPrint(const char *format, ...) {
	char buf[128];	
	va_list	aptr;
	va_start(aptr, format);
	vsnprintf(buf, 128, format, aptr);
	va_end(aptr);
	SSD1306_Puts (buf, SSD1306.Font, SSD1306.color);
	timeout = HAL_GetTick()+5000;
}
//______________________________________________________________________________
_BAR *barInit(uint8_t l, uint8_t t, uint8_t w, uint8_t h, btype type) {
	_BAR *b=malloc(sizeof(_BAR));
	b->left=l;
	b->top=t;
	b->width=w;
	b->height=h;
	b->type=type;
						
	SSD1306_DrawRectangle(l,t,w,h,SSD1306_COLOR_WHITE);
	SSD1306_DrawFilledRectangle(l+1,t+1,w-2,h-2,SSD1306_COLOR_BLACK);
	return b;
}
//______________________________________________________________________________
void	barDraw(_BAR *b, uint8_t v) {
	if(b->type==_BAR_VERTICAL) {
		if(v > b->val)
			SSD1306_DrawFilledRectangle(b->left+2,b->top+2+(b->height-4)*(100-v)/100,b->width-4,(b->height-4)*v/100,SSD1306_COLOR_WHITE);
		if(b->val > v)
			SSD1306_DrawFilledRectangle(b->left+1,b->top+1,b->width-2,(b->height-2)*(100-v)/100,SSD1306_COLOR_BLACK);
		b->val=v;
	}
	if(b->type==_BAR_HORIZONTAL) {
		if(v > b->val)
			SSD1306_DrawFilledRectangle(b->left+2,b->top+2,(b->width-4)*v/100,b->height-4,SSD1306_COLOR_WHITE);
		if(b->val > v)
			SSD1306_DrawFilledRectangle(b->left+1+(b->width-2)*v/100,b->top+1,(b->width-2)*(100-v)/100,b->height-2,SSD1306_COLOR_BLACK);
		b->val=v;
	}
	timeout = HAL_GetTick()+5000;
}
//______________________________________________________________________________
float rx(float x,float y,float fi) {
	return x*cos(fi)+y*sin(fi)+0.5;
}
float ry(float x,float y,float fi) {
	return y*cos(fi)-x*sin(fi)+0.5;
}
//...
void	*watch(void *v) {
	#define THICK	3
	#include	<math.h>
	static uint32_t	offset=0;
	if(v) {
		if(!offset) {
			_proc_add(watch,NULL,"watch",1000);
			SSD1306_Fill(SSD1306_COLOR_BLACK);
		}
		offset=(uint32_t)v;
	}
	timeout = HAL_GetTick()+5000;
	ssdClear();
	ssdHome();
	ssdFont(&Font_7x10);
	ssdPrint("%2d:%02d:%02d",((HAL_GetTick()-offset)/1000/3600)%24,((HAL_GetTick()-offset)/1000/60)%60,((HAL_GetTick()-offset)/1000)%60);

	float pi=3.14159265359f;
	float fi=(HAL_GetTick()-offset)/1000.0f/30.0f*pi;
	float Cos=cos(fi),Sin=sin(fi);
	SSD1306_DrawRectangle(64,0,63,63,SSD1306_COLOR_WHITE);
	for(int8_t i=0; i<4; ++i)
		SSD1306_DrawCircle(rx(0,28,pi*i/2)+96,32-ry(0,28,pi*i/2),1,SSD1306_COLOR_WHITE);
	for(int8_t i=0; i<12; ++i)
		SSD1306_DrawPixel(rx(0,28,pi*i/6)+96,32-ry(0,28,pi*i/6),SSD1306_COLOR_WHITE);
	SSD1306_DrawLine(rx(0,-8,fi)+96,32-ry(0,-8,fi), rx(0,28,fi)+96, 32-ry(0,28,fi) ,SSD1306_COLOR_WHITE);
	fi /= 60.0f;	
	SSD1306_DrawFilledTriangle(rx(-THICK,-8,fi)+96,32-ry(-THICK,-8,fi), rx(THICK,-8,fi)+96,32-ry(THICK,-8,fi), rx(0,28,fi)+96, 32-ry(0,28,fi) ,SSD1306_COLOR_BLACK);
	SSD1306_DrawTriangle(rx(-THICK,-8,fi)+96,32-ry(-THICK,-8,fi), rx(THICK,-8,fi)+96,32-ry(THICK,-8,fi), rx(0,28,fi)+96, 32-ry(0,28,fi) ,SSD1306_COLOR_WHITE);
	fi /= 12.0f;
	SSD1306_DrawFilledTriangle(rx(-THICK,-8,fi)+96,32-ry(-THICK,-8,fi), rx(THICK,-8,fi)+96,32-ry(THICK,-8,fi), rx(0,15,fi)+96, 32-ry(0,15,fi) ,SSD1306_COLOR_BLACK);
	SSD1306_DrawTriangle(rx(-THICK,-8,fi)+96,32-ry(-THICK,-8,fi), rx(THICK,-8,fi)+96,32-ry(THICK,-8,fi), rx(0,15,fi)+96, 32-ry(0,15,fi) ,SSD1306_COLOR_WHITE);
	return watch;
}			
