
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
					if(f->io && f->io->get) {
						c=f->io->get(f->io->rx);
					} 
					return c;
				}
				return f_getc((FIL *)f);
}
//__________________________________________________________________________________
int 		fputc(int c, FILE *f) {
				if(f==stdout) {
					if(f->io && f->io->put) {
						while(f->io->put(f->io->tx,c) == EOF) {
							_wait(2);
						}
					}
					return c;
				}
				return f_putc(c,(FIL *)f);
}
//__________________________________________________________________________________
_io			*_ITM;
volatile int32_t  ITM_RxBuffer=ITM_RXBUFFER_EMPTY; 
//__________________________________________________________________________________
void		*console(void *);
void		*itm(void *v) {
//__________________________________________________________________________________
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
				_ITM=_io_init(1024,1024);
				_proc_add(console,&_ITM,"console",0);
				_proc_add(itm,&_ITM,"itm",2);
				return _ITM;
}
