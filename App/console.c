#include	"console.h"
#include	"ascii.h"
#include	"io.h"
#include	"can.h"
#include	"ws.h"
#include	"leds.h"
#include	<stdlib.h>
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
FATFS			fatfs;
bool			isMounted=false;
int32_t		xxxtest=0x8000;

/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
int			Escape(void) {
int		i=fgetc(stdin);
			if((*stdin->io)->esc == NULL)
				(*stdin->io)->esc=calloc(1,sizeof(esc));
			if(i==__Esc) {
				(*stdin->io)->esc->seq=i;
				(*stdin->io)->esc->timeout=HAL_GetTick()+10;
			} else if(i==EOF) {
				if((*stdin->io)->esc->timeout && (HAL_GetTick() > (*stdin->io)->esc->timeout)) {
					(*stdin->io)->esc->timeout=0;
					return (*stdin->io)->esc->seq;
				}
			} else if((*stdin->io)->esc->timeout) {
				(*stdin->io)->esc->seq=(((*stdin->io)->esc->seq) << 8) | i;
			} else {
				return i;
			}
			return EOF;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
char		*cgets(int c, int mode) {
_buffer		*p=(*stdin->io)->gets;
			
			if(!p)
				p=(*stdin->io)->gets=_buffer_init((*stdin->io)->rx->size);
			switch(c) {
				case EOF:		
				case '\n':
					break;
				case '\r':
					*p->_push = '\0';
					p->_push=p->_pull=p->_buf;
					return(p->_buf);
				case 0x08:
				case 0x7F:
					if(p->_push != p->_pull) {
						--p->_push;
						if(mode)
							_print("\b \b");
					}
					break;
				default:
					if(p->_push != &p->_buf[p->size-1])
						*p->_push++ = c;
					else  {
						*p->_push=c;
						if(mode)
							_print("%c",'\b');
					}
					if(mode) {
						if(c < ' ' || c > 127)
							_print("%c%02X%c",'<',c,'>');
						else
							_print("%c",c);
					}
					break;
			}
			return(NULL);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
char		*trim(char **c) {
			if(!c)
				return NULL;
			if(*c) {
				while(**c==' ') ++*c;
				for(char *cc=strchr(*c,0); *c != cc && *--cc==' '; *cc=0);
			}
			return *c;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
FRESULT DecodePlus(char *c) {
			switch(*trim(&c)) {
				case 'd':
				case 'D':
				for(c=strchr(c,' '); c && *c;)
					debug |= (1<<strtoul(++c,&c,10));
				_DBG=stdout->io;
				break;

				case 'm':
				case 'M':
				c=strchr(c,' ');
				if(!c)
					testMask=(uint32_t)EOF;
				while(c && *c)
					testMask |= (1<<strtoul(++c,&c,10));
				SaveSettings();
				break;
				
				case 'r':
				case 'R':
				HAL_GPIO_WritePin(CAN_TERM_GPIO_Port, CAN_TERM_Pin, GPIO_PIN_RESET);
				_GREEN(200);
				SaveSettings();
				break;

				default:
					return FR_INVALID_NAME;
			}
			return FR_OK;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
FRESULT DecodeMinus(char *c) {
			switch(*trim(&c)) {
				case 'd':
				case 'D':
				c=strchr(c,' ');
				if(!c)
					debug=0;
				while(c && *c)
					debug &= ~(1<<strtoul(++c,&c,10));
				if(!debug)
					_DBG=NULL;
				break;

				case 'm':
				case 'M':
				c=strchr(c,' ');
				if(!c)
					testMask=0;
				while(c && *c)
					testMask &= ~(1<<strtoul(++c,&c,10));
				SaveSettings();
				break;
				
				case 'r':
				case 'R':
				HAL_GPIO_WritePin(CAN_TERM_GPIO_Port, CAN_TERM_Pin, GPIO_PIN_SET);
				_RED(200);
				SaveSettings();
				break;

				default:
				return FR_INVALID_NAME;
	}
	return FR_OK;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
FRESULT DecodeInq(char *c) {
				switch(*trim(&c)) {
				case 'a':
					while(fgetc(stdin) == EOF) {
						_print("\r%.1fV,%.1fV,%.1f'C",
							(float)(pwr.V45*3.3/4095.0*(1.2+47)/1.2),
							(float)(3.3 - (4095-pwr.Vm5)*((1.2+6.8)/1.2*3.3/4095.0)),
							(float)((pwr.T*3.3/4095.0 - 0.76)/2.5e-3+25.0));
						_wait(200);
						}
				break;
		
				case 'v':
					printVersion();
				break;
				
				case 't':
					while(fgetc(stdin) == EOF) {
						uint32_t t=HAL_GetTick()/1000;
						_print("\r%02d:%02d:%02d",t/3600,(t/60)%60,t%60);
						_wait(200);
						}
				break;

				default:
					return FR_INVALID_NAME;
	}
	return FR_OK;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
FRESULT DecodeEq(char *c) {
	return FR_OK;
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
void	remoteLoop() {
	payload 	buf;
	uint32_t	n=0,c;
	uint32_t	timeout=0;
	do {
		c=fgetc(stdin);
		if(c==(uint32_t)EOF) {
			if(n && !timeout) {
				Send(idCAN2COM,&buf,n);
				n=0;
			}
			_wait(2);
			continue;
		}
		if(c==__Esc)
			timeout=HAL_GetTick()+5;
		else
			timeout=0;
		buf.byte[n++]=c;
		if(n == 8) {
			Send(idCAN2COM,&buf,n);
			n=0;
		}
	} while(!timeout || HAL_GetTick() < timeout);
}
//-----------------------------------------------------
FRESULT Remote(int id) {
				if(*stdin->io == canConsole) {
					_print("... not allowed");
					return FR_DISK_ERR;
				}
				if(!devices[id]) {
					Parse(__Esc);
					if(!devices[id]) {
						_print("... no devices");
						DecodeCom(0);
						return FR_DISK_ERR;
					}
				}
				_io **io=_DBG;
				_DBG=stdout->io;
				uint32_t dbg=debug;
				debug |= (1<<DBG_CONSOLE);
				Send(_REMOTE_REQ,(payload *)&devices[id%4],sizeof(int32_t));
				_print("  remote console open...");
				Send(idCAN2COM,(payload *)"\r",1);
				
				remoteLoop(); 

				Send(_REMOTE_REQ,NULL,0);
				_DBG=io;
				debug=dbg;
				_print("  remote console closed...");
				DecodeCom(NULL);
				return FR_OK;
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
FRESULT fErase(int argc, char *argv[]) {
	ff_erase();
	return FR_OK;
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
FRESULT fFormat(int argc, char *argv[]) {
FRESULT	ret=ff_format("FLASH:");
	if(ret==FR_OK) {
		f_mount(&fatfs,"FLASH:",1);
		isMounted=true;
		if(argv[1])
			f_setlabel(argv[1]);
		SaveSettings();
	}
	return ret;
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
FRESULT fPack(int argc, char *argv[]) {
	ff_pack(EOF);
	return FR_OK;
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
FRESULT fCopy(int argc, char *argv[]) {
	FRESULT ret=FR_OK;
	FIL	fs,fd;
	BYTE buff[1024];
	UINT br,bw;
	ret=f_open(&fs,argv[1],FA_READ);
	if(ret==FR_OK) {
		ret=f_open(&fd,argv[2],FA_CREATE_ALWAYS | FA_WRITE);
		if(ret==FR_OK)
			for (;;) {
				ret=f_read(&fs, buff, sizeof buff, &br);
				if (ret != FR_OK || br == 0) 
					break;
				ret=f_write(&fd, buff, br, &bw);
				if (ret != FR_OK || bw < br) 
					break;
			}
	}
	f_close(&fs);
	f_close(&fd);
	return ret;
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
FRESULT fType(int argc, char *argv[]) {
	FRESULT ret=FR_OK;
	FIL	fs;
	ret=f_open(&fs,argv[1],FA_READ);
	if(ret==FR_OK) {
		_print("\r\n");
		while(!f_eof(&fs))
			_print("%c",fgetc((FILE *)&fs));
	}
	f_close(&fs);
	return ret;
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
FRESULT fIap(int argc, char *argv[]) {
	return iapRemote();
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
void		*testProc(void *v) {
	CanTxMsg *m=(CanTxMsg *)v;
	Send(m->hdr.StdId,&m->buf,m->hdr.DLC);
	return testProc;
}
//-----------------------------------------------------
FRESULT fLog(int argc, char *argv[]) {
	FRESULT ret=FR_OK;
	FIL	fs;
	ret=f_open(&fs,"log",FA_READ);
	if(ret==FR_OK) {
		char		c[64];
		short		k0,k1,k2,k3;
		while(!f_eof(&fs)) {
			f_gets(c,sizeof(c),&fs);
			sscanf(c,"%hu,%hu,%hu,%hu",&k0,&k1,&k2,&k3);
			if(k0==atoi(argv[1])) {
				_print("\r\n%d,%5hu,%5hu,%5hu",k0,k2 - sync(k3),k2,k3);
				_wait(10);
			}
		}
		f_close(&fs);
	}
	return ret;
}
//-----------------------------------------------------
FRESULT fTest(int argc, char *argv[]) {
	_proc *p=_proc_find(testProc,NULL);
	if(argv[1]) {
		CanTxMsg	*m=malloc(sizeof(CanTxMsg));
		m->hdr.StdId=_TEST_REQ;
		m->hdr.DLC=1;
		m->buf.hword[0]=atoi(argv[1]);				
		if(argv[2]) {
			if(p) {
				p->dt=atoi(argv[2]);
				memcpy(p->arg,m,sizeof(CanTxMsg));
				free(m);
			} else
				_proc_add(testProc,m,"test",atoi(argv[2]));
		} else {			
			Send(m->hdr.StdId,&m->buf,m->hdr.DLC);
			free(m);
			if(p) {
				p->f=NULL;
				free(p->arg);
			}
		}
	} else if(p) {
		p->f=NULL;
		free(p->arg);
	}
	return FR_OK;		
}
//-----------------------------------------------------
FRESULT fRename(int argc, char *argv[]) {
	return f_rename(argv[1],argv[2]);
}
//-----------------------------------------------------
FRESULT fAddress(int argc, char *argv[]) {
	if(argv[1]) {
		if(0 > atoi(argv[1]) || atoi(argv[1]) > _MAX_DEV)
			return FR_INVALID_PARAMETER;
		idPos=atoi(argv[1])-1;
		SaveSettings();
	}		
	DecodeCom(0);
	_print("  device address %d, %s",idPos+1, strPos[min(4,idPos)]);	
	return FR_OK;
}
//-----------------------------------------------------
FRESULT fDelete(int argc, char *argv[]) {
DIR			dir;
FILINFO	fno;
FRESULT	err;
	for(err=f_findfirst(&dir,&fno,"",argv[1]); err==FR_OK && *fno.fname; err=f_findnext(&dir,&fno))
		err=f_unlink(fno.fname);
	return err;
}
//-----------------------------------------------------
FRESULT mkDir(int argc, char *argv[]) {
	if(argc>1)
		return f_mkdir(argv[1]);
	else
		return f_mkdir("new");
}
//-----------------------------------------------------
FRESULT chDir(int argc, char *argv[]) {
	return f_chdir(argv[1]);
}
//-----------------------------------------------------
FRESULT Dir(int argc, char *argv[]) {
DIR			dir;
FILINFO	fno;
FRESULT	err=FR_OK;

	if(argc>1)
		err=f_findfirst(&dir,&fno,"",argv[1]);
	else
		err=f_findfirst(&dir,&fno,"","*");
	if(err==FR_OK) {
		do {
			_print("\r\n%-16s",fno.fname);
			if (fno.fattrib & AM_DIR)
				_print("%-8s","/");
			else
				_print("%-8d",(int)fno.fsize);
			err=f_findnext(&dir,&fno);
			if(err)
				break;
		} while(*fno.fname);
	}
	return err;
}
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
struct cmd {
	char *str;
	FRESULT (*f)(int, char *[]);
} cmds[]=
{
	{"copy",			fCopy},
	{"rename",		fRename},
	{"delete",		fDelete},
	{"directory",	Dir},
	{"mkd",				mkDir},
	{"mdir",			mkDir},
	{"cdir",			chDir},
	{"type",			fType},
	{"test",			fTest},
	{"log",				fLog},	
	{"address",		fAddress},
	{"iap",				fIap},
	{"format",		fFormat},
	{"erase",			fErase},
	{"pack",			fPack}
};
/*******************************************************************************
* Function Name	:
* Description		:
* Output				:
* Return				:
*******************************************************************************/
FRESULT	DecodeCom(char *c) {
CanRxMsg	rx;
FRESULT		ret=FR_OK;
//__________________________________________________Prompt only response ____
			if(!c) {
				TCHAR	c[128];
				if(isMounted && f_getcwd(c,sizeof(c))==FR_OK) {
					if(!strncmp(c,"0:/",3))
						*(strchr(c,':')-1)='U';
					else
						*(strchr(c,':')-1)=idPos+'1';
				} else
					strcpy(c,"?:/");
				_print("\r\n%s",c);
			}
			else
//___________________________________________________________________________
				switch(*trim(&c)) {
//__________________________________________________
					case 'v':
					case 'V':
						printVersion();
					break;
//__________________________________________________
				case '/':
					ret=f_mount(&fatfs,"FLASH:",1);
					if(ret==FR_OK)
						ret=f_chdrive("FLASH:");
					break;
//__________________________________________________
				case 'u':
				case 'U':
					ret=f_mount(&fatfs,"USB:",1);
					if(ret==FR_OK)
						ret=f_chdrive("USB:");
					break;
//__________________________________________________
				case 'I':
					if(USBH_Iap())
						_GREEN(1000);
					else
						_RED(1000);
				break;		
//__________________________________________________
				case '<': 
					rx.hdr.StdId=strtoul(++c,&c,16);
					do {
						while(*c == ' ') ++c;
						for(rx.hdr.DLC=0; *c && rx.hdr.DLC < 8; ++rx.hdr.DLC)
							rx.buf.byte[rx.hdr.DLC]=strtoul(c,&c,16);
						_buffer_push(_CAN->rx,&rx,sizeof(CanRxMsg));
					} while(*c);
					break;
//__________________________________________________
				case '>': 
					rx.hdr.StdId=strtoul(++c,&c,16);
					do {
						while(*c == ' ') ++c;
						for(rx.hdr.DLC=0; *c && rx.hdr.DLC < 8; ++rx.hdr.DLC)
							rx.buf.byte[rx.hdr.DLC]=strtoul(c,&c,16);
						Send(rx.hdr.StdId,&rx.buf,rx.hdr.DLC);
					} while(*c);
					break;
//__________________________________________________
				case '?':
					return DecodeInq(++c);
				case '+':
					return DecodePlus(++c);
				case '-':
					return DecodeMinus(++c);
				case '=':
					return DecodeEq(++c);
//_______________________________________________________________________________________
				default: {
					char *q[8];
					int n=0;
					q[n]=strtok(c," ,");
					while(q[n]) {
						q[++n]=strtok(NULL," ,");
					}
					for(int i=0; i<sizeof(cmds)/sizeof(struct cmd); ++i)
						if(q[0] && !strncmp(q[0],cmds[i].str,strlen(q[0])))
							return cmds[i].f(n,q);
					ret = FR_INVALID_NAME;
					}
				}				
			return ret;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	Parse(int i) {
char	*c;
			switch(i) {
				case EOF:																				// empty usart
					break;				
				
				case __CtrlZ:																		// call watchdog reset
					while(1);				
				
				case __CtrlY:																		// call system reset
					NVIC_SystemReset();	
//__________________________________________________
				case __Esc:
				{
_io 			**io=_DBG;
					_DBG=stdout->io;
uint32_t	dbg=debug;
					debug |= (1<<DBG_CONSOLE);
					
					nDev=0;
					Send(_ID_IAP_PING,NULL,0);
					_wait(500);
					_print("%6d dev. found",nDev);
					DecodeCom(NULL);
					_DBG=io;
					debug=dbg;
				}
				break;				
				
				case __f1:
				case __F1: Remote(0); break;
				case __f2:
				case __F2: Remote(1); break;
				case __f3:
				case __F3: Remote(2); break;
				case __f4:
				case __F4: Remote(3); break;

				case __f9:
				case __F9:
					MX_USB_HOST_DeInit();
					HAL_USBD_Setup();
					UsbDevice_Init();
				break;
				
				case __f10:
				case __F10:
					UsbDevice_DeInit();
					MX_USB_HOST_Init();
				break;
				
				case __f12:
				case __F12:
					iapRemote();
				break;


				case __Up:
					testRef = min(++testRef,128);
					testReq=1;
				break;

				case __Down:
					testRef = max(--testRef,1);
					testReq=1;
				break;

				default:
					c=cgets(i,EOF);				
					if(c) {		
						while(*c==' ') ++c;	
						i=DecodeCom(c);
						if(*c && i)				
							_print("... WTF(%d)",i);
						i=DecodeCom(NULL);
					}
			}
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	*console(void *v) {
_io		**in=stdin->io;
_io 	**out=stdout->io;
			_stdio((_io **)v);
			Parse(Escape());
			stdin->io=in;
			stdout->io=out;
			return v;
}	
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
****************************f***************************************************/
void	printVersion() {
			DecodeCom(0);
			_print("  V%d.%02d %s <%08X>",
				SW_version/100,SW_version%100,
					__DATE__,
						HAL_CRC_Calculate(&hcrc,(uint32_t *)_FLASH_TOP, (FATFS_ADDRESS-_FLASH_TOP)/sizeof(int)));	
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
****************************f***************************************************/
FRESULT	LoadSettings(void) {
	FIL f;
	FRESULT err;
	TCHAR	c[128];
	
	err=f_chdrive("FLASH:");							if(err != FR_OK)	return err;
	err=f_mount(&fatfs,"FLASH:",1);				if(err != FR_OK)	return err;
	isMounted=true;
	err=f_open(&f,"/L405.ini",FA_READ);		if(err != FR_OK)	return err;
	f_gets(c,sizeof(c),&f);
	sscanf(c,"%03X\n", &idPos);
	
	f_gets(c,sizeof(c),&f);
	uint32_t state=GPIO_PIN_SET;
	sscanf(c,"%03X\n", &state);
	HAL_GPIO_WritePin(CAN_TERM_GPIO_Port, CAN_TERM_Pin,(GPIO_PinState)state);
	
	f_gets(c,sizeof(c),&f);
	sscanf(c,"%08X\n", &testMask);

	f_close(&f);

	return FR_OK;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
****************************f***************************************************/
FRESULT	SaveSettings(void) {
			FIL f;
			FRESULT err=f_open(&f,"/L405.ini",FA_WRITE | FA_CREATE_ALWAYS);
			if(err==FR_OK) {
				f_printf(&f,"%-10d;dev. address\n", idPos);
				f_printf(&f,"%-10d;term. pin\n",HAL_GPIO_ReadPin(CAN_TERM_GPIO_Port, CAN_TERM_Pin));
				f_printf(&f,"%08X  ;mask\n", testMask);
				f_close(&f);
			}
			return err;
}
