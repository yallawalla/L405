#include	"stm32f4xx_hal.h"
#include <stdlib.h>
#include <stdio.h>

#define		N 100
static	double		linreg(double, double);
static	uint16_t	kalman(uint16_t x);

int32_t 	count, offset, upperq;
;
struct {
	int32_t	x,y;
} p[N];

double		sumX=0, sumX2=0, sumY=0, sumXY=0, a, b;
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
uint16_t	evall(uint16_t y) {
	return kalman(y);
}


uint16_t	eval(uint16_t y) {
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
	
//	if(count==N)
//		offset=a - (int32_t)(a+b*count )% 0x10000;

//	return (int32_t)linreg(count & N, offset) % 0x10000;
	return (int32_t)linreg(count, y+offset) % 0x10000;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
double	linreg(double x, double y) {
	uint32_t	i = count++ % N;
	uint32_t	j = count;

	if (count > N) {
		j = N;
		sumX -= p[i].x;
		sumX2 -= p[i].x*p[i].x;
		sumY -= p[i].y;
		sumXY -= p[i].x*p[i].y;
	}

	p[i].x = x;
	p[i].y = y;

	sumX += x;
	sumX2 += x*x;
	sumY += y;
	sumXY += x*y;
// y=b*x + a
	if (j > 1) 
		b = (j*sumXY/sumX-sumY)/(j*sumX2/sumX-sumX);
	else 
		b=0;
	a = (sumY - b*sumX)/j;
	
	return a+b*x;
}
void shlin() {
	printf("n=%d,a=%f,b=%f,off=%d\r\n",count,a,b,offset);
}

uint16_t	kalman(uint16_t x)  {
	uint32_t ky = 100, kdy = 10000;
	static uint32_t y,dy;
	if (count > 0) {
		if (upperq > 0 && x < 0x4000)
			y -= 0x10000;
		else if (upperq == 0 && x > 0xc000)
			y += 0x10000; 
	}
	if (x > 0x8000)
		upperq = 1;
	else
		upperq = 0;

	switch (count) {
			case 0:
					y = x;
					break;
			case 1:
					dy = x - y;
					break;
			default:
					dy += (x - y);
					y += (x - y) / ky + dy / kdy;
					//     y = y % 0x10000;
					break;
	}
	++count;
	return y % 0x10000;
}
