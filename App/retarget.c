
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
					if(stdin->io && *stdin->io && (*stdin->io)->get) {
						c=(*stdin->io)->get((*stdin->io)->rx);
					} 
					return c;
				}
				return f_getc((FIL *)f);
}
//__________________________________________________________________________________
int 		fputc(int c, FILE *f) {
				if(f==stdout) {
					if(stdout->io && (*stdout->io) && (*stdout->io)->put) {
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
				_proc_add(itm,&_ITM,"itmtx",2);
				return _ITM;
}
//__________________________________________________________________________________
_io			*_VCP;
__weak	void cdcTransmit(uint8_t *pbuf, uint16_t len) {}
__weak	bool cdcReady=false;
//__________________________________________________________________________________
void		*vcp(void *v) {
_io 		*io=*(_io **)v;
uint8_t pbuf[64];
				if(cdcReady && _VCP && io->tx && _buffer_count(io->tx))
					cdcTransmit(pbuf,_buffer_pull(io->tx,pbuf,64));
				return v;
}
//______________________________________________________________________________________
_io			*InitVCP(void) {
				_proc_add(console,&_VCP,"VCP console",0);
				_proc_add(vcp,&_VCP,"vcptx",2);
				return _VCP;
}
//______________________________________________________________________________________
//	
// cdc callbacks from USBDevice stack, IRQ level !!!
//______________________________________________________________________________________
void		cdcInit() {
				if(!_VCP)
					_VCP=_io_init(1024,1024);
}
//______________________________________________________________________________________
void		cdcDeInit(void *p) {
				if(_VCP)
					_VCP=_io_close(_VCP);
}
//______________________________________________________________________________________
void		cdcReceive(uint8_t *pbuf, uint16_t len) {
				if(_VCP && _VCP->rx)
					_buffer_push(_VCP->rx,pbuf,len);
}
//__________________________________________________________________________________
// hard fault handler in C,
// with stack frame location as input parameter
// called from HardFault_Handler in file eMac.s
//__________________________________________________________________________________
void hard_fault_handler_c (unsigned int * hardfault_args)
{
  unsigned int stacked_r0;
  unsigned int stacked_r1;
  unsigned int stacked_r2;
  unsigned int stacked_r3;
  unsigned int stacked_r12;
  unsigned int stacked_lr;
  unsigned int stacked_pc;
  unsigned int stacked_psr;
	FIL f;

  stacked_r0 = ((unsigned long) hardfault_args[0]);
  stacked_r1 = ((unsigned long) hardfault_args[1]);
  stacked_r2 = ((unsigned long) hardfault_args[2]);
  stacked_r3 = ((unsigned long) hardfault_args[3]);
 
  stacked_r12 = ((unsigned long) hardfault_args[4]);
  stacked_lr = ((unsigned long) hardfault_args[5]);
  stacked_pc = ((unsigned long) hardfault_args[6]);
  stacked_psr = ((unsigned long) hardfault_args[7]);
 
	if(f_open(&f,"fault",FA_OPEN_APPEND | FA_WRITE)==FR_OK) {
		f_printf(&f,"\n\r\n\r[Hard fault handler - all numbers in hex]\n\r");
		f_printf(&f,"R0 = %x\n\r", stacked_r0);
		f_printf(&f,"R1 = %x\n\r", stacked_r1);
		f_printf(&f,"R2 = %x\n\r", stacked_r2);
		f_printf(&f,"R3 = %x\n\r", stacked_r3);
		f_printf(&f,"R12 = %x\n\r", stacked_r12);
		f_printf(&f,"LR [R14] = %x  subroutine call return address\n\r", stacked_lr);
		f_printf(&f,"PC [R15] = %x  program counter\n\r", stacked_pc);
		f_printf(&f,"PSR = %x\n\r", stacked_psr);
		f_printf(&f,"BFAR = %x\n\r", (*((volatile unsigned long *)(0xE000ED38))));
		f_printf(&f,"CFSR = %x\n\r", (*((volatile unsigned long *)(0xE000ED28))));
		f_printf(&f,"HFSR = %x\n\r", (*((volatile unsigned long *)(0xE000ED2C))));
		f_printf(&f,"DFSR = %x\n\r", (*((volatile unsigned long *)(0xE000ED30))));
		f_printf(&f,"AFSR = %x\n\r", (*((volatile unsigned long *)(0xE000ED3C))));
		f_printf(&f,"SCB_SHCSR = %x\n\r", SCB->SHCSR);
		f_close(&f);
	}
	NVIC_SystemReset();
}




