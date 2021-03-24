#include	"stm32f4xx_hal.h"
#include <stdlib.h>
#include <stdio.h>

#define		N 300
static float	linreg(float, float);

int32_t 	count,offset;
struct {
	int32_t	x,y;
} p[N];

float		sumX=0, sumX2=0, sumY=0, sumXY=0, a, b;
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
uint16_t	eval(uint16_t y) {
	if(count) {
		if(y + offset - a - count*b > 32000)
			offset -= 0x10000;
		if(y + offset - a - count*b < -32000)
			offset += 0x10000;
	}
	return (int32_t)linreg(count,y+offset) % 0x10000;
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
		b = (j*sumXY - sumX*sumY) / (j*sumX2 - sumX*sumX);
	else 
		b=0;
	a = (sumY - b*sumX) / j;
	return a+b*x;
}
void shlin() {
	printf("n=%d,a=%f,b=%f,off=%d\r\n",count,a,b,offset);
}
