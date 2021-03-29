#include "stm32f4xx_hal.h"
#include "console.h"
#include <stdlib.h>
#include <stdio.h>

#define		N 100
static	double		linreg(double, double);

int32_t 	count, offset, upperq;
uint16_t	syncval;
struct {
	double 	x,y;
} p[N];

double		sumX=0, sumX2=0, sumY=0, sumXY=0, a, b;
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	eval(uint16_t *n,uint16_t *y) {
	if(*y > syncval)
		++*n;
	*y = *y-syncval;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
uint16_t	sync(uint16_t y) {
	if (count > 3*N) {
			double aa=a+b*count, bb=b;
			sumX = sumX2 = sumY = sumXY = offset = upperq = count = 0;
			while(count < N) {
				int32_t l=aa + bb * (count-N);
				if(l<0)
					l &= 0xffff;
				l=sync(l & 0xffff);
			}
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
	syncval=(int32_t)linreg(count, y+offset) % 0x10000;
	return syncval;
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
