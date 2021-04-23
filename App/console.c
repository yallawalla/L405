#include	"console.h"
#include	"ascii.h"
#include	"io.h"
#include	"can.h"
#include	"ws.h"
#include	"ssd1306.h"
#include	"leds.h"
#include	<stdlib.h>
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
FATFS			fatfs;
bool			isMounted=false,iapInproc=false;
uint32_t	debug,error,errmask,pinV;
_io				**_DBG;
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
				break;

				case 'e':
				case 'E':
				c=strchr(c,' ');
				if(!c)
					errmask=0;
				while(c && *c)
					errmask &= ~(1<<strtoul(++c,&c,10));
				break;
				
				case 'r':
				case 'R':
				HAL_GPIO_WritePin(CAN_TERM_GPIO_Port, CAN_TERM_Pin, GPIO_PIN_RESET);
				_GREEN(200);
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
				break;

				case 'e':
				case 'E':
				c=strchr(c,' ');
				if(!c)
					errmask=(uint32_t)EOF;
				while(c && *c)
					errmask |= (1<<strtoul(++c,&c,10));
				break;
				
				case 'r':
				case 'R':
				HAL_GPIO_WritePin(CAN_TERM_GPIO_Port, CAN_TERM_Pin, GPIO_PIN_SET);
				_RED(200);
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
				case 'u':
					while(fgetc(stdin) == EOF) {
						_print("\r%.1fV,%.1fV,%.1f'C",_V45,_VM5,_TEMP);
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
				switch(*trim(&c)) {
					case 'u':
					case 'U':
					for(c=strchr(c,' '); c && *c;)
						pinV = strtoul(++c,&c,10);
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
						DecodeCom(NULL);
						return FR_DISK_ERR;
					}
				}
				_io **io=_DBG;
				_DBG=stdout->io;
				uint32_t dbg=debug;
				debug |= (1<<DBG_CONSOLE);
				Send(_REMOTE_REQ,(payload *)&devices[id%4],sizeof(uint16_t));
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
	iapInproc=true;
	FRESULT ret=iapRemote();
	iapInproc=false;
	return ret;
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
	if(argv[1]) {
		testRef=atof(argv[1]);
		testReq=1;
		tim *t=&timStack[testRef];
		t->htim->hdma[((t->Channel)>>2)+1]=NULL;
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
	}		
	DecodeCom(NULL);
	_print("  device address %d, %s",idPos+1, strPos[min(_MAX_HEAD,idPos)]);	
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
//-----------------------------------------------------
void	*watch1(void *v) {
	static uint32_t	offset=0;
	char	t[16];
	if(v) {
		if(!offset) {
			_proc_add(watch1,NULL,"watch",1000);
			SSD1306_Fill(SSD1306_COLOR_BLACK);
		}
		offset=(uint32_t)v;
	}
	sprintf(t,"%2d:%02d:%02d",((HAL_GetTick()-offset)/1000/3600)%24,((HAL_GetTick()-offset)/1000/60)%60,((HAL_GetTick()-offset)/1000)%60);
	SSD1306_GotoXY (0,0);
	SSD1306_Puts (t, &Font_16x26, SSD1306_COLOR_WHITE);
	return watch1;
}
//-----------------------------------------------------
float rx(float x,float y,float fi) {
	return x*cos(fi)+y*sin(fi)+0.5;
}
float ry(float x,float y,float fi) {
	return y*cos(fi)-x*sin(fi)+0.5;
}
//-----------------------------------------------------
void	*watch(void *v) {
	#define THICK	3
	#include	<math.h>
	char	c[16];
	static uint32_t	offset=0;
	if(v) {
		if(!offset) {
			_proc_add(watch,NULL,"watch",1000);
			SSD1306_Fill(SSD1306_COLOR_BLACK);
		}
		offset=(uint32_t)v;
	}
	SSD1306_Fill(SSD1306_COLOR_BLACK);
	sprintf(c,"%2d:%02d:%02d",((HAL_GetTick()-offset)/1000/3600)%24,((HAL_GetTick()-offset)/1000/60)%60,((HAL_GetTick()-offset)/1000)%60);
	SSD1306_GotoXY (0,0);
	SSD1306_Puts (c, &Font_7x10, SSD1306_COLOR_WHITE);
	
	float pi=3.14159265359f;
	float fi=(HAL_GetTick()-offset)/1000.0f/30.0f*pi;
	float Cos=cos(fi),Sin=sin(fi);
	SSD1306_DrawRectangle(64,0,63,63,SSD1306_COLOR_WHITE);
	for(int8_t i=0; i<4; ++i)
		SSD1306_DrawCircle(rx(0,28,pi*i/2)+96,32-ry(0,28,pi*i/2),1,SSD1306_COLOR_WHITE);
	for(int8_t i=0; i<12; ++i)
		SSD1306_DrawPixel(rx(0,28,pi*i/6)+96,32-ry(0,28,pi*i/6),SSD1306_COLOR_WHITE);
	SSD1306_DrawLine(rx(0,-8,fi)+96,32-ry(0,-8,fi), rx(0,28,fi)+96, 32-ry(0,28,fi) ,SSD1306_COLOR_WHITE);
	fi /= 60.0f;	
	SSD1306_DrawFilledTriangle(rx(-THICK,-8,fi)+96,32-ry(-THICK,-8,fi), rx(THICK,-8,fi)+96,32-ry(THICK,-8,fi), rx(0,28,fi)+96, 32-ry(0,28,fi) ,SSD1306_COLOR_BLACK);
	SSD1306_DrawTriangle(rx(-THICK,-8,fi)+96,32-ry(-THICK,-8,fi), rx(THICK,-8,fi)+96,32-ry(THICK,-8,fi), rx(0,28,fi)+96, 32-ry(0,28,fi) ,SSD1306_COLOR_WHITE);
	fi /= 12.0f;
	SSD1306_DrawFilledTriangle(rx(-THICK,-8,fi)+96,32-ry(-THICK,-8,fi), rx(THICK,-8,fi)+96,32-ry(THICK,-8,fi), rx(0,15,fi)+96, 32-ry(0,15,fi) ,SSD1306_COLOR_BLACK);
	SSD1306_DrawTriangle(rx(-THICK,-8,fi)+96,32-ry(-THICK,-8,fi), rx(THICK,-8,fi)+96,32-ry(THICK,-8,fi), rx(0,15,fi)+96, 32-ry(0,15,fi) ,SSD1306_COLOR_WHITE);
	return watch;
}
//-----------------------------------------------------
FRESULT fWatch(int argc, char *argv[]) {
	uint32_t t=HAL_GetTick()-3600*1000*atoi(argv[1]) -60000*atoi(argv[2]);
	watch((void *)t);
	return FR_OK;		
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
	{"watch",			fWatch},
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
						q[++n]=strtok(NULL," ,:");
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
					_print("reload...");
					_wait(10);
					while(1);				
				
				case __CtrlY:																		// call system reset
					Send(_ID_RESET,NULL,0);
					_wait(10);
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
					_print("     ser.%04hX, hash <%08X>, %-12s(%04hX)\r\n",idDev,idCrc, strPos[min(_MAX_HEAD,idPos)],error);
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
					Watchdog_init(4000);
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
					DecodeCom("iap");
				break;

				case __f11:
				case __F11:
					SaveSettings();
				_print("...saved");
				DecodeCom(NULL);
				break;

				case __Up:
					testRef = min(++testRef,128);
					testReq=1;
				break;

				case __Down:
					testRef = max(--testRef,1);
					testReq=1;
				break;
				
				case __PageUp:
					if(flashLock(true))
						_print("locked");
					else
						_print("error");
					DecodeCom(NULL);
				break;
				case __PageDown:
					if(flashLock(false))
						_print("unlocked");
					else
						_print("error");
					DecodeCom(NULL);
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
			DecodeCom(NULL);
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
	sscanf(c,"%03X", &idPos);
	
	f_gets(c,sizeof(c),&f);
	uint32_t state=GPIO_PIN_SET;
	sscanf(c,"%03X", &state);
	HAL_GPIO_WritePin(CAN_TERM_GPIO_Port, CAN_TERM_Pin,(GPIO_PinState)state);
	
	f_gets(c,sizeof(c),&f);
	sscanf(c,"%08X", &testMask);
	f_gets(c,sizeof(c),&f);
	sscanf(c,"%08X", &errmask);
	f_gets(c,sizeof(c),&f);
	sscanf(c,"%08X", &pinV);

	for(int i=0; i<6; ++i) {
		f_gets(c,sizeof(c),&f);
		sscanf(c,"%hu,%hhu,%hhu", &ws[i].colour.h, &ws[i].colour.s, &ws[i].colour.v);
	}

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
				f_printf(&f,"%-12d;dev. address\n", idPos);
				f_printf(&f,"%-12d;term. pin\n",HAL_GPIO_ReadPin(CAN_TERM_GPIO_Port, CAN_TERM_Pin));
				f_printf(&f,"%08X    ;mask\n", testMask);
				f_printf(&f,"%08X    ;error mask\n", errmask);
				f_printf(&f,"%8d    ;pin voltage\n", pinV);
				for(int i=0; i<6; ++i)
					f_printf(&f,"%3d,%3d,%3d ;colour %d\n", ws[i].colour.h, ws[i].colour.s, ws[i].colour.v,i);
				f_close(&f);
			}
			return err;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
****************************f***************************************************/
void	*selftest(void) {
	for(tim *t=timStack; t->htim; ++t) {
		if(HAL_GPIO_ReadPin(t->gpio,t->pin) != GPIO_PIN_SET)
			_SETERR(ERR_PIN+t->sect);
	}
	if(abs(_V45 - pinV > 3.0f))	 _SETERR(ERR_V45);
	if(abs(_VM5 - 5.0f  > 0.5f)) _SETERR(ERR_VM5);
	if(error)	
		_RED(200);
	return selftest;
}
