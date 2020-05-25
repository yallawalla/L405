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
static struct {	
			uint32_t	seq;
			uint32_t	timeout;
			} esc, rpt;
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
int		i=getchar();
			if(i==EOF) {
				if(esc.timeout && (HAL_GetTick() > esc.timeout)) {
					esc.timeout=0;
					return esc.seq;
					}
				if(rpt.timeout && (HAL_GetTick() > rpt.timeout)) {
					rpt.timeout=0;
					return rpt.seq;
					}
			} else if(esc.timeout > 0) {
				esc.seq=(esc.seq<<8) | i;
				if(i=='~' || i=='A' || i=='B' || i=='C' || i=='D') {
					esc.timeout=0;
					return esc.seq;
				}
			} else if(i==__Esc) {
				esc.timeout=HAL_GetTick()+10;
				esc.seq=i;
			} else {
				esc.timeout=0;
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
char *trim(char **c) {
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
FATFS			fatfs;
DIR				dir;
FILINFO		fno;
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
FRESULT DecodeInq(char *c) {
	switch(*trim(&c)) {
		case 'v':
			printVersion();
		break;
		case 't':
			for(int i=0; i<4096; ++i) {
				_print("\r\n");
				for(int j=0x800; j; j/=2)
					i & j ? _print(" 1") : _print(" 0");
				_print("\r\n");				
				for(int j=0x800000; j; j/=2)
					DecodeTab[i] & j ? _print("1") : _print("0");
			}
		break;
		case 'd': {
FRESULT	err=f_findfirst(&dir,&fno,"/","*");
			if(err)
				return err;	
			do {
				_print("\r\n%-16s",fno.fname);
				if (fno.fattrib & AM_DIR)
					_print("%-8s","/");
				else
					_print("%-8d",(int)fno.fsize);
				err=f_findnext(&dir,&fno);
				if(err)
					return err;	
			} while(*fno.fname);
			break;
			}
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
FRESULT	DecodeCom(char *c) {
CanRxMsg	rx;
//__________________________________________________Prompt only response ____
			if(!c)
				_print("\r\n>");
			else
//___________________________________________________________________________
			switch(*c) {
//__________________________________________________SW version query_________
//__________________________________________________
				case '0':
					f_mount(&fatfs,"0:",1);
					f_chdrive("0:");
					break;
//__________________________________________________
				case '1':
					f_mount(&fatfs,"1:",1);
					f_chdrive("1:");
					break;
//__________________________________________________
				case 'F':
				{
					FIL f;
					ff_format("0:");
					SaveSettings();
					f_printf(&f,"ok...");
				}
				break;
//__________________________________________________
				case 'P':
					printf("...%d",ff_pack(0));
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
				case '?':
					return DecodeInq(++c);
//_______________________________________________________________________________________
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
void	Parse(int i) {
char	*c;
			switch(i) {
				case EOF:																				// empty usart
					break;				
				case __CtrlZ:																		// call watchdog reset
					while(1);				
				case __CtrlY:																		// call system reset
					NVIC_SystemReset();	
				case __Esc:
					Send(_ID_IAP_PING,NULL,0);
				break;
					
				case __f9:
				case __F9:
							MX_USB_HOST_DeInit();
							MSC_USB_DEVICE_DeInit();
							VCP_USB_DEVICE_Init();
				break;
				case __f10:
				case __F10:
							MX_USB_HOST_DeInit();
							VCP_USB_DEVICE_DeInit();
							MSC_USB_DEVICE_Init();
				break;
				case __f11:
				case __F11:
							VCP_USB_DEVICE_DeInit();
							MSC_USB_DEVICE_DeInit();
							MX_USB_HOST_Init();
				break;
				
				case __f12:
				case __F12:
					iapRemote("L405.hex");
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
			_print(" %d.%02d %s <%08X>",
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
			FRESULT err=f_open(&f,"/L405.ini",FA_READ);
			if(err==FR_OK) {
				TCHAR	c[128];
				f_gets(c,sizeof(c),&f);
				sscanf(c,"%03X\n", &idPos);
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
				f_close(&f);
			}
			return err;
}
