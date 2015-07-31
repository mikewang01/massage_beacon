/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: uart.c
 *
 * Description: Two UART mode configration and interrupt handler.
 *              Check your hardware connection while use this mode.
 *
 * Modification history:
 *     2014/3/12, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "assert.h"
#include "user_interface.h"
#include "task_signal.h"

// UartDev is defined and initialized in rom code.
extern UartDevice UartDev;

/*********************************************************************
* MACROS
*/
#define UART0   0
#define UART1   1

/*********************************************************************
* TYPEDEFS
*/

struct uart_private_data{
	uint16 task_id_reg;
	void (*callback_function)(void *arg);
};



	
/*********************************************************************
* GLOBAL VARIABLES
*/
	
	
/*********************************************************************
* LOCAL VARIABLES
*/

LOCAL void uart0_rx_intr_handler(void *para);
LOCAL CLASS(uart) *uart_object = NULL;


LOCAL bool delete_uart(CLASS(uart) *arg);
LOCAL bool uart_taskid_register(CLASS(uart) *arg, uint16 task_id);
LOCAL bool uart_callbackfunction_register(CLASS(uart) *arg, void (*callback)(void *pbuffer));
LOCAL bool uart_send(CLASS(uart) *arg, char *pbuffer, size_t size);

/******************************************************************************
 * FunctionName : uart_config
 * Description  : Internal used function
 *                UART0 used for data TX/RX, RX buffer size is 0x100, interrupt enabled
 *                UART1 just used for debug output
 * Parameters   : uart_no, use UART0 or UART1 defined ahead
 * Returns      : NONE
*******************************************************************************/


LOCAL void ICACHE_FLASH_ATTR
uart_config(uint8 uart_no)
{
    if (uart_no == UART1) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    } else {
        /* rcv_buff size if 0x100 */
        ETS_UART_INTR_ATTACH(uart0_rx_intr_handler,  &(UartDev.rcv_buff));
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    }

    uart_div_modify(uart_no, UART_CLK_FREQ / (UartDev.baut_rate));

    WRITE_PERI_REG(UART_CONF0(uart_no),    UartDev.exist_parity
                   | UartDev.parity
                   | (UartDev.stop_bits << UART_STOP_BIT_NUM_S)
                   | (UartDev.data_bits << UART_BIT_NUM_S));


    //clear rx and tx fifo,not ready
    SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
	if (uart_no == UART1){
		
	 	  //set rx fifo trigger
		  WRITE_PERI_REG(UART_CONF1(uart_no), (UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S);
	 	  //clear all interrupt
		  WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
		  //enable rx_interrupt
		 // SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_OVF_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
	}else{
		//set rx fifo trigger and  time out parameter
	    WRITE_PERI_REG(UART_CONF1(uart_no), ((100&UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) | ((UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S)|UART_RX_TOUT_EN);

	    //clear all interrupt
	    WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
	    //enable rx_interrupt
	    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_OVF_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
	}
}

/******************************************************************************
 * FunctionName : uart1_tx_one_char
 * Description  : Internal used function
 *                Use uart1 interface to transfer one char
 * Parameters   : uint8 TxChar - character to tx
 * Returns      : OK
*******************************************************************************/
LOCAL STATUS ICACHE_FLASH_ATTR
uart1_tx_one_char(uint8 TxChar)
{
    while (true)
	{
		uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(UART1)) & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S);
		if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
			break;
		}
	}

	WRITE_PERI_REG(UART_FIFO(UART1) , TxChar);
	return OK;
}

/******************************************************************************
 * FunctionName : uart1_write_char
 * Description  : Internal used function
 *                Do some special deal while tx char is '\r' or '\n'
 * Parameters   : char c - character to tx
 * Returns      : NONE
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
uart1_write_char(char c)
{
    if (c == '\n') {
        uart1_tx_one_char('\r');
        uart1_tx_one_char('\n');
    } else if (c == '\r') {
    } else {
        uart1_tx_one_char(c);
    }
}
/******************************************************************************
 * FunctionName : uart_rx_intr_disable
 * Description  : Internal used function
 *                Do some special deal while tx char is '\r' or '\n'
 * Parameters   : char c - character to tx
 * Returns      : NONE
*******************************************************************************/

void uart_rx_intr_disable(uint8 uart_no)
{
#if 1
    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
#else
    ETS_UART_INTR_DISABLE();
#endif
}

void uart_rx_intr_enable(uint8 uart_no)
{
#if 1
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
#else
    ETS_UART_INTR_ENABLE();
#endif
}

/******************************************************************************
 * FunctionName : uart0_rx_intr_handler
 * Description  : Internal used function
 *                UART0 interrupt handler, add self handle code inside
 * Parameters   : void *para - point to ETS_UART_INTR_ATTACH's arg
 * Returns      : NONE
*******************************************************************************/
void uart0_tx_buffer(uint8 *buf, uint16 len);
int massage_receive_one_char_callback(uint8 rev_char, RcvMsgBuff *para);
extern int flasg_test;

LOCAL void
uart0_rx_intr_handler(void *para)
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
     * uart1 and uart0 respectively
     */
    RcvMsgBuff *pRxBuff = (RcvMsgBuff *)para;
    uint8 RcvChar;
	uint16 lenth = 0;

#if 1	
	/*ATTENTION:*/
	/*IN NON-OS VERSION SDK, DO NOT USE "ICACHE_FLASH_ATTR" FUNCTIONS IN THE WHOLE HANDLER PROCESS*/
	/*ALL THE FUNCTIONS CALLED IN INTERRUPT HANDLER MUST BE DECLARED IN RAM */
	/*IF NOT , POST AN EVENT AND PROCESS IN SYSTEM TASK */
    if(UART_FRM_ERR_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_FRM_ERR_INT_ST)){
        CLING_DEBUG("FRM_ERR\r\n");
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_FRM_ERR_INT_CLR);
    }else if(UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)){
        CLING_DEBUG("f ");
		uart_rx_intr_disable(UART0);
		size_t fifo_len = (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
	 	for (lenth = 0; lenth < fifo_len; lenth++){
			massage_receive_one_char_callback(READ_PERI_REG(UART_FIFO(UART0)) & 0xFF, pRxBuff);
		}
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
		
    }else if(UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)){
        CLING_DEBUG("t ");
        uart_rx_intr_disable(UART0);
		/*get data from buffer and peocess one by one byte*/
		size_t fifo_len = (READ_PERI_REG(UART_STATUS(UART0))>>UART_RXFIFO_CNT_S)&UART_RXFIFO_CNT;
		for (lenth = 0; lenth < fifo_len; lenth++){
			massage_receive_one_char_callback(READ_PERI_REG(UART_FIFO(UART0)) & 0xFF, pRxBuff);
		}
		    if(flasg_test)
			CLING_DEBUG("recieved data during sending\n");
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
		
    }else if(UART_TXFIFO_EMPTY_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_TXFIFO_EMPTY_INT_ST)){
        CLING_DEBUG("e ");
	/* to output uart data from uart buffer directly in empty interrupt handler*/
	/*instead of processing in system event, in order not to wait for current task/function to quit */
	/*ATTENTION:*/
	/*IN NON-OS VERSION SDK, DO NOT USE "ICACHE_FLASH_ATTR" FUNCTIONS IN THE WHOLE HANDLER PROCESS*/
	/*ALL THE FUNCTIONS CALLED IN INTERRUPT HANDLER MUST BE DECLARED IN RAM */
	CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
	
#if 0
		tx_start_uart_buffer(UART0);
#endif
        //system_os_post(uart_recvTaskPrio, 1, 0);
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_TXFIFO_EMPTY_INT_CLR);
        
    }else if(UART_RXFIFO_OVF_INT_ST  == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_OVF_INT_ST)){
        WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_OVF_INT_CLR);
        CLING_DEBUG("RX OVF!!\r\n");
    }
	uart_rx_intr_enable(UART0);
#endif

#if 0

/*================================*/
    if (UART_RXFIFO_FULL_INT_ST != (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
        return;
    }

    WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);

    while (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) {
        RcvChar = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		receive_one_char_callback(RcvChar , pRxBuff);
#if 0
		/*fo debugging purpose*/
		if (pRxBuff->BuffState == WRITE_OVER){
			//uart0_tx_buffer("hello", 5);
			uart0_tx_buffer(pRxBuff->pRcvMsgBuff,(pRxBuff->pWritePos - pRxBuff->pRcvMsgBuff +1));
		}
#endif
        if (pRxBuff->pWritePos == (pRxBuff->pRcvMsgBuff + RX_BUFF_SIZE)) {
            // overflow ...we may need more error handle here.
            pRxBuff->pWritePos = pRxBuff->pRcvMsgBuff ;
        }
    }
#endif
}


/******************************************************************************
 * FunctionName : uart0_tx_buffer
 * Description  : use uart0 to transfer buffer
 * Parameters   : uint8 *buf - point to send buffer
 *                uint16 len - buffer len
 * Returns      :
*******************************************************************************/
void ICACHE_FLASH_ATTR
uart0_tx_buffer(uint8 *buf, uint16 len)
{
    uint16 i;
    for (i = 0; i < len; i++) {
        uart_tx_one_char(buf[i]);
    }
}

/******************************************************************************
 * FunctionName : uart_init
 * Description  : user interface for init uart
 * Parameters   : UartBautRate uart0_br - uart0 bautrate
 *                UartBautRate uart1_br - uart1 bautrate
 * Returns      : NONE
*******************************************************************************/
void ICACHE_FLASH_ATTR
uart_init(UartBautRate uart0_br, UartBautRate uart1_br)
{
	static bool if_configured = false;
	/*make sure this only be initilized once*/
	if(if_configured == false) {
	    // rom use 74880 baut_rate, here reinitialize
	    UartDev.baut_rate = uart0_br;
	    uart_config(UART0);
	    UartDev.baut_rate = uart1_br;
	    uart_config(UART1);
	   
	    // install uart1 putc callback
	    os_install_putc1((void *)uart1_write_char);

		if_configured = true;
	}
}

/******************************************************************************
 * FunctionName : init_uart
 * Description  : internal used to initiate uart device
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/


#if 1
bool ICACHE_FLASH_ATTR
init_uart(CLASS(uart) *arg)
{

	/*check object parameter*/
	assert(NULL != arg);
	/*this module can only be ccreated once ,so if uart_object is not NULL ,the free object and return false*/
	if (uart_object != NULL){
		return FALSE;
	}
	/*assign object pointer to local pointer*/
	uart_object = arg;
	
	/*malloc corresponed dparameter buffer*/
	struct uart_private_data *callback_data = (struct uart_private_data*)(os_malloc(sizeof(struct uart_private_data)));
	assert(NULL != callback_data);
	/*initialize uart device*/
	
	callback_data->task_id_reg = USER_TASK_PRIO_MAX + 1;
	callback_data->callback_function =NULL;
	/*assign callback private data to pointer in uart device object*/
	arg->user_data = callback_data;

	/*assign corresponded implementation*/
	arg->init = init_uart;
	arg->de_init = delete_uart;

	arg->task_register = uart_taskid_register;

	arg->recv_callback_register = uart_callbackfunction_register;
	arg->send = uart_send;

	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	return TRUE;

}
#endif

/******************************************************************************
 * FunctionName : delete_uart
 * Description  : internal used to delete uart device
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
delete_uart(CLASS(uart) *arg)
{
	/*check object parameter*/
	assert(NULL != arg);

	/*disable UART0 rx_interrupt so that it is much safer to release  memory*/
    CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_RXFIFO_FULL_INT_ENA);
	/*malloc corresponed dparameter buffer*/
	struct uart_private_data *callback_data = (struct uart_private_data*)(arg->user_data);

	os_free(callback_data);
	os_free(arg);
	/*clear local object pointer in case of memory error*/
	uart_object = NULL;
	return TRUE;


}
#endif
/******************************************************************************
 * FunctionName : uart_taskid_register
 * Description  : internal used to register specific taskdi passed over here
 * Parameters   : arg -- object pointer
 *				  task_id -- task-id who gonna recieve message 
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
uart_taskid_register(CLASS(uart) *arg, uint16 task_id)
{
	/*check object parameter*/
	assert(NULL != arg);


	/*malloc corresponed dparameter buffer*/
	struct uart_private_data *callback_data = (struct uart_private_data*)(arg->user_data);


	assert(NULL != callback_data);

	/*if taskid passed here is valid*/
	if (IS_TASK_VALID(task_id)){
		/*register taskid here*/
		callback_data->task_id_reg = task_id;
	}
	
	return TRUE;


}
#endif
/******************************************************************************
 * FunctionName : uart_taskid_register
 * Description  : internal used to register specific taskdi passed over here
 * Parameters   : arg -- object pointer
 *				  task_id -- task-id who gonna recieve message 
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
uart_callbackfunction_register(CLASS(uart) *arg, void (*callback)(void *pbuffer))
{
	/*check object parameter*/
	assert(NULL != arg);
	/*malloc corresponed dparameter buffer*/
	struct uart_private_data *callback_data = (struct uart_private_data*)(arg->user_data);
	assert(NULL != callback_data);

	/*if taskid passed here is valid*/
	if (NULL == callback_data->callback_function){
		/*register taskid here*/
		 callback_data->callback_function = callback;
	}else{
		return FALSE;
	}
	
	return TRUE;


}
#endif

/******************************************************************************
 * FunctionName : uart_taskid_register
 * Description  : internal used to register specific taskdi passed over here
 * Parameters   : arg -- object pointer
 *				  task_id -- task-id who gonna recieve message 
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
uart_send(CLASS(uart) *arg, char *pbuffer, size_t size)
{
	/*check object parameter*/
	assert(NULL != arg);
	assert(NULL != pbuffer);
	uart0_tx_buffer(pbuffer, size);

	return TRUE;


}
#endif



