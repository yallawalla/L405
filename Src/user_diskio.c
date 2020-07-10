/* USER CODE BEGIN Header */
/**
 ******************************************************************************
  * @file    user_diskio.c
  * @brief   This file includes a diskio driver skeleton to be completed by the user.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
 /* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/* 
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future. 
 * Kept to ensure backward compatibility with previous CubeMx versions when 
 * migrating projects. 
 * User code previously added there should be copied in the new user sections before 
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"
#include <stdio.h>
#include "io.h"
#include "proc.h"
/* Private typedef -----------------------------------------------------------*/
extern		IWDG_HandleTypeDef hiwdg;
/* Private define ------------------------------------------------------------*/
#define 	FATFS_SECTOR	FLASH_SECTOR_6
#define		FATFS_ADDRESS 0x08040000
#define		PAGE_SIZE			0x20000
#define		PAGE_COUNT		5
#define		SECTOR_SIZE		512
#define		CLUSTER_SIZE	4*SECTOR_SIZE
#define		SECTOR_COUNT	(int)(PAGE_SIZE*PAGE_COUNT/(SECTOR_SIZE + sizeof(uint32_t)))
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
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	*Watchdog(void) {
			HAL_IWDG_Refresh(&hiwdg);	
			return(Watchdog);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	Watchdog_init(int t) {
			hiwdg.Instance = IWDG;
			hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
			hiwdg.Init.Reload = t;
			HAL_IWDG_Init(&hiwdg);
}
/*******************************************************************************
* Function Name	: ff_pack
* Description		: packing flash dile system sectors
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
							disk_write (0,(uint8_t *)buf,p[SECTOR_SIZE/4],1);										// STORAGE_Write bo po prvem brisanju zacel na
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
FRESULT	ff_format(char *drv) {
	uint8_t	wbuf[SECTOR_SIZE];
	Watchdog_init(4000);
	for(int i=FATFS_SECTOR; i<FATFS_SECTOR+FLASH_SECTOR_1*PAGE_COUNT;i+=FLASH_SECTOR_1) {
		FLASH_Erase(i,1);
		Watchdog();
	}
	return f_mkfs(drv,1,CLUSTER_SIZE,wbuf,SECTOR_SIZE);
}
/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize (BYTE pdrv);
DSTATUS USER_status (BYTE pdrv);
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
  DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);  
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
  DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef  USER_Driver =
{
  USER_initialize,
  USER_status,
  USER_read, 
#if  _USE_WRITE
  USER_write,
#endif  /* _USE_WRITE == 1 */  
#if  _USE_IOCTL == 1
  USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_initialize (
	BYTE pdrv           /* Physical drive nmuber to identify the drive */
)
{
  /* USER CODE BEGIN INIT */
    Stat = RES_OK;
    return Stat;
  /* USER CODE END INIT */
}
 
/**
  * @brief  Gets Disk Status 
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS USER_status (
	BYTE pdrv       /* Physical drive number to identify the drive */
)
{
  /* USER CODE BEGIN STATUS */
    Stat = RES_OK;
    return Stat;
  /* USER CODE END STATUS */
}

/**
  * @brief  Reads Sector(s) 
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT USER_read (
	BYTE pdrv,      /* Physical drive nmuber to identify the drive */
	BYTE *buff,     /* Data buffer to store read data */
	DWORD sector,   /* Sector address in LBA */
	UINT count      /* Number of sectors to read */
)
{
  /* USER CODE BEGIN READ */
	int i,*p,*q=NULL;
	
	for(p=(int *)FATFS_ADDRESS; (int)p < FATFS_ADDRESS + PAGE_SIZE*PAGE_COUNT &&  p[SECTOR_SIZE/4]!=-1; p=&p[SECTOR_SIZE/4+1]) {
		if(p[SECTOR_SIZE/4] == sector)
			q=p;
	}
	if((int)p >= FATFS_ADDRESS + PAGE_SIZE*PAGE_COUNT)
		return RES_ERROR;
	if(q)
		p=q;
	q=(int *)buff;
	for(i=0;i<SECTOR_SIZE/4; ++i)
		*q++=~(*p++);	
	if(--count)
		return USER_read (pdrv, (uint8_t *)q, ++sector, count);
	return RES_OK;
  /* USER CODE END READ */
}

/**
  * @brief  Writes Sector(s)  
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT USER_write (
	BYTE pdrv,          /* Physical drive nmuber to identify the drive */
	const BYTE *buff,   /* Data to be written */
	DWORD sector,       /* Sector address in LBA */
	UINT count          /* Number of sectors to write */
)
{ 
  /* USER CODE BEGIN WRITE */
  /* USER CODE HERE */
	int i,*p,*q=NULL;
		
	for(p=(int *)FATFS_ADDRESS; p[SECTOR_SIZE/4]!=-1; p=&p[SECTOR_SIZE/4+1])
		if((int)p >= FATFS_ADDRESS + PAGE_SIZE*PAGE_COUNT)
			return RES_ERROR;

	q=(int *)buff;
	for(i=0; i<SECTOR_SIZE/4; ++i)	
		if(*q++)
			break;

	if(i<SECTOR_SIZE/4) {												// all zeroes ???
		q=(int *)buff;
		for(i=0; i<SECTOR_SIZE/4; ++i,++p,++q)
			FLASH_Program((int)p,~(*q));
		FLASH_Program((int)p,sector);
	}
	
	if(--count)
		return USER_write (pdrv, (uint8_t *)q, ++sector, count);
	return RES_OK;
  /* USER CODE END WRITE */
}
#endif /* _USE_WRITE == 1 */

/**
  * @brief  I/O control operation  
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT USER_ioctl (
	BYTE pdrv,      /* Physical drive nmuber (0..) */
	BYTE cmd,       /* Control code */
	void *buff      /* Buffer to send/receive control data */
)
{
  /* USER CODE BEGIN IOCTL */
	DRESULT res;
	switch (cmd) {
  case CTRL_SYNC :		/* Make sure that no pending write process */
    res = RES_OK;
    break;
    
  case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
		*(DWORD*)buff = (DWORD) SECTOR_COUNT;
		res = RES_OK;
    break;
    
  case GET_SECTOR_SIZE :	/* Get R/W sector size (WORD) */
		*(DWORD*)buff = (DWORD) SECTOR_SIZE;
    res = RES_OK;
    break;
    
  case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
		*(DWORD*)buff = (DWORD) PAGE_SIZE;
		res = RES_OK;    
    break;
   
  default:
    res = RES_PARERR;
  }
		return res; 
  /* USER CODE END IOCTL */
}
#endif /* _USE_IOCTL == 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
