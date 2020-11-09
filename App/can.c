#include	"can.h"
#include	"console.h"
#include	"ws.h"
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				: 
* Return				:
*******************************************************************************/
_io				*_CAN,*_DBG;
char			*strPos[]={"front left","front right","rear right","rear left"};
uint32_t	idDev=0, 
					idCrc=0,
					debug,
					idPos=_ACK_LEFT_FRONT,
					ackCount=0, 
					ackMax=_MAXDEV, 
					tref=0;
payload		py={0,0};

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

//tim timStack[] = {
//	{NULL,&htim1,TIM_CHANNEL_1,0,5,_DMA},					//PA8
//	{NULL,&htim1,TIM_CHANNEL_2,1,5,_DMA},					//PA9 ---
//	{NULL,&htim1,TIM_CHANNEL_3,2,5,_DMA},					//PA10
//	{NULL,&htim3,TIM_CHANNEL_2,0,4,_DMA},					//PA7
//	{NULL,&htim4,TIM_CHANNEL_2,1,4,_DMA},					//PB7
//	{NULL,&htim5,TIM_CHANNEL_1,2,4,_DMA},					//PA0
//	{NULL,&htim5,TIM_CHANNEL_2,0,0,_DMA},					//PA1 A1 1
//	{NULL,&htim5,TIM_CHANNEL_3,1,0,_DMA},					//PA2 A1 2
//	{NULL,&htim5,TIM_CHANNEL_4,2,0,_DMA},					//PA3 A1 3
//	{NULL,&htim8,TIM_CHANNEL_2,0,3,_DMA},					//PC7
//	{NULL,&htim8,TIM_CHANNEL_3,1,3,_DMA},					//PC8 -
//	{NULL,&htim8,TIM_CHANNEL_4,2,3,_DMA},					//PC9 -

//	{NULL,&htim3,TIM_CHANNEL_1,0,2,_IT},					//PA6
//	{NULL,&htim3,TIM_CHANNEL_3,1,2,_IT},					//PB0 -
//	{NULL,&htim3,TIM_CHANNEL_4,2,2,_IT},					//PB1 - 
//	
//	{NULL,&htim4,TIM_CHANNEL_1,0,1,_IT},					//PB6 A2 1
//	{NULL,&htim4,TIM_CHANNEL_3,1,1,_IT},					//PB8 A2 2
//	{NULL,&htim4,TIM_CHANNEL_4,2,1,_IT},					//PB9 A2 3
//	{NULL,NULL,0,0,0,_IT}
//};

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
		_io	*io=_stdio(_DBG);
		_print("\r%3d: > %03X",HAL_GetTick() % 1000,tx.hdr.StdId);
		for(int i=0; i<tx.hdr.DLC; ++i)
			_print(" %02X",tx.buf.bytes[i]);
		_print("\r\n");
		_stdio(io);
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
		canFilterCfg(_ACK_LEFT_FRONT,	0x7c0, _ID_IAP_GO, 0x7f0);
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
		tim 	*t;
		_io		*in=stdin->io;
		_io 	*out=stdout->io;
		_io 	*current=*(_io **)v;
		_stdio(current);
		for(t=timStack; t->htim; ++t) {
			uint32_t	tcapt,dt;
			if(t->tmode == _DMA)
				t->dma->_push = &t->dma->_buf[(t->dma->size - t->htim->hdma[((t->Channel)>>2)+1]->Instance->NDTR*sizeof(uint32_t))];
			while(_buffer_pull(t->dma,&tcapt,sizeof(uint32_t))) {
				if(t->timeout) {
					if(tcapt < t->to)
						dt = 0x10000 + tcapt - t->to;
					else
						dt = tcapt - t->to;
					t->to=tcapt;
					
					if(debug & (1<<DBG_TIMING)) {
						_io	*io=_stdio(_DBG);				
						if(dt/84>50) {
							if(dt/84 > 300)
							_print("-");
						else
							_print("_");
						}
						_stdio(io);
					}
					if(debug & (1<<DBG_USEC)) {
						_io	*io=_stdio(_DBG);		
						if(t->count % 2 == 0)
							_print(",%3d\r\n",dt/84);
						else
							_print("%d: %3d",t->sect,dt/84);
						_stdio(io);
					}
					

					if(dt/84>50) {
						__HAL_CRC_DR_RESET(&hcrc);
						hcrc.Instance->DR = t->crc;
						if(dt/84 > 300)
							hcrc.Instance->DR=0;
						else
							hcrc.Instance->DR=1;
						t->crc = hcrc.Instance->DR;
					}
					
					
					if(t->count % 2) {			// __---
						if(t->count == 1)
							t->sum=dt;
						else
							t->sum+=dt;
						if(t->sum/84 > 50) {
							++t->scount;
							t->sum=0;
						}
					} else {								// --___
						if(dt/84 > 100)
							t->sum=0;
					}
				}	else {
					t->to = tcapt;
				}
				++t->count;
				t->timeout=HAL_GetTick()+20;
			}
			if(t->timeout && HAL_GetTick() > t->timeout) {
				t->timeout=0;
				if(debug & (1<<DBG_CRC)) {
					_io	*io=_stdio(_DBG);
					_print(" %08X\r\n>",t->crc);	
					_stdio(io);
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
							
					case _IAP_REQ:
						while(1);
							
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
						
					default:
						if(t->ch == 0) {
							if(t->count > 14)
								py.byte[t->sect] |= (1 << 1);
							else
								py.byte[t->sect] |= (1 << 0);
						}
			
						if(t->ch == 1 &&  t->scount > 14)
							py.byte[t->sect] |= (1 << 2);
						
						
						if(t->ch == 3) {
							if(t->count > 14)
								py.byte[t->sect] |= (1 << 4);
							else
								py.byte[t->sect] |= (1 << 3);
						}
			
						if(t->ch == 4 &&  t->scount > 14)
							py.byte[t->sect] |= (1 << 5);
				}						
				t->count=0;
				t->scount=0;
				t->sum=0;
				t->crc = 0;
			}
		}
		for(t=timStack; t->htim; ++t)
			if(t->timeout)
				break;
		if(t->htim && py.word[0]) {
			for(int k=0; k<3; ++k)
				if((py.byte[k] & 6)==6)
					py.byte[k]=(py.byte[k] & ~6) | 2;
								
			Send(idPos,&py,3*sizeof(uint8_t));
			memset(&py,0,sizeof(payload));	
		}
		stdin->io=in;
		stdout->io=out;
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
_io		*in=stdin->io;
_io 	*out=stdout->io;
_io 	*current=*(_io **)v;
			_stdio(current);

		CanRxMsg	rx;
payload		p;
		
		if(_buffer_pull(_CAN->rx,&rx,sizeof(CanRxMsg))) {
			_BLUE(500);
			if(debug & (1<<DBG_CAN_RX)) {
				_print("\r%3d: < %03X",HAL_GetTick() % 1000,rx.hdr.StdId);
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
						++ackCount;
						_print("  ser %08X, boot",rx.buf.word[1]);
						DecodeCom(NULL);
					}
					break;
					
				case _THR_LEFT_FRONT:
				case _THR_RIGHT_FRONT:
				case _THR_RIGHT_REAR:
				case _THR_LEFT_REAR:
					if(rx.hdr.StdId - 0x010 == idPos) {
uint32_t 		ch=rx.buf.byte[0],
						sect=rx.buf.byte[1],
						n=rx.buf.hword[1],
						pw=rx.buf.hword[2]*84,
						per=rx.buf.hword[3]*84,
						to=HAL_GetTick();
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

				case _TEST_LEFT_FRONT:
				case _TEST_RIGHT_FRONT:
				case _TEST_RIGHT_REAR:
				case _TEST_LEFT_REAR:
					if(rx.hdr.StdId - 0x020 == idPos) {
						test *p=(test *)&rx.buf;
						for(int i=0; i<rx.hdr.DLC; ++i) {
							for(int j=0; j<6; ++j) {
								if(p->mask & (1<<j))
									for(tim *t=timStack; t->htim; ++t) {
										if(t->sect == p->sect && t->ch == j)
											_buffer_put(t->dma, (uint32_t *)t, sizeof(uint32_t));
									}
							}
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
							++ackCount;
							_print("  v <%08X>, ser %08X, %s",rx.buf.word[0],rx.buf.word[1], strPos[rx.hdr.StdId-_ACK_LEFT_FRONT]);
							DecodeCom(NULL);
						break;
						default:
							break;
					}					
				default:
					break;
			}
		}
		stdin->io=in;
		stdout->io=out;
	}
	return v;
}
