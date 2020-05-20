#include 		"fatfs.h"
#include 		"can.h"
#include 		"console.h"
#include 		"proc.h"
//______________________________________________________________________________________
//______________________________________________________________________________________
//______________________________________________________________________________________
//______________________________________________________________________________________
//______________________________________________________________________________________
//______________________________________________________________________________________
/*******************************************************************************
* Function Name  : AckWait
* Description    : Stop the main CAN RX process and wait for the remote iap ack 
* Input          : CAN TX msg, waiting time (ms) 
* Return         : IAP message or EOF (not successful...)
*******************************************************************************/
int					AckWait(int t) {
						int to;
						if(t == 0)
							return EOF;
						to = HAL_GetTick() + t;																// nastavi cakalni nterval
						for(ackCount=0; HAL_GetTick() < to; _proc_loop()) {
							Watchdog();
							if(ackCount == 3)
								return EOF;
						}
						return 0;
}
/*******************************************************************************
* Function Name : str2hex
* Description   : pretvorba stringa v hex stevilo
* Input         : **p, pointer na string array (char *), n stevilo znakov
* Output        : char * se poveca za stevilo znakov
* Return        : hex integer
*******************************************************************************/
int					str2hex(char **p,int n) {
char				q[16];
						strncpy(q,*p,n)[n]='\0';
						*p += n;
						return(strtoul(q,NULL,16));
						}
/*******************************************************************************
* Function Name  : HexChecksumError
* Description    : preverja konsistentnost vrstice iz hex fila
* Input          : pointer na string 
* Return         : 0 ce je OK
*******************************************************************************/
int					HexChecksumError(char *p) {
int	 				err,n=str2hex(&p,2);
						for(err=n;n-->-5;err+=str2hex(&p,2));
						return(err & 0xff);
}
/*******************************************************************************
* Function Name  : CanHexProg request, server
* Description    : dekodira in razbije vrstice hex fila na 	pakete 8 bytov in jih
*								 : po�lje na CAN bootloader
* Input          : pointer na string, zaporedne vrstice hex fila, <cr> <lf> ali <null> niso nujni
* Output         : 
* Return         : 0 ce je checksum error sicer eof(-1). bootloader asinhrono odgovarja z ACK message
*				 				 : za vsakih 8 bytov !!!
*******************************************************************************/
int					CanHexProg(char *p) {
static int	ExtAddr=0;
int	 				n,a;
payload 		pyld;

						if(HexChecksumError(++p))
							return 0;
						n=str2hex(&p,2);
						a=(ExtAddr<<16)+str2hex(&p,4);
						switch(str2hex(&p,2)) {
							case 00:
								if(a<_FLASH_TOP)
									return 0;
								Send(_ID_IAP_ADDRESS,(payload *)&a,sizeof(int));
								for(int i=0; n--;) {
									pyld.byte[i++]=str2hex(&p,2);
									if(i==8 || !n) {	
										Send(_ID_IAP_DWORD,&pyld,i);
										i=0;
										if(!AckWait(200))
											return 0;
									}	
								}
								break;
							case 01:
								break;
							case 02:
								break;
							case 04:
							case 05:
								ExtAddr=str2hex(&p,4);
								break;
						}
						return(EOF);
}
/*******************************************************************************
* Function Name  : CanHexProg request, server
* Description    : dekodira in razbije vrstice hex fila na 	pakete 8 bytov in jih
*								 : po�lje na CAN bootloader
* Input          : pointer na string, zaporedne vrstice hex fila, <cr> <lf> ali <null> niso nujni
* Output         : 
* Return         : 0 ce je checksum error sicer eof(-1). bootloader asinhrono odgovarja z ACK message
*				 				 : za vsakih 8 bytov !!!
*******************************************************************************/
void				iapRemote(char * filename) {
						uint32_t n,k;																		// misc
																														// count lines >>>> n
						for(k=n=_FLASH_TOP; k<FATFS_ADDRESS; k+=sizeof(uint32_t)) {
							if(*(int32_t *)k != _FLASH_BLANK)
								n=k;
							Watchdog();
						}
												
						Send(_ID_IAP_GO,NULL,0);												// send iap req. as from sys
						if(!AckWait(3000)) {														// wait for ping
							Send(_ID_IAP_PING,NULL,0);										// send ping
							if(!AckWait(100))															// wait for response '-'
								return;
						}
						_print("\r\niap ping received...");
						_print("\r\nerasing");													// erase 5 pages (att. CubeMX ima drugacne 
																														//oznake za sektorje !!!
						for(k=FLASH_SECTOR_1<<3; k<FLASH_SECTOR_6<<3; k+=FLASH_SECTOR_1<<3) {
							Send(_ID_IAP_ERASE,(payload *)&k,sizeof(int));
							if(!AckWait(3000))														// send erase page, wait for ack
								return;
							_print(".");
						}
						
						_print("\r\nprogramming");
						k=_FLASH_TOP;
						Send(_ID_IAP_ADDRESS,(payload *)&k,sizeof(uint32_t));
						while(k <= n) {
							Send(_ID_IAP_DWORD,(payload *)k,sizeof(payload));
							if(!AckWait(200))
								return;

							if((k-_FLASH_TOP) % (8*((n-_FLASH_TOP)/8/20)) == 0)
								_print(".%3d%c%c%c%c%c",(100*(k-_FLASH_TOP))/(n-_FLASH_TOP),'%','\x8','\x8','\x8','\x8');
							
							k+=sizeof(payload);
						}

						Send(_ID_IAP_SIGN,NULL,0);											// send sign command
						if(!AckWait(300))																// wait for ack...
							return;
						_print("\r\nsign ...");
						Send(_ID_IAP_GO,NULL,0);												// send <go> command
						AckWait(0);
						_print("and RUN :)\r\n");
						return;
}
/*******************************************************************************
* Function Name  : CanHexProg request, server
* Description    : dekodira in razbije vrstice hex fila na 	pakete 8 bytov in jih
*								 : po�lje na CAN bootloader
* Input          : pointer na string, zaporedne vrstice hex fila, <cr> <lf> ali <null> niso nujni
* Output         : 
* Return         : 0 ce je checksum error sicer eof(-1). bootloader asinhrono odgovarja z ACK message
*				 				 : za vsakih 8 bytov !!!
*******************************************************************************/
int32_t			USBH_Iap(void) {	
FATFS				fs0,fs1;
DIR					dir;
FIL					f0,f1;
BYTE 				buffer[128];   																										//	file copy buffer */
FRESULT 		fr;          																											//	FatFs function common result code	*/
UINT 				br, bw;         																									//	File read/write count */
FILINFO			fno;
	
						if(f_mount(&fs0,"1:",1) ||
							f_mount(&fs1,"0:",1) ||
							f_chdrive("1:") || 
							f_chdir("/sync") ||
							f_findfirst(&dir,&fno,"/sync","*")) 
								return 0;
						Watchdog_init(4000);
						do {
							_print("\r\n%-16s",fno.fname);
							if (fno.fattrib & AM_DIR)	{
								_print("%-8s","/");
								continue;
							}
							_print("%-8d",(int)fno.fsize);
							if(f_chdrive("1:") || f_open(&f0,fno.fname,FA_OPEN_EXISTING | FA_READ))
								continue;
							if(f_chdrive("0:") || f_open(&f1,fno.fname,FA_CREATE_ALWAYS | FA_WRITE))
								continue;
							for (;;) {
								if((HAL_GetTick() / 100) % 2)
									_YELLOW(1000);
								else
									_YELLOW(0);
								fr = f_read(&f0, buffer, sizeof buffer, &br);	
								if (fr || br == 0) break; 
								fr = f_write(&f1, buffer, br, &bw);
								if (fr || bw < br) break;
								Watchdog();
							}
							f_close(&f0);
							f_close(&f1);
						}  while(f_findnext(&dir,&fno) == FR_OK && *fno.fname);
						Watchdog_init(400);
						return(EOF);
}
