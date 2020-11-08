#include	"console.h"
#include	"ascii.h"
#include	"io.h"
#include	"can.h"
#include	"ws.h"
#include	<stdlib.h>
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
int		Escape(void) {
int		i=getchar();
			if(stdin->io->esc == NULL)
				stdin->io->esc=calloc(1,sizeof(esc));
			if(i==__Esc) {
				stdin->io->esc->seq=i;
				stdin->io->esc->timeout=HAL_GetTick()+10;
			} else if(i==EOF) {
				if(stdin->io->esc->timeout && (HAL_GetTick() > stdin->io->esc->timeout)) {
					stdin->io->esc->timeout=0;
					return stdin->io->esc->seq;
				}
			} else if(stdin->io->esc->timeout) {
				stdin->io->esc->seq=((stdin->io->esc->seq) << 8) | i;
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
char	*cgets(int c, int mode)
{
_buffer		*p=__stdin.io->gets;
			
			if(!p)
				p=__stdin.io->gets=_buffer_init(__stdin.io->rx->size);
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
char	*trim(char **c) {
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
				case 'D':
				for(c=strchr(c,' '); c && *c;)
					debug |= (1<<strtoul(++c,&c,10));
				_DBG=stdout->io;
				break;
				
				case 't':
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
				case 'D':
				for(c=strchr(c,' '); c && *c;)
					debug &= ~(1<<strtoul(++c,&c,10));
				break;
		
				case 't':
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
FRESULT DecodeEq(char *c) {
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
//-----------------------------------------------------
FRESULT fRename(int argc, char *argv[]) {
	return f_rename(argv[1],argv[2]);
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
	{"type",			fType}
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
FATFS			fatfs;
//__________________________________________________Prompt only response ____
			if(!c) {
				TCHAR	c[128];
				f_getcwd(c,sizeof(c));
				_print("\r\n%s>",c);
			}
			else
//___________________________________________________________________________
				switch(*c) {
//__________________________________________________SW version query_________
//__________________________________________________
				case '0':
					ret=f_mount(&fatfs,"FLASH:",1);
					if(ret==FR_OK)
						ret=f_chdrive("FLASH:");
					break;
//__________________________________________________
				case '1':
					ret=f_mount(&fatfs,"USB:",1);
					if(ret==FR_OK)
						ret=f_chdrive("USB:");
					break;
//__________________________________________________
				case 'F':
					ret=ff_format("FLASH:");
					if(ret==FR_OK)
						SaveSettings();
					break;
//__________________________________________________
				case 'P':
					printf("...%d",ff_pack(EOF));
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
						if(!strncmp(q[0],cmds[i].str,strlen(q[0])))
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
				case __CtrlD:
				{
					_io *io=_DBG;
					_DBG=stdout->io;
					ackCount=0;
					Send(_ID_IAP_PING,NULL,0);
					_wait(500);
					ackMax=ackCount;
					_print("%3d dev. found",ackMax);
					DecodeCom(NULL);
					_DBG=io;
					printVersion();
				}
				break;				
				case __f9:
				case __F9:
							VCP_USB_DEVICE_Init();
				break;
				case __f10:
				case __F10:
							MSC_USB_DEVICE_Init();
				break;
				case __f11:
				case __F11:
							MX_USB_HOST_Init();
				break;
				
				case __f12:
				case __F12:
					iapRemote();
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
_io		*in=stdin->io;
_io 	*out=stdout->io;
_io 	*current=*(_io **)v;
			_stdio(current);
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
			_print("\rV%d.%02d %s <%08X>",
				SW_version/100,SW_version%100,
					__DATE__,
						HAL_CRC_Calculate(&hcrc,(uint32_t *)_FLASH_TOP, (FATFS_ADDRESS-_FLASH_TOP)/sizeof(int)));	
			DecodeCom(NULL);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
****************************f***************************************************/
FRESULT	LoadSettings(void) {
			FIL f;
			FRESULT err=f_open(&f,"/L405.ini",FA_READ);
			if(err==FR_OK) {
				TCHAR	c[128];
				f_gets(c,sizeof(c),&f);
				sscanf(c,"%03X\n", &idPos);
				
				f_gets(c,sizeof(c),&f);
				uint32_t state=GPIO_PIN_SET;
				sscanf(c,"%03X\n", &state);
				HAL_GPIO_WritePin(CAN_TERM_GPIO_Port, CAN_TERM_Pin,(GPIO_PinState)state);
				
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
FRESULT	SaveSettings(void) {
			FIL f;
			FRESULT err=f_open(&f,"/L405.ini",FA_WRITE | FA_CREATE_ALWAYS);
			if(err==FR_OK) {
				f_printf(&f,"%03X\n", idPos);
				f_printf(&f,"%03X\n",HAL_GPIO_ReadPin(CAN_TERM_GPIO_Port, CAN_TERM_Pin));
				f_close(&f);
			}
			return err;
}
