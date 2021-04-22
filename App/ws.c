#include	"ws.h"
#include	"console.h"
#include	"ssd1306.h"
#define		SW_version			100

__ALIGN_BEGIN 
uint32_t	wsdma[(__NWS+8+8)*24]; 
__ALIGN_END;


_ws ws[] =
{
	{{0,255,100},		0,0,{0}},
	{{60,255,100},	0,0,{0}},
	{{120,255,100},	0,0,{0}},
	{{180,255,100},	0,0,{0}},
	{{240,255,100},	0,0,{0}},
	{{300,255,100},	0,0,{0}},
	{{0,0,5},				0,0,{0}}
};

uint32_t	DecodeTab[4096];
uint32_t	pTimeout,crcTimeout,pCount[__COLS][__NWS];
uint32_t	tref[__NWS/6],chLev;
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void			Decode(int sect,payload *p) {
					if(crcTimeout && crcTimeout > HAL_GetTick())
						return;
					crcTimeout=0;
					
					if(p) {
						if(!pTimeout)
							pTimeout=HAL_GetTick()+20;
						if(!tref[sect])
							tref[sect]=p->pulse.tref;
							
						for(int i=0; i<__NWS/4/2; ++i)
							if(p->pulse.sect[i].count && p->pulse.sect[i].ch >= chLev) {
								ws[p->pulse.sect[i].ch].bit12 |= 1<<(sect*__NWS/4/2+i);
								chLev=p->pulse.sect[i].ch;
							}
						
					}	else {
						crcTimeout=HAL_GetTick()+1000;
						wsStream(sect*__NWS/4,0,__NWS/4-1);
						for(int i=0; i<__NWS/4; ++i)
							ws[0].timeout[sect*__NWS/4+i]=HAL_GetTick()+1000;
					}
}
/*******************************************************************************
* Function Name	: 
* Description		:
* Output				:
* Return				:
*******************************************************************************/
void			wsStream(int32_t npos, int32_t colour, int32_t n) {
RGB				rgb={0,0,0};
//uint32_t	*p = &wsdma[(npos+8+4)*24];
uint32_t	*p = &wsdma[(npos+4)*24];
					if(colour >= 0)
						HSV2RGB(ws[colour].colour, &rgb);
// krog					
					for(int i=0; i<n; ++i) {
						for(int k=0; k<8; ++k)
							(rgb.g & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
						for(int k=0; k<8; ++k)
							(rgb.r & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
						for(int k=0; k<8; ++k)
							(rgb.b & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
					}
// indikatorji			
//					if(n==1) {
//						int i=colour;
//						if(i<0)
//							i=-i-1;
//						p = &wsdma[(8+i-2)*24];
//						for(int k=0; k<8; ++k)
//							(rgb.g & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
//						for(int k=0; k<8; ++k)
//							(rgb.r & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
//						for(int k=0; k<8; ++k)
//							(rgb.b & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
//					}
					
					
					HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_4,wsdma,sizeof(wsdma)/sizeof(uint32_t));
					_DEBUG(DBG_LED,"\r%3d: * %02X %02X %02X %02X %02X %2d\r\n",HAL_GetTick() % 1000, npos, rgb.r,rgb.g,rgb.b,n,colour);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void			wsProcInit(void) {
					uint32_t *p = &wsdma[4*24];
		
					for(int i=0; i<(__NWS+8)*24; ++i)
						*p++=__TL;
					HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_4,wsdma,sizeof(wsdma)/sizeof(uint32_t));
					for (int i = 1; i < 4095; ++i) {
						int j = i,n = 0;
						while (j % 2)
							j = ror(12, j, 1);
						do {
							int k = 1;
							if (j % (1 << k)) {
								while (j % (1 << k) != j % (1 << (k + 1)))
									++k;
								DecodeTab[i] |= (1 << k) / 2;
							}
							DecodeTab[i] = ror(24, DecodeTab[i], 2 * k);
							j = ror(12, j, k);
							n += k;
						} while (n < 12);
						while (j != i) {
							DecodeTab[i] = ror(24, DecodeTab[i], 2);
							j = ror(12, j, 1);
						}
					}
					_proc_add(wsProc,NULL,"wsProc",20);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void			*wsProc(void *p) {
// procesiranje po timeoutu	
// iskanje prvega, cas in slot v t oz. s
					if(pTimeout && HAL_GetTick() > pTimeout) {
uint32_t		t=0;
						for(int i=0; i<__NWS/6; ++i) 
							if(tref[i]) {
								if(t && tref[i] > t)
									continue;
								t=tref[i];
							}
// briši bit12 za kvadrant, ki ni blizu prvemu
						for(int i=0; i<__NWS/6; ++i) {
							if(tref[i] && tref[i] > t+2) {
								for(int j=0; j<__COLS; ++j)
									ws[j].bit12 &= ~(7 << (i*__NWS/4/2));
							}
						}
// briši timeout, slot in referenco
						pTimeout=0;
						for(int i=0; i<__NWS/6; ++i)
							tref[i]=0;
// dekodiranje bit12>>bit24						
						for(int i=0; i<__COLS; ++i) {
							for(int j=0; j < __NWS; ++j)
								if(DecodeTab[ws[i].bit12] & (1 << j)) {
									ws[i].timeout[j]=HAL_GetTick()+500;
									ssdPrint(i,j);
								}
							ws[i].bit24 |= DecodeTab[ws[i].bit12];
							ws[i].bit12=0;
						}
					}
// proženje aktivnih v bit24 na 20ms				
					for(int i=0,j; i < __NWS; ++i) {
						for(j=0; j < __COLS; ++j) {
							if((ws[j].bit24 & (1 << i)) && pCount[j][i] < 5) {
								if(!pCount[j][i]++) {
									wsStream(i,j,1);
								}
								break;
							}
						}
						if(j==__COLS)
							for(j=0; j < __COLS; ++j)
								pCount[j][i]=0;
					}
// brisanje aktivnih po timeoutu	
					for(int i=0; i<__COLS; ++i) 
						for(int j=0; j<__NWS; ++j)
							if(ws[i].timeout[j] && HAL_GetTick() > ws[i].timeout[j]) {
								wsStream(j,-i-1,1);
								ws[i].timeout[j]=0;
								ws[i].bit24 &= ~(1<<j);
								pCount[i][j]=0;
							}
							
					for(int i=0; i<__COLS; ++i)
						if(ws[i].bit24)
							return wsProc;
					chLev=0;

					return wsProc;
}
/*******************************************************************************
 * Function RGB2HSV
 * Description: Converts an RGB color value into its equivalen in the HSV color space.
 * Copyright 2010 by George Ruinelli
 * The code I used as a source is from http://www.cs.rit.edu/~ncs/color/t_convert.html
 * Parameters:
 *   1. struct with RGB color (source)
 *   2. pointer to struct HSV color (target)
 * Notes:
 *   - r, g, b values are from 0..255
 *   - h = [0,360], s = [0,255], v = [0,255]
 *   - NB: if s == 0, then h = 0 (undefined)
 ******************************************************************************/
void	RGB2HSV(RGB RGB, HSV *HSV){
 unsigned char min, max, delta;
 
 if(RGB.r<RGB.g)min=RGB.r; else min=RGB.g;
 if(RGB.b<min)min=RGB.b;
 
 if(RGB.r>RGB.g)max=RGB.r; else max=RGB.g;
 if(RGB.b>max)max=RGB.b;
 
 HSV->v = max;                			// v, 0..255
 
 delta = max - min;									// 0..255, < v
 
 if( max != 0 )
 HSV->s = (int)(delta)*255 / max;		// s, 0..255
 else {
 // r = g = b = 0										// s = 0, v is undefined
 HSV->s = 0;
 HSV->h = 0;
 return;
 }
 
 if( RGB.r == max )
 HSV->h = (RGB.g - RGB.b)*60/delta;					// between yellow & magenta
 else if( RGB.g == max )
 HSV->h = 120 + (RGB.b - RGB.r)*60/delta;		// between cyan & yellow
 else
 HSV->h = 240 + (RGB.r - RGB.g)*60/delta;		// between magenta & cyan
 
// if(HSV->h < 0 )
// HSV->h += 360;
}
/*******************************************************************************
 * Function HSV2RGB
 * Description: Converts an HSV color value into its equivalen in the RGB color space.
 * Copyright 2010 by George Ruinelli
 * The code I used as a source is from http://www.cs.rit.edu/~ncs/color/t_convert.html
 * Parameters:
 *   1. struct with HSV color (source)
 *   2. pointer to struct RGB color (target)
 * Notes:
 *   - r, g, b values are from 0..255
 *   - h = [0,360], s = [0,255], v = [0,255]
 *   - NB: if s == 0, then h = 0 (undefined)
 ******************************************************************************/
void	HSV2RGB(HSV HSV, RGB *RGB){
 int i;
 float f, p, q, t, h, s, v;
 
 h=(float)HSV.h;
 s=(float)HSV.s;
 v=(float)HSV.v;
 
 s /=255;
 
 if( s == 0 ) {					// achromatic (grey)
 RGB->r = RGB->g = RGB->b = v;
 return;
 }
 
 h /= 60;								// sector 0 to 5
 i = floor( h );
 f = h - i;            // factorial part of h
 p = (unsigned char)(v * ( 1 - s ));
 q = (unsigned char)(v * ( 1 - s * f ));
 t = (unsigned char)(v * ( 1 - s * ( 1 - f ) ));
 
 switch( i ) {
 case 0:
 RGB->r = v;
 RGB->g = t;
 RGB->b = p;
 break;
 case 1:
 RGB->r = q;
 RGB->g = v;
 RGB->b = p;
 break;
 case 2:
 RGB->r = p;
 RGB->g = v;
 RGB->b = t;
 break;
 case 3:
 RGB->r = p;
 RGB->g = q;
 RGB->b = v;
 break;
 case 4:
 RGB->r = t;
 RGB->g = p;
 RGB->b = v;
 break;
 default:        // case 5:
 RGB->r = v;
 RGB->g = p;
 RGB->b = q;
 break;
 }
}
/*******************************************************************************
* Function Name	: 
* Description		:
* Output				:
* Return				:
*******************************************************************************/
void	ssdPrint(uint32_t col, uint32_t pos) {
	char c[8];
	sprintf(c,"%3d",((pos*360)/24+15)%360);
	SSD1306_GotoXY (0,0);
	SSD1306_Puts (c, &Font_32x64, SSD1306_COLOR_WHITE);
	SSD1306_GotoXY (105,0);
	SSD1306_Puts ("O", &Font_11x18, SSD1306_COLOR_WHITE);
	SSD1306_GotoXY (0,45);
	if(col==0)
		SSD1306_Puts ("LR      ", &Font_11x18, SSD1306_COLOR_WHITE);
	else if(col==1)
		SSD1306_Puts ("   LI   ", &Font_11x18, SSD1306_COLOR_WHITE);
	else if(col==2)
		SSD1306_Puts ("      BR", &Font_11x18, SSD1306_COLOR_WHITE);
//		else
//			SSD1306_Puts (" ---    ", &Font_11x18, SSD1306_COLOR_WHITE);
	SSD1306_UpdateScreen();

}
