#include 		"fatfs.h"
#include 		"can.h"
#include 		"console.h"
#include 		"proc.h"
#include		"leds.h"
//______________________________________________________________________________________
//______________________________________________________________________________________
//______________________________________________________________________________________
//______________________________________________________________________________________
//______________________________________________________________________________________
//______________________________________________________________________________________
  DRESULT USER_write (BYTE, const BYTE *, DWORD , UINT);
/*******************************************************************************
* Function Name  : AckWait
* Description    : Stop the main CAN RX process and wait for the remote iap ack 
* Input          : CAN TX msg, waiting time (ms) 
* Return         : IAP message or EOF (not successful...)
*******************************************************************************/
int					AckWait(int t) {
						int to,n=nDev;
						if(t == 0)
							return EOF;
						to = HAL_GetTick() + t;																// nastavi cakalni nterval
						for(nDev=0; HAL_GetTick() < to; _proc_loop()) {
							Watchdog();
							if(n == nDev)
								return EOF;
						}
						_print("...err");
						return 0;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
FRESULT			iapRemote() {
						uint32_t n,k;																		// misc
																														// count lines >>>> n
						for(k=n=_FLASH_TOP; k<FATFS_ADDRESS; k+=sizeof(uint32_t)) {
							if(*(int32_t *)k != _FLASH_BLANK)
								n=k;
							Watchdog();
						}
						Send(_ID_IAP_REQ,NULL,0);												// send iap req. reset slaves
						_wait(500);
						nDev=0;
						Send(_ID_IAP_PING,NULL,0);											// send ping;	
						_wait(500);
						if(nDev==0) {
							_print("\r\nno device detected...");
							return FR_NOT_READY;
						}

						_print("\r\n%d pings received...",nDev);
						_print("\r\nerasing");													// erase 5 pages (att. CubeMX ima drugacne 
																														// oznake za sektorje kot bootloader!!!
						for(k=FLASH_SECTOR_1<<3; k<FLASH_SECTOR_6<<3; k+=FLASH_SECTOR_1<<3) {
							Send(_ID_IAP_ERASE,(payload *)&k,sizeof(int));
							if(!AckWait(3000))														// send erase page, wait for ack
								return FR_NOT_READY;
							_print(".");
						}
						
						_print("\r\nprogramming");
						k=_FLASH_TOP;
						Send(_ID_IAP_ADDRESS,(payload *)&k,sizeof(uint32_t));
						_wait(10);
						while(k <= n) {
							Send(_ID_IAP_DWORD,(payload *)k,sizeof(payload));
							if(!AckWait(200))
								return FR_NOT_READY;
							if((k-_FLASH_TOP) % (8*((n-_FLASH_TOP)/8/20)) == 0)
								_print(".%3d%c%c%c%c%c",(100*(k-_FLASH_TOP))/(n-_FLASH_TOP),'%','\x8','\x8','\x8','\x8');
							k+=sizeof(payload);
						}
						_print(".%3d%c%c%c%c%c",100,'%','\x8','\x8','\x8','\x8');

						Send(_ID_IAP_SIGN,NULL,0);											// send sign command
						if(!AckWait(300))																// wait for ack...
							return FR_NOT_READY;
						_print("\r\nsign ...");
						Send(_ID_IAP_GO,NULL,0);												// send <go> command
						AckWait(0);
						_print("and RUN :)\r\n");
						DecodeCom(NULL);
						return FR_OK;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
int32_t			USBH_Iap(void) {	
FATFS				fs0,fs1;
DIR					dir;
FIL					f0,f1;
BYTE 				buffer[128];   																										//	file copy buffer */
FRESULT 		fr;          																											//	FatFs function common result code	*/
UINT 				br, bw;         																									//	File read/write count */
FILINFO			fno;
	
						if(f_mount(&fs0,"USB:",1) ||
							f_mount(&fs1,"FLASH:",1) ||
							f_chdrive("USB:") || 
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
							if(f_chdrive("USB:") || f_open(&f0,fno.fname,FA_OPEN_EXISTING | FA_READ))
								continue;
							if(f_chdrive("FLASH:") || f_open(&f1,fno.fname,FA_CREATE_ALWAYS | FA_WRITE))
								continue;
							for (;;) {
								if((HAL_GetTick() / 100) % 2)
									_RED(1000);
								else
									_RED(0);
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
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
HAL_StatusTypeDef	FLASH_Program(uint32_t Address, uint32_t Data) {
			HAL_StatusTypeDef status;
			__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPERR  | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR );
			if(*(uint32_t *)Address !=  Data) {
				HAL_FLASH_Unlock();
				do
					status=HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,Address,Data);
				while(status == HAL_BUSY);
				HAL_FLASH_Lock();
			}	
			return status;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
HAL_StatusTypeDef	FLASH_Erase(uint32_t sector, uint32_t n) {
FLASH_EraseInitTypeDef EraseInitStruct;
HAL_StatusTypeDef ret;
uint32_t	SectorError;
			HAL_FLASH_Unlock();
			EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
			EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
			EraseInitStruct.Sector = sector;
			EraseInitStruct.NbSectors = n;
			ret=HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
			HAL_FLASH_Lock(); 
			return ret;
}
/*******************************************************************************
* Function Name	: ff_pack
* Description		: packing flash file system sectors
* Input					: mode flag, 0-analyze, 1-rewrite
* Output				: 
* Return				: percentage of number of sectors reduced
*******************************************************************************/
int		ff_pack(int mode) {
int 	i,f,e,*p,*q,buf[SECTOR_SIZE/4];
int		c0=0,c1=0;

			Watchdog_init(4000);
			f=FATFS_SECTOR;																															// f=koda prvega 128k sektorja
			e=PAGE_SIZE;																																// e=velikost sektorja
			p=(int *)FATFS_ADDRESS;																											// p=hw adresa sektorja
			do {
				do {
					++c0;
					Watchdog();																															//jk822iohfw
					q=&p[SECTOR_SIZE/4+1];																									
					while(p[SECTOR_SIZE/4] != q[SECTOR_SIZE/4] && q[SECTOR_SIZE/4] != -1)		// iskanje ze prepisanih sektorjev
						q=&q[SECTOR_SIZE/4+1];
					if(q[SECTOR_SIZE/4] == -1) {																						// ce ni kopija, se ga prepise na konec fs
						for(i=0; i<SECTOR_SIZE/4;++i)
							buf[i]=~p[i];
						Watchdog();
						if(mode)
							USER_write (0,(uint8_t *)buf,p[SECTOR_SIZE/4],1);										// STORAGE_Write bo po prvem brisanju zacel na
					} else																																	// zacetku !!!
						++c1;
					p=&p[SECTOR_SIZE/4+1]; 
				} while(((int)p)-FATFS_ADDRESS <  e && p[SECTOR_SIZE/4] != -1);						// prepisana cela stran...
				if(mode) {
					_print(".");
					_wait(2);
					FLASH_Erase(f,1);																												// brisi !
				}
				f+=FLASH_SECTOR_1; 
				e+=PAGE_SIZE;
			} while(p[SECTOR_SIZE/4] != -1);	
			if(mode) {
				_print(". OK");
				_wait(2);
				FLASH_Erase(f,1);																													// se zadnja !
				c1=0;
			}
			Watchdog_init(400);
			return(100*c1/c0);
}
/*******************************************************************************
* Function Name	: ff_format
* Description		: formatting flash file system sectors
* Input					: 
* Output				: 
* Return				: 
*******************************************************************************/
void	ff_erase() {
	Watchdog_init(4000);
	for(int i=FATFS_SECTOR; i<FATFS_SECTOR+FLASH_SECTOR_1*PAGE_COUNT;i+=FLASH_SECTOR_1) {
		FLASH_Erase(i,1);
		Watchdog();
	}
}
/*******************************************************************************
* Function Name	: ff_format
* Description		: formatting flash file system sectors
* Input					: 
* Output				: 
* Return				: 
*******************************************************************************/
FRESULT	ff_format(char *drv) {
	uint8_t	wbuf[SECTOR_SIZE];
	ff_erase();
	return f_mkfs(drv,1,CLUSTER_SIZE,wbuf,SECTOR_SIZE);
}
