#include	"can.h"
#include	"console.h"
#include	"ws.h"
#include	"leds.h"
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				: 
* Return				:
*******************************************************************************/
_io				*_CAN, *canConsole,**_DBG;
//______________________________________________________________________________________
const char *strPos[]={"front left","front right","rear right","rear left","console"};
uint32_t	idDev,
					nDev,
					idCrc,
					debug,
					idPos,
					testMask;
payload		py;
uint32_t	devices[_MAX_DEV];
uint32_t	refCnt,flushTout;

uint8_t		prio[6][3]={
{0,0,2},
{0,1,2},
{2,2,2},
{0,0,2},
{0,1,2},
{2,2,2}
};

tim timStack[] = {
	{NULL,&htim1,TIM_CHANNEL_1,0,0,_DMA},					//PA8		1A1
	{NULL,&htim1,TIM_CHANNEL_2,1,0,_DMA},					//PA9		2A1
	{NULL,&htim1,TIM_CHANNEL_3,2,0,_DMA},					//PA10	3A1
	
	{NULL,&htim3,TIM_CHANNEL_2,0,1,_DMA},					//PA7		1A2
	{NULL,&htim4,TIM_CHANNEL_2,0,4,_DMA},					//PB7		1B2
	
	{NULL,&htim5,TIM_CHANNEL_1,1,1,_DMA},					//PA0 	2A2
	{NULL,&htim5,TIM_CHANNEL_2,1,4,_DMA},					//PA1 	2B2
	
	{NULL,&htim5,TIM_CHANNEL_3,2,1,_DMA},					//PA2 	3A2
	{NULL,&htim5,TIM_CHANNEL_4,2,4,_DMA},					//PA3		3B2
	
	{NULL,&htim8,TIM_CHANNEL_2,0,3,_DMA},					//PC7		1B1
	{NULL,&htim8,TIM_CHANNEL_3,1,3,_DMA},					//PC8		2B1
	{NULL,&htim8,TIM_CHANNEL_4,2,3,_DMA},					//PC9		3B1

	{NULL,&htim3,TIM_CHANNEL_1,0,2,_IT},					//PA6		1A3
	{NULL,&htim3,TIM_CHANNEL_3,1,2,_IT},					//PB0		2A3
	{NULL,&htim3,TIM_CHANNEL_4,2,2,_IT},					//PB1 	3A3
	
	{NULL,&htim4,TIM_CHANNEL_1,0,5,_IT},					//PB6 	1B3
	{NULL,&htim4,TIM_CHANNEL_3,1,5,_IT},					//PB8 	2B3
	{NULL,&htim4,TIM_CHANNEL_4,2,5,_IT},					//PB9 	3B3

	{NULL,NULL,0,0,0,_IT}
};

uint32_t	filter_count;
uint32_t	testReq,testRef=64;
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if(htim == &htim3) {
		HAL_GPIO_WritePin(GPIOA, TREF_Pin, GPIO_PIN_SET);	// ????
		if(++refCnt % 128 == 0 && idPos > 3)
			Send(_ID_SYNC_REQ,NULL,0);
		if((refCnt % 128 == testRef) && testReq && idPos > 3) {
			testReq=0;
			HAL_GPIO_TogglePin(TREF_GPIO_Port, TREF_Pin); 
			HAL_GPIO_TogglePin(TREF_GPIO_Port, TREF_Pin); 	
		}
	}		
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	uint32_t	i;
	tim				*p=NULL;
	
	if(htim->Instance==TIM3)	p=&timStack[12];
	if(htim->Instance==TIM4)	p=&timStack[15];

	if(p)
		switch(htim->Channel) {
			case HAL_TIM_ACTIVE_CHANNEL_1:
				i=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_1);
				_buffer_push(p[0].dma, &i, sizeof(uint32_t));
			break;
			case HAL_TIM_ACTIVE_CHANNEL_3:
				i=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_3);
				_buffer_push(p[1].dma, &i, sizeof(uint32_t));
			break;
			case HAL_TIM_ACTIVE_CHANNEL_4:
				i=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_4);
				_buffer_push(p[2].dma, &i, sizeof(uint32_t));
			break;
			default:
				break;
		}
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	Send(int id,  payload *buf, int len) {
	CanTxMsg tx={{0,0,CAN_ID_STD,CAN_RTR_DATA,0,DISABLE},{0,0}};
	uint32_t mailbox;
	tx.hdr.StdId=id;
	tx.hdr.DLC=len;
	if(buf && len)
		memcpy(&tx.buf,buf,len);
	if(HAL_CAN_AddTxMessage(&hcan2, &tx.hdr, (uint8_t *)&tx.buf, &mailbox) != HAL_OK)
		_buffer_push(_CAN->tx,&tx,sizeof(CanTxMsg));
	_GREEN(20);
	_DEBUG(DBG_CAN_TX,"\r%5d: > %03X",HAL_GetTick() % 10000,tx.hdr.StdId);
	for(int i=0; i<tx.hdr.DLC; ++i)
		_DEBUG(DBG_CAN_TX," %02X",tx.buf.bytes[i]);
	_DEBUG(DBG_CAN_TX,"%s","\r\n");
}
/*******************************************************************************
* Function Name  : CAN_Initialize
* Description    : Configures the CAN, transmit and receive using interrupt.
* Input          : None
* Output         : None
* Return         : PASSED if the reception is well done, FAILED in other case
*******************************************************************************/
void	canFilterCfg(int id1, int mask1, int id2, int mask2) {
CAN_FilterTypeDef  sFilterConfig;

  sFilterConfig.FilterBank = 14 + filter_count++;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
	sFilterConfig.FilterIdHigh = id1<<5;
	sFilterConfig.FilterMaskIdHigh = mask1<<5;	
	sFilterConfig.FilterIdLow =  id2<<5;
	sFilterConfig.FilterMaskIdLow = mask2<<5;
  sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  sFilterConfig.FilterActivation = ENABLE;
  sFilterConfig.SlaveStartFilterBank = 0;
	HAL_CAN_ConfigFilter(&hcan2, &sFilterConfig) ;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan) {
	CanRxMsg msg;
	HAL_CAN_GetRxMessage(hcan,CAN_RX_FIFO0,&msg.hdr,(uint8_t *)&msg.buf);
	if(msg.hdr.StdId==_ID_SYNC_REQ) {
		msg.buf.hword[3]=	htim1.Instance->CNT;
	}
	_buffer_push(_CAN->rx,&msg,sizeof(CanRxMsg));
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef* hcan) {
	HAL_CAN_RxFifo0MsgPendingCallback(hcan);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* hcan) {
	CanTxMsg	tx;
	uint32_t	mailbox;
	if(_buffer_pull(_CAN->tx,&tx,sizeof(CanTxMsg)))
		HAL_CAN_AddTxMessage(&hcan2, &tx.hdr, (uint8_t *)&tx.buf, &mailbox);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* hcan) {
	HAL_CAN_TxMailbox2CompleteCallback(hcan); 
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* hcan) {
	HAL_CAN_TxMailbox2CompleteCallback(hcan); 
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	*canTx(void *v) {
// first entry...
	if(!_CAN) {
		_CAN	=	_io_init(100*sizeof(CanTxMsg), 100*sizeof(CanTxMsg));

		canFilterCfg(_ACK_LEFT_FRONT,	0x780, _ID_IAP_GO, 0x7f0);
		idDev=(uint32_t)HAL_CRC_Calculate(&hcrc,(uint32_t *)0x1FFF7A10,3);
		
		for(tim *t=timStack; t->htim; ++t) {
			t->dma=_buffer_init(128*sizeof(uint32_t));
			if(t->tmode == _DMA)
				HAL_TIM_IC_Start_DMA(t->htim,t->Channel, (uint32_t *)t->dma->_buf, t->dma->size/sizeof(uint32_t));
			else
				HAL_TIM_IC_Start_IT(t->htim,t->Channel);
			t->htim->State=HAL_TIM_STATE_READY;			//!!! https://community.st.com/s/question/0D50X00009XkWIcSAN/multiple-channels-input-capture-at-the-same-time
		}
		
		HAL_CAN_ActivateNotification(&hcan2,CAN_IT_RX_FIFO0_MSG_PENDING);
		HAL_CAN_ActivateNotification(&hcan2,CAN_IT_TX_MAILBOX_EMPTY);
		HAL_CAN_Start(&hcan2);
// periodic polling req....
	} else {
		tim 	*t,*first;
		for(t=timStack; t->htim; ++t) {
			uint32_t	tcapt,dt;
			if(t->htim->Instance && t->htim->hdma[((t->Channel)>>2)+1])
				t->dma->_push = &t->dma->_buf[(t->dma->size - t->htim->hdma[((t->Channel)>>2)+1]->Instance->NDTR*sizeof(uint32_t))];
			while(_buffer_pull(t->dma,&tcapt,sizeof(uint32_t))) 
				if((1 << t->ch) & ~testMask) {
					if(t->timeout) {
						if(tcapt < t->to)
							dt = (0x10000 + tcapt - t->to)/uS;
						else
							dt = (tcapt - t->to)/uS;
						t->to=tcapt;
					
						if(dt>MIN_BURST) {
							if(dt > 300)
								_DEBUG(DBG_TIMING,"%c",'-');
							else
								_DEBUG(DBG_TIMING,"%c",'_');
						}
						
						if(t->cnt % 2)														// __--
							_DEBUG(DBG_USEC,"\r\n%d,%d:%3d",t->ch,t->sect,dt);
						else 
							_DEBUG(DBG_USEC,",%3d",dt);							//--__
		
						if(dt>MIN_BURST) {
							__HAL_CRC_DR_RESET(&hcrc);
							hcrc.Instance->DR = t->crc;
							if(dt > CRC_THRHOLD)
								hcrc.Instance->DR=1;
							else
								hcrc.Instance->DR=0;
							t->crc = hcrc.Instance->DR;
						}
						
						if(t->cnt % 2) {													// __---
							t->hi +=dt;
							t->shi+=dt*dt;
							t->pw +=dt;
						} else {																	// --___
							t->lo += dt;
							t->slo+=dt*dt;
							if(dt > MAX_BURST && t->pw > MIN_BURST) {
								++t->longcnt;
								t->pw=0;
							}
						}
					}	else {
//----------------------------------------------------------------			
int32_t			n=eval(refCnt)-tcapt;
						t->trefcnt=refCnt;																		// referencni count
						if(t->ch==0 || t->ch==3)															// prioriteta kanalov
							n-=10;
						if(t->ch==2 || t->ch==5)
							n-=20;
						if(n < 0)
							t->trefcnt = --t->trefcnt % 0x10000;
						t->tref = n % 0x10000;
						_DEBUG(10,"\r\n%d,%d,%3d,%5d",t->ch, t->sect,t->trefcnt % 1000,t->tref);
//----------------------------------------------------------------						
						t->to=tcapt;
						t->cnt=t->longcnt=t->pw=t->crc = 0;
						t->hi=t->lo=t->shi=t->slo=0;
					}
					++t->cnt;
					t->timeout=HAL_GetTick()+MAX__INT;
					if(!flushTout) {
						flushTout=HAL_GetTick()+FLUSH__INT;
					}
				}

			if(t->timeout && HAL_GetTick() >= t->timeout) {
				t->timeout=0;
				_DEBUG(DBG_CRC,"\r\n%d,%d:<%08X>",t->ch,t->sect,t->crc);
				switch(t->crc) {
					case _LEFT_FRONT:
						idPos=0;
						t->cnt=t->longcnt=0;
						Send(_ACK_LEFT_FRONT+idPos,NULL,0);
						SaveSettings();
					break;
						
					case _RIGHT_FRONT:
						idPos=1;
						t->cnt=t->longcnt=0;
						Send(_ACK_LEFT_FRONT+idPos,NULL,0);
						SaveSettings();
					break;
						
					case _RIGHT_REAR:
						idPos=2;
						t->cnt=t->longcnt=0;
						Send(_ACK_LEFT_FRONT+idPos,NULL,0);
						SaveSettings();
					break;
						
					case _LEFT_REAR:
						idPos=3;
						t->cnt=t->longcnt=0;
						Send(_ACK_LEFT_FRONT+idPos,NULL,0);
						SaveSettings();
					break;
			
					case _REPEAT:
						t->cnt=t->longcnt=0;
					break;
					
					default: 												// signature
					break;
				}				
				uint32_t n = t->cnt/2;
				if(n > 1)
					_DEBUG(DBG_STAT,"\r\n%d,%d:%5d,%5d,%5d,%5d --- %d,%d",t->ch,t->sect,t->hi/n,t->lo/(n-1),(int)sqrt(n*t->shi - t->hi*t->hi)/n,(int)sqrt((n-1)*t->slo - t->lo*t->lo)/(n-1),n,t->longcnt);
				else if(n > 0)
					_DEBUG(DBG_STAT,"\r\n%d,%d:%5d,%5d,%5d,%5d --- %d,%d",t->ch,t->sect,	t->hi,0,0,0,n,t->longcnt);
			}
		}
// isci prvega.... prekini, ce je katerikoli timeout se aktiven..		
		for(t=timStack,first=NULL; t->htim; ++t) {
			if(flushTout && HAL_GetTick() > flushTout) {
				t->timeout=0;
			}
			if(t->timeout)
				return canTx;
			if(t->cnt) {
				if(first) {
					if(t->trefcnt < first->trefcnt || (t->trefcnt == first->trefcnt &&  t->tref < first->tref))
						first = t;
				} else
					first=t;
			}
		}
// isci vse v rangu +/-1 od prvega.... brisi payload, ce first ne obstaja, preskok
		if(first) {
			memset(&py,0,sizeof(payload));
			py.pulse.tref=first->tref;
			py.pulse.slot=first->trefcnt;
			for(t=timStack; first && t->htim; ++t) {
// na kanalu je nek count in je blizu first...
				if(t->cnt && t->trefcnt == first->trefcnt && t->tref <= first->tref+1) {
					if(t->longcnt) {
						if(t->longcnt > 15) {
							py.pulse.sect[t->sect].ch=2;
							py.pulse.sect[t->sect].longpulse=true;
							py.pulse.sect[t->sect].count=min(t->longcnt,0xf);
						}
					} else if (t->cnt > 15) {
						py.pulse.sect[t->sect].ch=1;
						py.pulse.sect[t->sect].longpulse=false;
						py.pulse.sect[t->sect].count=min(t->cnt,0xf);
					} else if (t->cnt && t->cnt <= 2) {
						py.pulse.sect[t->sect].ch=0;
						py.pulse.sect[t->sect].longpulse=false;
						py.pulse.sect[t->sect].count=min(t->cnt,0xf);
					}
////////////////////////////
					{
						uint32_t n = t->cnt/2;
						if(n > 1)
							_DEBUG(21,"\r\n%d,%d:%5d,%5d,%5d,%5d --- %d,%d",t->ch,t->sect,t->hi/n,t->lo/(n-1),(int)sqrt(n*t->shi - t->hi*t->hi)/n,(int)sqrt((n-1)*t->slo - t->lo*t->lo)/(n-1),n,t->longcnt);
						else if(n > 0)
							_DEBUG(21,"\r\n%d,%d:%5d,%5d,%5d,%5d --- %d,%d",t->ch,t->sect,	t->hi,0,0,0,n,t->longcnt);
					}
					_DEBUG(20,"\r\n%d,%d:<%08X>",t->ch,t->sect,t->crc);
////////////////////////////
				}
				t->cnt=t->longcnt=0;
			}
			if(py.pulse.sect[0].count || py.pulse.sect[1].count || py.pulse.sect[2].count) {
				Send(_ACK_LEFT_FRONT+idPos,&py,sizeof(payload));
			}
		}
		flushTout=0;
	}
	return canTx;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	*canRx(void *v) {
	if(_CAN) {
	CanRxMsg	rx;
	payload		p;
	

		if(canConsole) {
			int n=_buffer_pull(canConsole->tx,p.bytes,8);
			if(n)
				Send(idCOM2CAN,&p,n);
		}		
		
		if(_buffer_pull(_CAN->rx,&rx,sizeof(CanRxMsg))) {
			_RED(20);
			_DEBUG(DBG_CAN_RX,"\r%5d: < %03X",HAL_GetTick() % 10000,rx.hdr.StdId);
			for(int i=0; i<rx.hdr.DLC; ++i)
				_DEBUG(DBG_CAN_RX," %02X",rx.buf.bytes[i]);
			_DEBUG(DBG_CAN_RX,"%s","\r\n");

			switch(rx.hdr.StdId) {
				
				case _ID_IAP_PING ... _ID_IAP_PING+_MAX_DEV-1:
					if(rx.hdr.DLC) {
						if(nDev)
							_DEBUG(DBG_CONSOLE,"     ser %08X, hash <%08X>, %s",rx.buf.word[1],rx.buf.word[0], strPos[min(4,rx.hdr.StdId-_ID_IAP_PING)]);
						else
							_DEBUG(DBG_CONSOLE,"  ser %08X, hash <%08X>, %s",rx.buf.word[1],rx.buf.word[0], strPos[min(4,rx.hdr.StdId-_ID_IAP_PING)]);
						_DEBUG(DBG_CONSOLE,"%s","\r\n");
						devices[nDev++]=rx.buf.word[1];
					} else {			
						p.word[0]=idCrc;
						p.word[1]=idDev;
						Send(_ID_IAP_PING+idPos,&p,sizeof(payload));
					}
				break;

				case _ID_IAP_REQ:
					if(rx.hdr.DLC)
						if(rx.buf.word[0]!=idDev)
							break;
					while(1);

				case _ID_IAP_ACK:
					++nDev;
					_DEBUG(DBG_CONSOLE,"\r\n  ser %08X, boot",rx.buf.word[1]);
					break;

				case _ID_SYNC_REQ:
					sync(rx.buf.hword[3]);
					refCnt=0;
				break;
						
				case _ID_SYNC_ACK:
					_DEBUG(DBG_SYNC,"%.*s",rx.hdr.DLC,rx.buf.bytes);
					break;
				
				case _TEST_REQ:
				{
					register int i __asm("r3");
					i=rx.buf.hword[0];
					while(i--)
						TEST_GPIO_Port->BSRR = TEST_Pin;
					TEST_GPIO_Port->BSRR = (TEST_Pin<<16);
				}
				break;

				case	_REMOTE_REQ:
					if(rx.hdr.DLC==0 && canConsole) {
						_proc_find(console,&canConsole)->f=NULL;
						canConsole=_io_close(canConsole);
					}
					else if(rx.buf.word[0]==idDev && !canConsole) {
						canConsole=_io_init(128,128);
						_proc_add(console,&canConsole,"can console",0);				
					}
				break;

				case idCAN2COM:
					if(canConsole)
						while(rx.hdr.DLC && !_buffer_push(canConsole->rx,rx.buf.bytes,rx.hdr.DLC))
							_wait(2);
				break;

				case idCOM2CAN:
					_DEBUG(DBG_CONSOLE,"%.*s",rx.hdr.DLC,rx.buf.bytes);
				break;

				case _TEST_LEFT_FRONT ... _TEST_LEFT_FRONT+_MAX_DEV-1:
					if(rx.hdr.StdId - _TEST_LEFT_FRONT == idPos) {
uint16_t 		ch=rx.buf.byte[0],
						sect=rx.buf.byte[1],
						n=rx.buf.hword[1],
						pw=rx.buf.hword[2]*uS,
						per=rx.buf.hword[3]*uS,
						to=HAL_GetTick() & 0xffff;
						for(tim *t=timStack; t->htim; ++t)
							if(t->sect == sect && (ch & (1<<(t->ch)))) {
								while(n--) {
									_buffer_put(t->dma,&to,sizeof(uint32_t)); 
									to-=pw;
									_buffer_put(t->dma,&to,sizeof(uint32_t));
									to-=per;
								}
								break;
							}
					}
				break;
			
				case _ACK_LEFT_FRONT ... _ACK_LEFT_FRONT+_MAX_DEV-1:
					if(idPos > 3) {
						if(rx.hdr.DLC)
							Decode(rx.hdr.StdId-_ACK_LEFT_FRONT,&rx.buf);		
						else
							Decode(rx.hdr.StdId-_ACK_LEFT_FRONT,NULL);
					} else
						flushTout = HAL_GetTick();
					break;	
					
				default:
					break;
			}
		}		
	}
	return canRx;
}
/*
____-_-_-_-_-_______-_-________-_-_-_-_-_______________________-_-_-_-_-_______-_-________-_-_-_-_-___________________
____|_|_|_|_|_______|_|________|_|_|_|_|_______________________|_|_|_|_|_______|_|________|_|_|_|_|___________________
*/
