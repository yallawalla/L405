#include	"can.h"
#include	"console.h"
#include	"ws.h"
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				: 
* Return				:
*******************************************************************************/
_io				*_CAN,*_DBG, *canConsole;
//______________________________________________________________________________________
const char *strPos[]={"front left","front right","rear right","rear left","console","console","console","console","console","console","console","console","console","console","console","console"};
uint32_t	idDev,
					nDev,
					idCrc,
					debug,
					idPos=_ACK_LEFT_FRONT,
					testMask=0;
payload		py={0,0};
uint32_t	devices[_MAX_DEV];


#ifdef	__DISCO__
		#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
		#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
		led Leds = {{0,0,0,0},GPIOD,{GPIO_PIN_14,GPIO_PIN_12,GPIO_PIN_15,GPIO_PIN_13}};
#else 
	#ifdef	__NUCLEO__
		#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
		#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
		led Leds = {{0,0,0,0},GPIOB,{GPIO_PIN_14,GPIO_PIN_0,GPIO_PIN_7,GPIO_PIN_7}};
	#else
		#define ledOff(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_SET)
		#define ledOn(a,b)		HAL_GPIO_WritePin(a,b,GPIO_PIN_RESET)
		led Leds = {{0,0,0,0},LED_R_GPIO_Port,{LED_R_Pin,LED_G_Pin,LED_R_Pin,LED_G_Pin}};
	#endif
#endif

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

int		filter_count=0;
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	uint32_t	i;
	tim				*p;
	
	switch((uint32_t)htim->Instance) {
		case (uint32_t)TIM3:
			p=&timStack[12];
		break;
		case (uint32_t)TIM4:
			p=&timStack[15];
		break;
		default:
			return;
	}
			
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
	_YELLOW(500);
	if(debug & (1<<DBG_CAN_TX)) {
		_print("\r%4d: > %03X",HAL_GetTick() % 10000,tx.hdr.StdId);
		for(int i=0; i<tx.hdr.DLC; ++i)
			_print(" %02X",tx.buf.bytes[i]);
		_print("\r\n");
	}
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
uint32_t	flushFilter(uint32_t *ch) {
	#define N_FLUSH 4
	static uint32_t old = 0, cnt = 0, cnteq = 0, tout = 0;
	uint32_t ret = 0;
	if (ch) {
		tout = HAL_GetTick() + 30;
		++cnt;
		if (*ch == old || cnt==1)
			++cnteq;
		else
			cnteq = 0;
		old = *ch;
		if (cnteq && !(cnteq % N_FLUSH))
			ret=cnteq;
	}
	else if (tout && HAL_GetTick() > tout) {
		tout=0;
		if (cnt == cnteq)
			ret = cnteq;
		cnt = cnteq = 0;
		

	}
	return ret;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	*canTx(void *v) {
	for(int i=0;i<4;++i) {
		if(!Leds.t[i])
			continue;
		if(HAL_GetTick() > Leds.t[i]) {
			ledOff(Leds.port, Leds.pin[i]);
			Leds.t[i]=0;
		}
		else
			ledOn(Leds.port, Leds.pin[i]);
	}
/*******************************************************************************/	
	if(!_CAN) {
		_CAN	=	_io_init(100*sizeof(CanRxMsg), 100*sizeof(CanTxMsg));

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
/*******************************************************************************/
	} else {
		_io *io=_stdio(*(_io **)v);
		tim 	*t;
		
		for(t=timStack; t->htim; ++t) {
			uint32_t	tcapt,dt;
			if(t->tmode == _DMA)
				t->dma->_push = &t->dma->_buf[(t->dma->size - t->htim->hdma[((t->Channel)>>2)+1]->Instance->NDTR*sizeof(uint32_t))];
			while(_buffer_pull(t->dma,&tcapt,sizeof(uint32_t))) {
				if(t->timeout) {
					if(tcapt < t->to)
						dt = (0x10000 + tcapt - t->to)/uS;
					else
						dt = (tcapt - t->to)/uS;
					t->to=tcapt;
					if(debug & (1<<DBG_TIMING) 	&& (1 << t->ch) & ~testMask) {		
						if(dt>MIN_BURST) {
							if(dt > 300)
							_print("-");
						else
							_print("_");
						}
					}
					
					if(debug & (1<<DBG_USEC)		&& (1 << t->ch) & ~testMask) {	
						if(t->cnt % 2)															// __--
								_print("\r\n%d,%d:%3d",t->ch,t->sect,dt);
						else {
							if(t->cnt)																//--__
								_print(",%3d",dt);
						}
					}
					
					if(dt>MIN_BURST) {
						__HAL_CRC_DR_RESET(&hcrc);
						hcrc.Instance->DR = t->crc;
						if(dt > CRC_THRHOLD)
							hcrc.Instance->DR=1;
						else
							hcrc.Instance->DR=0;
						t->crc = hcrc.Instance->DR;
					}
					
					if(t->cnt % 2) {			// __---
						if(t->cnt == 1)
							t->pw=dt;
						else
							t->pw +=dt;
						
						t->hi +=dt;
						t->shi+=dt*dt;
						
						if(t->pw > MIN_BURST) {
							++t->longcnt;
						}
					} else {								// --___
							if(dt < MAX_BURST) {
								if(t->pw > MIN_BURST)
									--t->longcnt;
							} else
								t->pw=0;
						t->lo += dt;
						t->slo+=dt*dt;
					}
				}	else {
					t->to = tcapt;
				}
				if((1 << t->ch) & ~testMask) {
					++t->cnt;
					t->timeout=HAL_GetTick()+MAX__INT;
				}
			}

			if(t->timeout && HAL_GetTick() >= t->timeout) {
				t->timeout=0;
				
				if(debug & (1<<DBG_CRC) && (1 << t->ch) & ~testMask) {
					_print("\r\n%d,%d:<%08X>",t->ch,t->sect,t->crc);	
				}
				
				switch(t->crc) {
					case _VCP_CDC:
						if(_VCP) {
							VCP_USB_DEVICE_DeInit();
							MSC_USB_DEVICE_Init();
						} else {
							MSC_USB_DEVICE_DeInit();
							VCP_USB_DEVICE_Init();
						}
					break;
							
					case _LEFT_FRONT:
						idPos=_ACK_LEFT_FRONT;
						Send(idPos,NULL,0);
						SaveSettings();
					break;
						
					case _RIGHT_FRONT:
						idPos=_ACK_RIGHT_FRONT;
						Send(idPos,NULL,0);
						SaveSettings();
					break;
						
					case _RIGHT_REAR:
						idPos=_ACK_RIGHT_REAR;
						Send(idPos,NULL,0);
						SaveSettings();
					break;
						
					case _LEFT_REAR:
						idPos=_ACK_LEFT_REAR;
						Send(idPos,NULL,0);
						SaveSettings();
					break;
			
					case _REPEAT:
					break;
					
					default: // signature
						if(t->longcnt)
							py.byte[t->sect] |=4;
						else if(t->cnt > 4)						// >4 (dva pulta ) za izogib dvojnega pulza pri 
								py.byte[t->sect] |=2;
							else
								py.byte[t->sect] |=1;
					break;
				}				
				t->cnt/=2;
				if(debug & (1<<DBG_STAT)&& (1 << t->ch) & ~testMask) {
					if(t->cnt > 1) {
						_print("\r\n%d,%d:%5d,%5d,%5d,%5d --- %d,%d",t->ch,t->sect,
						t->hi/t->cnt,
						t->lo/(t->cnt-1),
						(int)sqrt(t->cnt*t->shi - t->hi*t->hi)/t->cnt,
						(int)sqrt((t->cnt-1)*t->slo - t->lo*t->lo)/(t->cnt-1),
						t->cnt,t->longcnt);
					} else if(t->cnt > 0) {
						_print("\r\n%d,%d:%5d,%5d,%5d,%5d --- %d,%d",t->ch,t->sect,
						t->hi,0,0,0,t->cnt,t->longcnt);
					}
				}
				t->cnt=t->longcnt=t->pw=t->crc = 0;
				t->hi=t->lo=t->shi=t->slo=0;
			}
		}
			
		for(t=timStack; t->htim; ++t)
			if(t->timeout)
				break;
			
		if(t->htim == NULL) { 
			if(py.word[0]) {
				if(flushFilter(&py.word[0]) > 8) {
					if(py.byte[0]==1)	py.byte[0]=2;
					if(py.byte[1]==1)	py.byte[1]=2;
					if(py.byte[2]==1)	py.byte[2]=2;
					Send(idPos,(payload *)&py.word[0],3*sizeof(uint8_t));
				}
				py.word[1]=py.word[0];																			//backuo za timeout
			} else {
				py.word[0]=py.word[1];
				if(flushFilter(NULL) == 1) {																// osamljen pulz je laser
//					if(py.byte[0])	py.byte[0]=1;
//					if(py.byte[1])	py.byte[1]=1;
//					if(py.byte[2])	py.byte[2]=1;
					Send(idPos,(payload *)&py.word[0],3*sizeof(uint8_t));
				}
			}
			py.word[0]=0;
		}
		_stdio(io);
	}
	return v;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void	*canRx(void *v) {
	if(_CAN) {
	_io		*io=_stdio(*(_io **)v);

	CanRxMsg	rx;
	payload		p;
	

		if(canConsole) {
			int n=_buffer_pull(canConsole->tx,p.bytes,8);
			if(n)
				Send(idCOM2CAN,&p,n);
		}		
		
		if(_buffer_pull(_CAN->rx,&rx,sizeof(CanRxMsg))) {
			_BLUE(500);
			if(debug & (1<<DBG_CAN_RX)) {
				_print("\r%4d: < %03X",HAL_GetTick() % 10000,rx.hdr.StdId);
				for(int i=0; i<rx.hdr.DLC; ++i)
					_print(" %02X",rx.buf.bytes[i]);
				_print("\r\n");
			}
			switch(rx.hdr.StdId) {
				
				case _ID_IAP_PING:
					p.word[0]=idCrc;
					p.word[1]=idDev;
					Send(idPos,&p,sizeof(payload));
				break;

				case _ID_IAP_REQ:
					if(rx.hdr.DLC)
						if(rx.buf.word[0]!=idDev)
							break;
					while(1);

				case _ID_IAP_ACK:
					if(rx.hdr.DLC==sizeof(payload) &&  rx.buf.word[0]==0) {
						++nDev;
						if(debug & (1<<DBG_CONSOLE)) {
							_print("\r\n  ser %08X, boot",rx.buf.word[1]);
							DecodeCom(NULL);
						}
					}
					break;
					
				case _TEST_REQ:
//						HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, GPIO_PIN_SET);
//					HAL_GPIO_WritePin(TEST_GPIO_Port, TEST_Pin, GPIO_PIN_RESET);	
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
					if((debug & (1<<DBG_CONSOLE))) {
						while(rx.hdr.DLC && !_buffer_push(stdout->io->tx,rx.buf.bytes,rx.hdr.DLC))
							_wait(2);
					}
				break;

				case _TEST_LEFT_FRONT:
				case _TEST_RIGHT_FRONT:
				case _TEST_RIGHT_REAR:
				case _TEST_LEFT_REAR:
					if(rx.hdr.StdId - 0x010 == idPos) {
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
			
				case _ACK_LEFT_FRONT:
				case _ACK_RIGHT_FRONT:
				case _ACK_RIGHT_REAR:
				case _ACK_LEFT_REAR:
					switch(rx.hdr.DLC) {
						case 0:																									// quadrant id.
							Decode(rx.hdr.StdId-_ACK_LEFT_FRONT,NULL);						
						break;
						case 3*sizeof(uint8_t):																	// thread
							Decode(rx.hdr.StdId-_ACK_LEFT_FRONT,rx.buf.bytes);		
						break;
						case sizeof(payload):																		// device ack.
							devices[nDev++]=rx.buf.word[1];
							_print("  ser %08X, hash <%08X>, %s",rx.buf.word[1],rx.buf.word[0], strPos[rx.hdr.StdId-_ACK_LEFT_FRONT]);
							DecodeCom(NULL);
						break;
						default:
							break;
					}					
				default:
					break;
			}
		}		
		_stdio(io);
	}
	return v;
}
/*
____-_-_-_-_-_______-_-________-_-_-_-_-_______________________-_-_-_-_-_______-_-________-_-_-_-_-___________________
____|_|_|_|_|_______|_|________|_|_|_|_|_______________________|_|_|_|_|_______|_|________|_|_|_|_|___________________
*/
