#include	"ws.h"
#include	"can.h"


__ALIGN_BEGIN 
uint32_t	_leds[(__LEDS+8+8)*24]; 
__ALIGN_END;


struct led leds[] =
{
	{{0,255,100},		0,0,{0}},
	{{60,255,100},	0,0,{0}},
	{{120,255,100},	0,0,{0}},
	{{180,255,100},	0,0,{0}},
	{{240,255,100},	0,0,{0}},
	{{300,255,100},	0,0,{0}}
};

uint32_t	DecodeTab[4096];
uint32_t	pTimeout,pCount[__COLS][__LEDS];
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void			Decode(int sect,uint8_t *p) {
					if(p) {
						if(!pTimeout)
							pTimeout=HAL_GetTick()+20;
						
						for(int i=0; i<__LEDS/8; ++i)
							for(int j=0; j<__COLS; ++j)
								if(p[i] & (1<<j)) {
									leds[j].bit12 |= 1 << (sect*__LEDS/8+i);
								}
					}	else {
						ledStream(sect*__LEDS/4,0,__LEDS/4-1);
						for(int i=0; i<__LEDS/4; ++i)
							leds[0].timeout[sect*__LEDS/4+i]=HAL_GetTick()+1000;
					}
}
/*******************************************************************************
* Function Name	: 
* Description		:
* Output				:
* Return				:
*******************************************************************************/
void			ledStream(int32_t sect, int32_t colour, int32_t n) {
RGB				rgb={0,0,0};
uint32_t	*p = &_leds[(sect+8+4)*24];
					if(colour >= 0)
						HSV2RGB(leds[colour].colour, &rgb);
					
					for(int i=0; i<n; ++i) {
						for(int k=0; k<8; ++k)
							(rgb.g & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
						for(int k=0; k<8; ++k)
							(rgb.r & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
						for(int k=0; k<8; ++k)
							(rgb.b & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
					}
							
					if(n==1) {
						int i=colour;
						if(i<0)
							i=-i-1;
						p = &_leds[(8+i)*24];
						for(int k=0; k<8; ++k)
							(rgb.g & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
						for(int k=0; k<8; ++k)
							(rgb.r & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
						for(int k=0; k<8; ++k)
							(rgb.b & (0x80>>k)) ? (*p++=__TH)	: (*p++=__TL);
					}
					
					
					HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_4,_leds,sizeof(_leds)/sizeof(uint32_t));
					if(debug & (1<<DBG_LED)) {
						_io	*io=_stdio(_DBG);
						_print("\r%3d: * %02X %02X %02X %02X %02X %2d\r\n",HAL_GetTick() % 1000, sect, rgb.r,rgb.g,rgb.b,n,colour);
						_stdio(io);
					}
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void			ledProcInit(void) {
					uint32_t *p = &_leds[4*24];
		
					for(int i=0; i<(__LEDS+8)*24; ++i)
						*p++=__TL;
					HAL_TIM_PWM_Start_DMA(&htim2,TIM_CHANNEL_4,_leds,sizeof(_leds)/sizeof(uint32_t));
	
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
					_proc_add(ledProc,NULL,"ledProc",20);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void			*ledProc(void *p) {
	
					if(pTimeout && HAL_GetTick() > pTimeout) {
						pTimeout=0;
						for(int i=0; i<__COLS; ++i) {
							leds[i].bit24 |= DecodeTab[leds[i].bit12];
							for(int j=0; j < __LEDS; ++j)
								if(leds[i].timeout[j] == 0 && leds[i].bit24 & (1 << j))
									leds[i].timeout[j]=HAL_GetTick()+2000;
							leds[i].bit12=0;
						}
					}
						
					for(int i=0; i < __LEDS; ++i) {
						int j;
						for(j=0; j < __COLS; ++j) {
							if((leds[j].bit24 & (1 << i)) && pCount[j][i] < 5) {
								if(!pCount[j][i]++)
									ledStream(i,j,1);
								break;
							}
						}
						if(j==__COLS)
							for(j=0; j < __COLS; ++j)
								pCount[j][i]=0;
					}
	

					for(int i=0; i<__COLS; ++i) 
						for(int j=0; j<__LEDS; ++j)
							if(leds[i].timeout[j] && HAL_GetTick() > leds[i].timeout[j]) {
								ledStream(j,-i-1,1);
								leds[i].timeout[j]=0;
								leds[i].bit24 &= ~(1<<j);
								pCount[i][j]=0;
							}
					return ledProc;
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
 
 if(HSV->h < 0 )
 HSV->h += 360;
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


