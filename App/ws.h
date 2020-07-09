#ifndef		WS2812_H
#define		WS2812_H

#include	"stm32f4xx_hal.h"
#include	<math.h>
#include	"proc.h"

#define		__TH		94
#define		__TL		36
#define 	__TP		130
#define 	__LEDS	24
#define 	__COLS	6

typedef		struct	{uint8_t r; uint8_t g; uint8_t b; }	RGB;
typedef		struct	{int16_t h; uint8_t s; uint8_t v; }	HSV;

struct led {
	HSV	colour;
	uint32_t	bit12;
	uint32_t	bit24;
	uint32_t	timeout[__LEDS];
	uint32_t	row[__LEDS];
};

extern		TIM_HandleTypeDef htim2;
extern		uint32_t	DecodeTab[];

void			wsProcInit(void),
					wsStream(int32_t, int32_t, int32_t),
					*wsProc(void *),
					Decode(int,uint8_t *),
					RGB2HSV(RGB, HSV *),
					HSV2RGB(HSV, RGB *);

static __inline	uint32_t	rot12(uint32_t i, uint32_t n) {
	return (((i << n) | (i >> (12 - n))) & 0xfff);
}

static __inline	uint32_t	rot32(uint32_t i, uint32_t n) {
	return ((i << n) | (i >> (32 - n)));
}

static __inline	uint32_t ror(uint32_t base, uint32_t i, uint32_t n) {
	while (n--)
		i = ((i % 2) << (base-1)) + i / 2;
	return i;
}
#endif
