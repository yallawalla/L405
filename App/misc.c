#include	"stm32f4xx_hal.h"
#include	"console.h"
#include	<stdlib.h>
#include	<stdio.h>

#define		N 100
static		float			linreg(float, float);

static		int32_t 	count, offset, upperq;
static		uint16_t	syncval;
static		float			sX=0, sX2=0, sY=0, sXY=0, a, b;
static		struct		{float 	x,y;} p[N];
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
int32_t	eval(int32_t t) {
	return t-syncval-t*b/(float)(65536.0*128.0);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
float	linreg(float x, float y) {
	uint32_t	i = count++ % N;
	uint32_t	j = count;

	if (count > N) {
		j = N;
		sX -= p[i].x;
		sX2 -= p[i].x*p[i].x;
		sY -= p[i].y;
		sXY -= p[i].x*p[i].y;
	}

	p[i].x = x;
	p[i].y = y;

	sX += x;
	sX2 += x*x;
	sY += y;
	sXY += x*y;
// y=b*x + a
	if (j > 1) 
		b = (j*sXY/sX-sY)/(j*sX2/sX-sX);
	else 
		b=0;
	a = (sY - b*sX)/j;
	
	return a+b*x;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
uint16_t	sync(uint16_t y) {
	if (count > 3*N) {
			float a0=a+b*count, b0=b;
			sX = sX2 = sY = sXY = offset = upperq = count = 0;
			while(count < N)
				sync((int32_t)(a0 + b0 * (count-N)) & 0xffff);
	}
	if(count) {
		if(upperq && y < 0x4000)
			offset += 0x10000;
		else if(!upperq && y > 0xc000)
			offset -= 0x10000;
	}
	if(y > 0x8000)
			upperq=1;
	else
			upperq=0;
//	syncval=(int32_t)linreg(count, y+offset) % 0x10000;
	syncval=(int32_t)linreg(count, y+offset);
	return syncval;
}