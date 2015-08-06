/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_json.c
 *
 * Description: JSON format set up and parse.
 *              Check your hardware transmation while use this data format.
 *
 * Modification history:
 *     2014/5/09, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_json.h"
#include "protocol/massage_mac.h"
#include "protocol/massage_cmd.h"
#include "driver/uart.h"
#include "driver/spi_comm.h"


#define PACKAGE_HEADER_WRTITE_LOWFIVEBYTESREAD_ORDER    0XF0
#define PACKAGE_HEADER_READORDER_HIGH_FIVE_BYTES    0XF1


#define PACKAGE_HEADER_SYMBOL    0X7D
#define PACKAGE_TRANSFER_SYMBOL  0X7F
#define PACKAGE_END_SYMBOL		 0X7E

#define DATA_0X7D_TRANSFERRED	 0X01
#define DATA_0X7E_TRANSFERRED	 0X02
#define DATA_0X7F_TRANSFERRED	 0X03

/*SEMAPHORE RELATED*/
#define SEMAPHORE 	 bool
#define LOCKED		 1
#define UNLOCKED       0
#define IS_SEMAPHORE_LOCKED(__x) (__x == LOCKED)
#define LOCK_SEMAPHORE(__X) 	 (__X =  LOCKED)
#define UNLOCK_SEMAPHORE(__x)	 (__x = UNLOCKED)

#ifdef   MAC_SEND_BUFFER
#undef   MAC_SEND_BUFFER
#endif
#define  MAC_SEND_BUFFER(__X, __Y)  uart0_tx_buffer(__X, __Y)


#define  ENTER_CRITICAL_SECTION() ETS_INTR_LOCK()
#define  LEAVE_CRITICAL_SECTION() ETS_INTR_UNLOCK()

/*THE UPPER LIMIT OF TIMES THE DATA BEEN SENDED*/

#define  RESEND_TIMES_UPPER_THREADHOLD  5
#define  IS_RECIEVED_DATA_VALID(__x)  ((__x[1]^__x[2]^__x[3]^__x[4]^__x[5]) == __x[6])
/*********************************************************************
* TYPEDEFS
*/


enum package_send_state {
    PACKAGE_WAITING_FOR_SENDDING = 0,
    PACKAGE_WAITING_FOR_ACK,
    PACKAGE_SENDDED_SUCCESSFULLY

};

/*used to implement state machine*/
enum package_state {
    PACKAGE_BEGIN = 0,
    PACKAGE_TRANSFER,
    PACKAGE_PAYLOAD,
    PACKAGE_END,
    PACKAGE_NULL,
    PACKAGE_CMDACK_READLOWFIVEBYTES,
    PACKAGE_READHIGHFIVEBYTES,
    PACKAGE_CMDACK_RECIEVELOWFIVEBYTES,
    PACKAGE_RECIEVEHIGHFIVEBYTES
};

/*********************************************************************
* GLOBAL VARIABLES
*/
#define MESSAGE_MAX_CHAIN_LENTH 	8
#define CLING_TX_UART_FIFO_POLL_PERIOAD 100

/*********************************************************************
* LOCAL VARIABLES
*/
LOCAL CLASS(uart) *phy_obj = NULL;
/*recieve fifo related pointer*/
LOCAL enum package_state current_state = PACKAGE_NULL;
LOCAL struct massage_mac_layer_payload_rev *rev_payload_list_header = NULL;
LOCAL struct massage_mac_layer_payload_rev *rev_payload_list_tail = NULL;

LOCAL void (*cmd_recieved_callback)(char cmd) = NULL;

LOCAL SEMAPHORE rev_fifo_semphore = UNLOCKED;


/**/
LOCAL struct massage_mac_layer_payload_send *send_package_list_header = NULL;
LOCAL struct massage_mac_layer_payload_send *send_package_list_tail = NULL;
LOCAL SEMAPHORE send_fifo_semphore = UNLOCKED;

LOCAL os_timer_t  uart_send_timer;
LOCAL bool send_package_assemble(struct massage_mac_layer_payload_send *payload_temp, enum package_type type);
LOCAL bool add_payload2revlist(uint8 *pbuffer, size_t size);
LOCAL bool mac_sendlist_mantain_demon();
LOCAL void received_data_process(RcvMsgBuff *para, enum package_type type);
int massage_receive_one_char_callback(uint8 rev_char, RcvMsgBuff *para);
LOCAL void received_data_process(RcvMsgBuff *para, enum package_type type);

/******************************************************************************
 * FunctionName : receive_one_char_callback
 * Description  : call back function when received one char
 * Parameters   : uint8 rev_char
 * Returns      : none
*******************************************************************************/
int massage_receive_one_char_callback(uint8 rev_char, RcvMsgBuff *para)
{
#if 0
    MAC_SEND_BUFFER(&rev_char, 1);
#endif
   // CLING_DEBUG("rev_char = 0x%02x state = %d \r\n",rev_char,current_state);

    //*(para.pWritePos) = rev_char;
    /*this is a state machien from now on*/
    switch (current_state) {
    case PACKAGE_END:
    case PACKAGE_NULL: {
        if (rev_char == PACKAGE_HEADER_WRTITE_LOWFIVEBYTESREAD_ORDER) {
            /*change current state machine*/
            current_state = PACKAGE_CMDACK_RECIEVELOWFIVEBYTES;
            /*reset message buffer write pointer*/
            /*onece a start header started*/
            para->pWritePos = para->pRcvMsgBuff;
            para->pWritePos++;
            *(para->pWritePos) = rev_char;
            para->BuffState = UNDER_WRITE;
        } else if(rev_char == PACKAGE_HEADER_READORDER_HIGH_FIVE_BYTES) {
            /*change current state machine*/
            current_state = PACKAGE_RECIEVEHIGHFIVEBYTES;
            /*reset message buffer write pointer*/
            /*onece a start header started*/
            para->pWritePos = para->pRcvMsgBuff;
            *(para->pWritePos) = rev_char;
            para->pWritePos++;
            para->BuffState = UNDER_WRITE;

        }
    }
    break;

    case PACKAGE_CMDACK_RECIEVELOWFIVEBYTES:
        if (rev_char == PACKAGE_HEADER_WRTITE_LOWFIVEBYTESREAD_ORDER) {
            /*change current state machine*/
            current_state = PACKAGE_CMDACK_RECIEVELOWFIVEBYTES;
            /*reset message buffer write pointer*/
            /*onece a start header started*/
            para->pWritePos = para->pRcvMsgBuff;
            para->pWritePos++;
            *(para->pWritePos) = rev_char;
            para->BuffState = UNDER_WRITE;
            break;
        } else if(rev_char == PACKAGE_HEADER_READORDER_HIGH_FIVE_BYTES) {
            /*change current state machine*/
            current_state = PACKAGE_RECIEVEHIGHFIVEBYTES;
            /*reset message buffer write pointer*/
            /*onece a start header started*/
            para->pWritePos = para->pRcvMsgBuff;
            *(para->pWritePos) = rev_char;
            para->pWritePos++;
            para->BuffState = UNDER_WRITE;
            break;
        }

        *(para->pWritePos) = rev_char;
        para->pWritePos++;
        if(para->pWritePos - para->pRcvMsgBuff == 3) {
            /*when in 0xf0 mode, we need to check this pakage is a cmd ack one or a data send back one*/
            if(*(para->pWritePos-1) == (uint8)(~(*(para->pWritePos - 2)))) {
                /*this is a cmd ack package*/

              //  CLING_DEBUG("cmd ack recieved\r\n");
				received_data_process(para, PACKAGE_CMD_ACK);
                para->pWritePos = para->pRcvMsgBuff;
                current_state = PACKAGE_END;
            }
        } else if(para->pWritePos - para->pRcvMsgBuff >= 7) {
            /*when in 0xf0 mode, we need to check this pakage is a cmd ack one or a data send back one*/

           // CLING_DEBUG("low five status bytes recieved\r\n");
			received_data_process(para, PACKAGE_LOW_FIVEBYTES);
            para->pWritePos = para->pRcvMsgBuff;
            current_state = PACKAGE_END;
        }
        break;
    case PACKAGE_RECIEVEHIGHFIVEBYTES:
        if (rev_char == PACKAGE_HEADER_WRTITE_LOWFIVEBYTESREAD_ORDER) {
            /*change current state machine*/
            current_state = PACKAGE_CMDACK_RECIEVELOWFIVEBYTES;
            /*reset message buffer write pointer*/
            /*onece a start header started*/
            para->pWritePos = para->pRcvMsgBuff;
            para->pWritePos++;
            *(para->pWritePos) = rev_char;
            para->BuffState = UNDER_WRITE;
            break;
        } else if(rev_char == PACKAGE_HEADER_READORDER_HIGH_FIVE_BYTES) {
            /*change current state machine*/
            current_state = PACKAGE_RECIEVEHIGHFIVEBYTES;
            /*reset message buffer write pointer*/
            /*onece a start header started*/
            para->pWritePos = para->pRcvMsgBuff;
            *(para->pWritePos) = rev_char;
            para->pWritePos++;
            para->BuffState = UNDER_WRITE;
            break;
        }

        *(para->pWritePos) = rev_char;
        para->pWritePos++;

        if(para->pWritePos - para->pRcvMsgBuff >= 7) {
            /*when in 0xf0 mode, we need to check this pakage is a cmd ack one or a data send back one*/
           // =CLING_DEBUG("high five status bytes recieved\r\n");
			received_data_process(para, PACKAGE_HIGH_FIVEBYTES);
            para->pWritePos = para->pRcvMsgBuff;
            current_state = PACKAGE_END;
        }
        break;
    }
    return  ((current_state == PACKAGE_END)?1:0);
}

/******************************************************************************
 * FunctionName : set_massage_recieved_cmd_call_back
 * Description  : call back function when received one char
 * Parameters   : uint8 rev_char
 * Returns      : none
*******************************************************************************/

void ICACHE_FLASH_ATTR
set_massage_recieved_cmd_call_back(void (*callback)(char))
{

    cmd_recieved_callback = callback;

}
/******************************************************************************
 * FunctionName : macsend_ack_recieved()
 * Description  : internally used when a ack package is recieved
 * Parameters   : struct mac_layer_payload_rev *ppayload: mac layer layload pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
macsend_ack_recieved()
{
    struct massage_mac_layer_payload_send *payload_temp = send_package_list_header;
    /**/
    if (NULL == payload_temp) {
        return TRUE;

    } else {

        if (payload_temp->state == PACKAGE_WAITING_FOR_ACK) {
            /*clent has revieced the package successfully */
            payload_temp->state = PACKAGE_SENDDED_SUCCESSFULLY;
			//mac_sendlist_mantain_demon();
        }
    }


    return TRUE;


}



/******************************************************************************
 * FunctionName : receive_one_char_callback
 * Description  : call back function when received one char
 * Parameters   : uint8 rev_char
 * Returns      : none
*******************************************************************************/

LOCAL void ICACHE_FLASH_ATTR
received_data_process(RcvMsgBuff *para, enum package_type type)
{

    /*this means this is a ack package*/
    if (type == PACKAGE_CMD_ACK) {

    } else if (type == PACKAGE_HIGH_FIVEBYTES) {
        /*add messge to reveived messge list waiting for processing*/
	//	CLING_DEBUG("five status bytes recieved\r\n");
        if(IS_RECIEVED_DATA_VALID(para->pRcvMsgBuff)){
			CLING_DEBUG("valid\r\n");
        	add_payload2revlist(para->pRcvMsgBuff + 1, (para->pWritePos - para->pRcvMsgBuff - 1));
		}
		// send_package_assemble(NULL, PACKAGE_ACK);
    } else if (type == PACKAGE_LOW_FIVEBYTES) {
		//CLING_DEBUG("five status bytes recieved\r\n");
		if(IS_RECIEVED_DATA_VALID(para->pRcvMsgBuff)){
		//	CLING_DEBUG("valid\r\n");
        	add_payload2revlist(para->pRcvMsgBuff + 1, (para->pWritePos - para->pRcvMsgBuff - 1));
		}
        /*for the real time purpose, call back function is prefered*/
        /*after receieving ,ssend back ack package*/
       // send_package_assemble(NULL, PACKAGE_ACK);
    }

     macsend_ack_recieved();
}


/******************************************************************************
 * FunctionName : add_payload2list
 * Description  : internally usedly add a data struct to the chain list
 * Parameters   : uint8 rev_char
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
add_payload2revlist(uint8 *pbuffer, size_t size)
{

    /*if this lock is occupied by other process then return*/
    if (IS_SEMAPHORE_LOCKED(rev_fifo_semphore)) {
        return FALSE;
    }
    /*enter critical section so that this is a atom acess*/
    LOCK_SEMAPHORE(rev_fifo_semphore);

    if (rev_payload_list_tail != NULL) {
        if (rev_payload_list_tail->num >= MESSAGE_MAX_CHAIN_LENTH) {
            CLING_DEBUG("rev_payload_list_tail full return error\n");
            /*check if the queue is full*/
            goto FAILED;
        }
    }

    struct massage_mac_layer_payload_rev *payload_temp = (struct massage_mac_layer_payload_rev*)os_malloc(sizeof(struct massage_mac_layer_payload_rev));
    assert(NULL != payload_temp);
#if 0
    payload_temp->ppayload = (char*)os_malloc(size);
    assert(NULL != payload_temp->ppayload);
    /*initialize payload structor*/
#endif
    /*in case of overlow of buffer*/
    if(size > MASSAGE_MAX_BUFFER ) {
        size = MASSAGE_MAX_BUFFER;
    }
    os_memcpy(payload_temp->ppayload, pbuffer, size);
    payload_temp->next = NULL;
    payload_temp->lenth = size;


    /*add this structor to the list*/
    if (rev_payload_list_header == NULL) {
        rev_payload_list_header = payload_temp;
        payload_temp->num = 0;
    } else {

        payload_temp->num = rev_payload_list_tail->num + 1;
        /*assert */
        rev_payload_list_tail->next = payload_temp;

    }

    rev_payload_list_tail = payload_temp;


SUCESS:
    /*leave criticla section ,release semphore here*/
    UNLOCK_SEMAPHORE(rev_fifo_semphore);
    return TRUE;
FAILED:
    /*leave criticla section ,release semphore here*/
    UNLOCK_SEMAPHORE(rev_fifo_semphore);
    return FALSE;

}

 
/******************************************************************************
 * FunctionName : send_package_assemble()
 * Description  : internally used ,obbtain a data struct to the chain list
 * Parameters   : struct massage_mac_layer_payload_rev *ppayload: mac layer layload pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
send_package_assemble(struct massage_mac_layer_payload_send *payload_temp, enum package_type type)
{
	uint8 buffer[MASSAGE_MAX_BUFFER];
	int n = 0;
	switch(type){
		/*
			package frame structor:: 0xf0+ÃüÁîÂë£¨¶ÁÊý¾ÝÂë³ýÍâ£©£«ÃüÁî·´Âë
		*/
		case PACKAGE_CMD:
		case PACKAGE_LOW_FIVEBYTES:
			buffer[n++] = PACKAGE_HEADER_WRTITE_LOWFIVEBYTESREAD_ORDER;
			break;

		case PACKAGE_HIGH_FIVEBYTES:			
			buffer[n++] = PACKAGE_HEADER_READORDER_HIGH_FIVE_BYTES;
			break;
		default:break;

	}
	payload_temp->resend_times++;
	buffer[n++] = payload_temp->ppayload[0];/*command bytes*/
	buffer[n++] = (uint8)(~payload_temp->ppayload[0]);
	//uart0_tx_buffer(payload_temp->ppayload, payload_temp->lenth);
	MAC_SEND_BUFFER(buffer, (n));
    return TRUE;
}

/******************************************************************************
 * FunctionName : massage_mac_send_payload
 * Description  : internally used ,obbtain a data struct to the chain list
 * Parameters   : struct mac_layer_payload_rev *ppayload: mac layer layload pointer
 * Returns      : none
*******************************************************************************/
bool ICACHE_FLASH_ATTR
massage_mac_send_payload(char *ppayload, size_t lenth, uint8 type)
{

    //CLING_DEBUG("==============ble fota============%d\n", lenth);
    /*if this lock is occupied by other process then return*/
    if (IS_SEMAPHORE_LOCKED(send_fifo_semphore)) {
        return FALSE;
    }
    /*enter critical section to lock semphore so to make sure this is a atom acess*/
    LOCK_SEMAPHORE(send_fifo_semphore);

    /*check  heap is enough or not*/
    if (system_get_free_heap_size() < lenth) {
        CLING_DEBUG("heap is not enought%d\n",system_get_free_heap_size());
        goto FAILED;
    }
    struct massage_mac_layer_payload_send *payload_temp = (struct massage_mac_layer_payload_send*)os_malloc(sizeof(struct

            massage_mac_layer_payload_send));
    if	(NULL == payload_temp) {
        goto FAILED;
    }
    /*RESET RESEND TIMES*/
    payload_temp->resend_times = 0;

#if 0
    payload_temp->ppayload = (char*)os_malloc(lenth);
    if	(NULL == payload_temp->ppayload) {
        goto FAILED;
    }
#endif
    /*in case of overlow of buffer*/
    if(lenth > MASSAGE_MAX_BUFFER ) {
        lenth = MASSAGE_MAX_BUFFER;
    }
    /*copy data from buffer*/
    os_memcpy(payload_temp->ppayload, ppayload, lenth);
    payload_temp->state = PACKAGE_WAITING_FOR_SENDDING;
	payload_temp->pakage_type = type;
    payload_temp->lenth = lenth;
    payload_temp->next = NULL;

    /**/
    if (NULL == send_package_list_header) {

        send_package_list_header = payload_temp;
        /*if there is no more data listed in chain send this immmidiately*/
        payload_temp->state = PACKAGE_WAITING_FOR_ACK;/*this must be set v=before being sended*/
        send_package_assemble(send_package_list_header, payload_temp->pakage_type);
		//payload_temp->resend_times++;
        /*restart flag updating progress*/
        os_timer_disarm(&(uart_send_timer));
        os_timer_setfn(&(uart_send_timer), (os_timer_func_t *)mac_sendlist_mantain_demon, 0);
        os_timer_arm(&(uart_send_timer), CLING_TX_UART_FIFO_POLL_PERIOAD, 0);

    } else {

        send_package_list_tail->next = payload_temp;
    }
    /*rewind the pointer to the end of chain*/
    send_package_list_tail = payload_temp;

SUCESS:
    /*leave critical section ,release semphore here*/
    UNLOCK_SEMAPHORE(send_fifo_semphore);
    return TRUE;
FAILED:
    /*leave criticla section ,release semphore here*/
    UNLOCK_SEMAPHORE(send_fifo_semphore);
    return FALSE;


}

/******************************************************************************
 * FunctionName : obbtain_payload_from_revlist
 * Description  : internally used ,obbtain a data struct to the chain list
 * Parameters   : struct mac_layer_payload_rev *ppayload: mac layer layload pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
mac_sendlist_mantain_demon()
{

    struct massage_mac_layer_payload_send *payload_temp = send_package_list_header;
    /*if this lock is occupied by other process then return*/
    if (IS_SEMAPHORE_LOCKED(send_fifo_semphore)) {
        return FALSE;
    }
    /*enter critical section to lock semphore so to make sure this is a atom acess*/
    LOCK_SEMAPHORE(send_fifo_semphore);
    /**/
    if (NULL == payload_temp) {
        /*if the fifo ha already been empty,then it is not gonna rearm the timer again*/
        goto SUCESS;

    } else {
        /*current data need to be transmitted*/
        if (payload_temp->state == PACKAGE_WAITING_FOR_SENDDING) {

            send_package_assemble(payload_temp, payload_temp->pakage_type);
            //payload_temp->resend_times++;
            /*send data over here*/
            payload_temp->state = PACKAGE_WAITING_FOR_ACK;

        } else if (payload_temp->state == PACKAGE_WAITING_FOR_ACK) {

            if (payload_temp->resend_times < RESEND_TIMES_UPPER_THREADHOLD) {
                send_package_assemble(payload_temp, payload_temp->pakage_type);
                // MAC_SEND_BUFFER(payload_temp->, position);
               // payload_temp->resend_times ++;
            } else {

                CLING_DEBUG("send time up\n");
                /*remove the packege sended successfully*/
                send_package_list_header = send_package_list_header->next;
                /*free payload buffer*/
                //os_free(payload_temp->ppayload);
                os_free(payload_temp);
                /*once abandon one package send nex package*/
                payload_temp = send_package_list_header;
                if (payload_temp != NULL) {
                    send_package_assemble(payload_temp, payload_temp->pakage_type);
                }

            }
            /*timeout process*/

        } else if (payload_temp->state == PACKAGE_SENDDED_SUCCESSFULLY) {
            /*remove the packege sended successfully*/
            send_package_list_header = send_package_list_header->next;
            /*free payload buffer*/
            //os_free(payload_temp->ppayload);
            os_free(payload_temp);
            /*once recieved ack send nex package*/
            payload_temp = send_package_list_header;
            if (payload_temp != NULL) {
                send_package_assemble(payload_temp, payload_temp->pakage_type);
            }


        }
        CLING_DEBUG("mac_sendlist_mantain_demon()\r\n");
        /*arm the check routine period again*/
        os_timer_arm(&(uart_send_timer), CLING_TX_UART_FIFO_POLL_PERIOAD, 0);
    }

SUCESS:
    /*leave criticla section ,release semphore here*/
    UNLOCK_SEMAPHORE(send_fifo_semphore);
    return TRUE;
FAILED:
    /*leave criticla section ,release semphore here*/
    UNLOCK_SEMAPHORE(send_fifo_semphore);
    return FALSE;


}

/******************************************************************************
 * FunctionName : massage_obtain_payload_from_revlist
 * Description  : internally used ,obbtain a data struct to the chain list
 * Parameters   : struct mac_layer_payload_rev *ppayload: mac layer layload pointer
 * Returns      : none
*******************************************************************************/
bool ICACHE_FLASH_ATTR
massage_obtain_payload_from_revlist(struct massage_mac_layer_payload_rev **ppayload)
{

    /*if this lock is occupied by other process then return*/
    if (IS_SEMAPHORE_LOCKED(rev_fifo_semphore)) {
        return FALSE;
    }

    /*enter critical section to lock semphore so to make sure this is a atom acess*/
    LOCK_SEMAPHORE(rev_fifo_semphore);

    struct massage_mac_layer_payload_rev *ptemp = rev_payload_list_header;
    /*initialte parameter passed to this function*/
    *ppayload = NULL;
    /*if no data existed*/
    if (ptemp == NULL) {
        goto FAILED;
    } else {
        /*there is only one data struct exsited in fifo*/
        if (ptemp == rev_payload_list_tail) {
            *ppayload = ptemp;
            rev_payload_list_header = NULL;
            rev_payload_list_tail = NULL;
            goto SUCESS;
        }
        /*more than two data block existed, SO first step is to find the the last data block*/
        for (; ptemp->next != rev_payload_list_tail; ptemp = ptemp->next);
        /*assign the last */
        *ppayload = rev_payload_list_tail;
        rev_payload_list_tail = ptemp;
        ptemp->next = NULL;
    }
SUCESS:
    /*leave criticla section ,release semphore here*/
    UNLOCK_SEMAPHORE(rev_fifo_semphore);
    return TRUE;
FAILED:
    /*leave criticla section ,release semphore here*/
    UNLOCK_SEMAPHORE(rev_fifo_semphore);
    return FALSE;
}

/******************************************************************************
 * FunctionName : init_massage_mac_layer
 * Description	: implement uart mac layer interface
 * Parameters	: none
 * Returns		: none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
init_massage_mac_layer()
{
    /*initiate phisical tranmit method*/
    NEW(phy_obj,  uart);
#if 0
    phy_obj->recv_callback_register(phy_obj, receive_one_char_callback);
#endif
    return TRUE;
}




