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
_io				*_CAN, *canConsole;
//______________________________________________________________________________________
const char *strPos[]={"front right","rear right","rear left","front left","console"};
uint32_t	idDev,
					nDev,
					idCrc,
					idPos,
					testMask;
payload		py;
uint32_t	devices[_MAX_DEV];
uint32_t	flushTout;

uint8_t		prio[6][3]={
{0,0,2},
{0,1,2},
{2,2,2},
{0,0,2},
{0,1,2},
{2,2,2}
};

tim timStack[] = {
	//DMA
	{NULL,&htim1,TIM_CHANNEL_1,0,0,_1A1_GPIO_Port,_1A1_Pin},					//PA8		1A1
	{NULL,&htim1,TIM_CHANNEL_2,1,0,_2A1_GPIO_Port,_2A1_Pin},					//PA9		2A1
	{NULL,&htim1,TIM_CHANNEL_3,2,0,_3A1_GPIO_Port,_3A1_Pin},					//PA10	3A1
	
	{NULL,&htim3,TIM_CHANNEL_2,0,1,_1A2_GPIO_Port,_1A2_Pin},					//PA7		1A2
	{NULL,&htim4,TIM_CHANNEL_2,0,4,_1B2_GPIO_Port,_1B2_Pin},					//PB7		1B2
	
	{NULL,&htim5,TIM_CHANNEL_1,1,1,_2A2_GPIO_Port,_2A2_Pin},					//PA0 	2A2
	{NULL,&htim5,TIM_CHANNEL_2,1,4,_2B2_GPIO_Port,_2B2_Pin},					//PA1 	2B2
	
	{NULL,&htim5,TIM_CHANNEL_3,2,1,_3A2_GPIO_Port,_3A2_Pin},					//PA2 	3A2
	{NULL,&htim5,TIM_CHANNEL_4,2,4,_3B2_GPIO_Port,_3B2_Pin},					//PA3		3B2
	
	{NULL,&htim8,TIM_CHANNEL_2,0,3,_1B1_GPIO_Port,_1B1_Pin},					//PC7		1B1
	{NULL,&htim8,TIM_CHANNEL_3,1,3,_2B1_GPIO_Port,_2B1_Pin},					//PC8		2B1
	{NULL,&htim8,TIM_CHANNEL_4,2,3,_3B1_GPIO_Port,_3B1_Pin},					//PC9		3B1
	// IT
	{NULL,&htim3,TIM_CHANNEL_1,0,2,_1A3_GPIO_Port,_1A3_Pin},					//PA6		1A3
	{NULL,&htim3,TIM_CHANNEL_3,1,2,_2A3_GPIO_Port,_2A3_Pin},					//PB0		2A3
	{NULL,&htim3,TIM_CHANNEL_4,2,2,_3A3_GPIO_Port,_3A3_Pin},					//PB1 	3A3
	
	{NULL,&htim4,TIM_CHANNEL_1,0,5,_1B3_GPIO_Port,_1B3_Pin},					//PB6 	1B3
	{NULL,&htim4,TIM_CHANNEL_3,1,5,_2B3_GPIO_Port,_2B3_Pin},					//PB8 	2B3
	{NULL,&htim4,TIM_CHANNEL_4,2,5,_3B3_GPIO_Port,_3B3_Pin},					//PB9 	3B3

	{0}
};

uint32_t	filter_count;
uint32_t	testReq,testRef=64;
uint32_t	syncTimeout=3000;
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if(htim == &htim12 && testReq && idPos <= _MAX_HEAD-1) {
		_buffer_push(timStack[testRef].dma,(void *)&timStack[testRef].htim->Instance->CNT,sizeof(uint32_t));
		testReq = 0;
	}
	
	if(htim == &htim3  && idPos > _MAX_HEAD-1) {
		if(__HAL_TIM_GET_COUNTER(&htim9) % 128  == 0 && !iapInproc)
			Send(_ID_SYNC_REQ,NULL,0);
		if(__HAL_TIM_GET_COUNTER(&htim9) % 1280 == 0)
			_GREEN(20);

		if((__HAL_TIM_GET_COUNTER(&htim9) % 128 == testRef) && testReq) {
			testReq=0;
			HAL_GPIO_TogglePin(TREF_GPIO_Port, TREF_Pin); 
//			uint32_t to=htim3.Instance->CNT+84*20;
//			while(htim3.Instance->CNT < to);
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
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim) {
	if(htim == &htim12 && testReq && idPos <= _MAX_HEAD-1) {
		_buffer_push(timStack[testRef].dma,(void *)&timStack[testRef].htim->Instance->CNT,sizeof(uint32_t));
	}
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	uint32_t	i=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_1);
	uint32_t	j=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_3);
	uint32_t	k=HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_4);
	
	tim				*p=NULL;
	if(htim->Instance==TIM3)	p=&timStack[12];
	if(htim->Instance==TIM4)	p=&timStack[15];

	if(p)
		switch(htim->Channel) {
			case HAL_TIM_ACTIVE_CHANNEL_1:
				_buffer_push(p[0].dma, &i, sizeof(uint32_t));
			break;
			case HAL_TIM_ACTIVE_CHANNEL_3:
				_buffer_push(p[1].dma, &j, sizeof(uint32_t));
			break;
			case HAL_TIM_ACTIVE_CHANNEL_4:
				_buffer_push(p[2].dma, &k, sizeof(uint32_t));
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
		if(_buffer_push(_CAN->tx,&tx,sizeof(CanTxMsg))!=sizeof(CanTxMsg)) {
			_SETERR(ERR_CAN_TX);
		}
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
			if(t->htim->Instance && t->htim->hdma[((t->Channel)>>2)+1])
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
			int32_t	tcapt,dt;
			if(t->htim->Instance && t->htim->hdma[((t->Channel)>>2)+1])
				t->dma->_push = &t->dma->_buf[(t->dma->size - t->htim->hdma[((t->Channel)>>2)+1]->Instance->NDTR*sizeof(uint32_t))];
			while(_buffer_pull(t->dma,&tcapt,sizeof(uint32_t))) 
				if((1 << t->ch) & ~testMask) {
					if(t->timeout) {
						dt = (((tcapt - t->to) & 0xffff) + uS/2)/uS;
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
//----------------------------------------------------------------			
					}	else {
//----------------------------------------------------------------			
						t->to=tcapt;																								// referenca za dt
						if(tcapt > 0x8000 && t->htim->Instance->CNT < 0x8000)
							tcapt -= 0x10000;
						t->tref=eval(tcapt + (__HAL_TIM_GET_COUNTER(&htim9)<<16));	// izracun glede na sinhronizacijo
						if(t->ch==0 || t->ch==3)																		// tref iz prioritete kanalov
							t->tref+=16;
						if(t->ch==1 || t->ch==4)
							t->tref+=32;
//----------------------------------------------------------------						
						t->cnt=t->longcnt=t->pw=t->crc = 0;
						t->hi=t->lo=t->shi=t->slo=0;
					}
					++t->cnt;
					t->timeout=HAL_GetTick()+MAX__INT;
					if(!flushTout)
						flushTout=HAL_GetTick()+FLUSH__INT;
					_RED(40);
				}

			if(t->timeout && HAL_GetTick() >= t->timeout) {
				t->timeout=0;
				_CLRERR(ERR_PIN+t->sect);
				_DEBUG(DBG_CRC,"\r\n%d,%d:<%08X>",t->ch,t->sect,t->crc);
				switch(t->crc) {
					case _LEFT_FRONT:
						idPos=3;
						t->cnt=t->longcnt=0;
						Send(_ACK_LEFT_FRONT+idPos,NULL,0);
						SaveSettings();
					break;
						
					case _RIGHT_FRONT:
						idPos=0;
						t->cnt=t->longcnt=0;
						Send(_ACK_LEFT_FRONT+idPos,NULL,0);
						SaveSettings();
					break;
						
					case _RIGHT_REAR:
						idPos=1;
						t->cnt=t->longcnt=0;
						Send(_ACK_LEFT_FRONT+idPos,NULL,0);
						SaveSettings();
					break;
						
					case _LEFT_REAR:
						idPos=2;
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
// clear timeout & error, if forced flushing
			if(flushTout && HAL_GetTick() > flushTout) {
				t->timeout=0;												
				_CLRERR(ERR_PIN+t->sect);
			}
			if(t->timeout)
				return canTx;
			if(t->cnt && (!first || t->tref < first->tref))
				first=t;
		}
// isci vse v rangu +/-1 od prvega.... brisi payload, ce first ne obstaja, preskok
		if(first) {
			memset(&py,0,sizeof(payload));
			py.pulse.tref=first->tref;
			for(t=timStack; first && t->htim; ++t) {
// na kanalu je nek count in je blizu first...
				if(t->cnt && t->tref <= first->tref+1) {
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
		if(HAL_GetTick() > syncTimeout && idPos < _MAX_HEAD)
			_SETERR(ERR_SYNC);
		
		if(_buffer_pull(_CAN->rx,&rx,sizeof(CanRxMsg))) {
			_DEBUG(DBG_CAN_RX,"\r%5d: < %03X",HAL_GetTick() % 10000,rx.hdr.StdId);
			for(int i=0; i<rx.hdr.DLC; ++i)
				_DEBUG(DBG_CAN_RX," %02X",rx.buf.bytes[i]);
			_DEBUG(DBG_CAN_RX,"%s","\r\n");

			switch(rx.hdr.StdId) {
				
				case _ID_IAP_PING ... _ID_IAP_PING+_MAX_DEV-1:
					if(rx.hdr.DLC) {
						if(nDev)
							_DEBUG(DBG_CONSOLE,"%s","   ");
						_DEBUG(DBG_CONSOLE,"  ser.%04hX, hash <%08X>, %-12s(%04hX)\r\n",rx.buf.hword[2],rx.buf.word[0], strPos[min(_MAX_HEAD,rx.hdr.StdId-_ID_IAP_PING)],rx.buf.hword[3]);
						devices[nDev++]=rx.buf.word[1];
						Decode(rx.hdr.StdId-_ID_IAP_PING,NULL);
					} else {			
						p.word[0]=idCrc;
						p.hword[2]=idDev & 0xffff;
						p.hword[3]=error & 0xffff;
						Send(_ID_IAP_PING+idPos,&p,sizeof(payload));
					}
				break;

				case _ID_IAP_REQ:
					if(rx.hdr.DLC)
						if(rx.buf.word[0]!=idDev)
							break;
					while(1);

				case _ID_IAP_ACK:
					if(nDev)
						_DEBUG(DBG_CONSOLE,"%s","   ");
					_DEBUG(DBG_CONSOLE,"  ser %08X, boot\r\n",rx.buf.word[1]);
					if(!rx.buf.word[0])
						devices[nDev++]=rx.buf.word[1];
					break;

				case _ID_SYNC_REQ:
					sync(rx.buf.hword[3]);
					syncTimeout=HAL_GetTick()+200;
					__HAL_TIM_SET_COUNTER(&htim9,0);
				break;
						
				case _ID_SYNC_ACK:
					_DEBUG(DBG_SYNC,"%.*s",rx.hdr.DLC,rx.buf.bytes);
					break;

				case _ID_RESET:
					NVIC_SystemReset();	
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
					else if(rx.buf.hword[0]==(idDev & 0xffff) && !canConsole) {
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

