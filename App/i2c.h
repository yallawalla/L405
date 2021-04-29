#ifndef		_I2C_H
#define		_I2C_H

#include "fonts.h"

typedef	enum {_BAR_VERTICAL,_BAR_HORIZONTAL} btype;
typedef struct {
	uint8_t		left, top, height, width;
	uint16_t	val;
	btype			type;
} _BAR;

_BAR *barInit(uint8_t, uint8_t, uint8_t, uint8_t, btype);
void	barDraw(_BAR *, uint8_t);

void	Error_Handler(void);
void	ssdClear(void);
void	ssdXY(uint8_t, uint8_t);
void	ssdHome(void);
void	ssdFont(FontDef_t *);
void	ssdColor(SSD1306_COLOR_t);
void	ssdPrint(const char *format, ...);


#endif
