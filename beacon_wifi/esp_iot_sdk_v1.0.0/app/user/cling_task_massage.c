/***********************************************************************************
 * 文 件 名   : cling_task_massage.c
 * 负 责 人   : mike
 * 创建日期   : 2015年8月6日
 * 文件描述   : 
 * 版权说明   : Copyright (c) 2008-2015   xx xx xx xx 技术有限公司
 * 其    他   : 
 * 修改日志   : 
***********************************************************************************/

/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: cling_task_massage.c
 *
 * Description: Thecling tastk massage mode calcultator and resiponsible for communication wirth massage chair.
 *              Check your hardware connection with the host while use this mode.
 * Modification history:
 *     2015/8/5, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "Hmac_sha1.h"
#include "task_signal.h"
#include "time_sync_json.h"
#include "sys_timestamp_mgr.h"
#include "cling_upload_json.h"


#include "user_iot_version.h"
#include "espconn.h"
#include "user_json.h"
#include "user_esp_platform.h"
#include "user_webclient.h"
#include "user_json.h"
#include "driver/uart.h"

#include "protocol/protocol_cmd.h"
#include "cling_fifo.h"
#include "cling_ap_para.h"
#include "io_assignment.h"
#include "gpio.h"
#include "cling_fota.h"
#include "cling_healthdata_json.h"
#include "cling_rtc.h"
#include "ble_fota.h"
#include "protocol/massage_cmd.h"

/*********************************************************************
* MACROS
*/
#ifdef SELF_TASK_ID
#undef SELF_TASK_ID
#endif
#define  SELF_TASK_ID  			USER_TASK_PRIO_2
#define	 TASK_MISC_QUEUE_LEN 	20
/*********************************************************************
* TYPEDEFS
*/


/*********************************************************************
* GLOBAL VARIABLES
*/


/*********************************************************************
* LOCAL VARIABLES
*/


LOCAL CLASS(massage_protocol) *massage_obj;
LOCAL os_event_t *message_queue;


/*********************************************************************
* LOCAL FUNCRTION
*/



/******************************************************************************
 * FunctionName : user_task_data_process
 * Description  : process the data posted to this task
 * Parameters   : port -- server port
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
ipc_event_process(os_event_t *e)
{

    switch (GET_EVENT_TYPE(e->sig)) {

    case EVENT_INIT:
        break;

    case EVENT_GET_AP_INF: 
    break;

    case EVENT_AP_CONNECTED:
       
        break;

    case EVENT_FOTA_START: 
    break;
    case EVENT_OBJECT_RESET:

        break;
	case EVENT_CLING_HEALTH_RECIEVED:{
			static int i = 0;
			if(i%2 == 0){
	    		massage_obj->power_on(massage_obj);
			}else{
				massage_obj->power_off(massage_obj);
			}
			i++;
	}
		break;
    default:
        break;
    }

}


/******************************************************************************
 * FunctionName : user_task_data_process
 * Description  : process the data posted to this task
 * Parameters   : port -- server port
 * Returns      : none
*******************************************************************************/

LOCAL void ICACHE_FLASH_ATTR
http_event_process(os_event_t *e)
{


    switch (GET_EVENT_TYPE(e->sig)) {

    case EVENT_FOTA_CONFIG: 
    break;
    default:
        break;



    }

}
/******************************************************************************
 * FunctionName : timestamp_event_process
 * Description  : process the data posted to this task
 * Parameters   : os_event_t *e : event recieved
 * Returns      : none
*******************************************************************************/

LOCAL void ICACHE_FLASH_ATTR
timestamp_event_process(os_event_t *e)
{

    switch (GET_EVENT_TYPE(e->sig)) {

        /*outofdate event*/
    case EVENT_TIMESTAMP_OUTOFDATE:
        break;
    default:
        break;



    }

}
/******************************************************************************
 * FunctionName : uart_event_process
 * Description  : process the data realted to uart posted to this task
 * Parameters   : os_event_t *e : event recieved
 * Returns      : none
*******************************************************************************/

LOCAL void ICACHE_FLASH_ATTR
uart_event_process(os_event_t *e)
{

    switch (GET_EVENT_TYPE(e->sig)) {

        /*outofdate event*/
    case EVENT_UART_RX:
        break;
    default:
        break;



    }

}



/******************************************************************************
 * FunctionName : user_task_data_process
 * Description  : process specific event passed to this task
 * Parameters   : port -- server port
 * Returns      : none
*******************************************************************************/


LOCAL void ICACHE_FLASH_ATTR
cling_misc_task_data_process(os_event_t *e)
{

    switch (GET_MSG_TYPE(e->sig)) {
    case MSG_IPC:
        ipc_event_process(e);
        break;
    case MSG_HTTP:
        http_event_process(e);
        break;

    case MSG_TIMESTAMP:
        timestamp_event_process(e);
        break;
    case MSG_UART:
        uart_event_process(e);
        break;
    default:
        break;
    }
}


/******************************************************************************
 * FunctionName : user_task_data_process_init
 * Description  : initiate data process task
 * Parameters   : port -- server port
 * Returns      : none
*******************************************************************************/


void ICACHE_FLASH_ATTR
cling_task_massage_process_init(void)
{

    message_queue = (os_event_t *)os_malloc(sizeof(os_event_t)*TASK_MISC_QUEUE_LEN);
    system_os_task(cling_misc_task_data_process, SELF_TASK_ID, message_queue, TASK_MISC_QUEUE_LEN);
	NEW(massage_obj, massage_protocol);
} //1970-01-01 00:00:00.

