
/*----------------------------------------------------------------------------
 * Name:    Retarget.c
 * Purpose: 'Retarget' layer for target-dependent low level functions
 * Note(s):
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2012 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/
#include "io.h"
#include "proc.h"
#include "ff.h"
//_________________________________________________________________________________
FILE 		__stdout;
FILE 		__stdin;
FILE 		__stderr;
/*-----------------------------------------------------------------------*/
/* Get a character from the file                                          */
/* add. to FatFs																												 */
/*-----------------------------------------------------------------------*/
int			f_getc (FIL* fil) {						/* Pointer to the file object */
				UINT br;
				BYTE s[4];
				f_read(fil, s, 1, &br);				/* Write the char to the file */
				return (br == 1) ? s[0] : EOF;/* Return the result */
}
//__________________________________________________________________________________
int 		fgetc(FILE *f) {
int			c=EOF;
				if(f==stdin) {
					if(*stdin->io && (*stdin->io)->get) {
						c=(*stdin->io)->get((*stdin->io)->rx);
					} 
					return c;
				}
				return f_getc((FIL *)f);
}
//__________________________________________________________________________________
int 		fputc(int c, FILE *f) {
				if(f==stdout) {
					if((*stdout->io) && (*stdout->io)->put) {
						while((*stdout->io)->put((*stdout->io)->tx,c) == EOF) {
							_wait(2);
						}
					}
					return c;
				}
				return f_putc(c,(FIL *)f);
}
//__________________________________________________________________________________
void		*console(void *);
//__________________________________________________________________________________
_io			*_ITM;
volatile int32_t  ITM_RxBuffer=ITM_RXBUFFER_EMPTY; 
//__________________________________________________________________________________
void		*itm(void *v) {
int			i=0;
_io 		*io=*(_io **)v;
				if(_buffer_pull(io->tx,&i,1))
					ITM_SendChar(i);
				if(ITM_CheckChar()) {
					i=ITM_ReceiveChar();
					_buffer_push(io->rx,&i,1);
				}
				return v;
}
//______________________________________________________________________________________
_io			*InitITM(void) {
				_ITM=_io_init(128,128);
				_proc_add(console,&_ITM,"ITM console",0);
				_proc_add(itm,&_ITM,"itm",2);
				return _ITM;
}


//__________________________________________________________________________________
_io			*_VCP;
__weak	void VCP_Transmit(uint8_t *pbuf, uint16_t len) {}
__weak	bool VCP_Ready=false;
//__________________________________________________________________________________
void		*vcp(void *v) {
_io 		*io=*(_io **)v;
uint8_t pbuf[16];
				if(VCP_Ready && _buffer_count(io->tx))
					VCP_Transmit(pbuf,_buffer_pull(io->tx,pbuf,16));
				return v;
}
//______________________________________________________________________________________
void		VCP_Init() {
				_VCP=_io_init(128,128);
				_proc_add(vcp,&_VCP,"vcp",0);
}
//______________________________________________________________________________________
void		VCP_DeInit(void *p) {
				_VCP=_io_close(_VCP);
				_proc_find(console,&_VCP)->f=NULL;
				_proc_find(vcp,&_VCP)->f=NULL;
}
//______________________________________________________________________________________
void		VCP_Receive(uint8_t *pbuf, uint16_t len) {
				_buffer_push(_VCP->rx,pbuf,len);
}




