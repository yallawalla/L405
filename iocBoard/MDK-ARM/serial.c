#include	"stm32f4xx_hal.h"
#include	"io.h"
#include	"proc.h"
//__________________________________________________________________________________
_io			*_UART;
uint8_t	rx,tx[64];
//__________________________________________________________________________________
extern 	UART_HandleTypeDef huart1;
void		*console(void *);
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void		*uart(void *v) {
_io 		*io=*(_io **)v;
				if (huart1.gState == HAL_UART_STATE_READY)
					HAL_UART_Transmit_IT(&huart1, tx, _buffer_pull(io->tx,tx,sizeof(tx)));	
				return v;
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
				_buffer_push(_UART->rx,&rx,1);	
				HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx, 1);
}
/*******************************************************************************
* Function Name	: 
* Description		: 
* Output				:
* Return				:
*******************************************************************************/
_io			*InitUART(void) {
				_UART=_io_init(128,128);
				_proc_add(console,&_UART,"console",0);
				_proc_add(uart,&_UART,"uart",2);
				HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx, 1);
				return _UART;
}


