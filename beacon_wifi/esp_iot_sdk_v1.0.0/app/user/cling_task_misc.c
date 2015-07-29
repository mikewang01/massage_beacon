/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_webserver.c
 *
 * Description: The web server mode configration.
 *              Check your hardware connection with the host while use this mode.
 * Modification history:
 *     2014/3/12, v1.0 create this file.
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
/*********************************************************************
* MACROS
*/
#define  SELF_TASK_ID  			USER_TASK_PRIO_1
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

LOCAL CLASS(cling_uart)  *cling_uart_obj = NULL;
LOCAL struct station_config station_conf;
LOCAL CLASS(cling_ap_para) *smart_config_obj = NULL;
LOCAL os_event_t *messge_queue;
LOCAL CLASS(cling_fota) *cling_fota_obj = NULL;
LOCAL CLASS(ble_fota) *ble_fota_obj = NULL;

LOCAL CLASS(cling_health_data) *health_data_obj = NULL;


/*********************************************************************
* LOCAL FUNCRTION
*/
LOCAL void smart_config_call_back(void *arg);
LOCAL void upgrade_finish_callback(struct upgrade_server_info server);
LOCAL void upgrade_startup_callback();


/******************************************************************************
 * FunctionName : user_task_data_process
 * Description  : process the data posted to this task
 * Parameters   : port -- server port
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
ipc_event_process(os_event_t *e)
{

    char send_buffer[jsonSize];
    switch (GET_EVENT_TYPE(e->sig)) {

    case EVENT_INIT:
        break;

    case EVENT_GET_AP_INF: {
        struct station_config *sta_conf = (struct station_config *)os_malloc(sizeof(struct station_config));
        *sta_conf = station_conf;
        system_os_post(USER_TASK_PRIO_0,  IPC_EVENT(EVENT_INIT), (u32)(sta_conf));
    }
    break;

    case EVENT_AP_CONNECTED:
        /*this means this is the first time to connect with the AP and stop smart config from now*/
        if (smart_config_obj != NULL) {
            smart_config_obj->stop_smartconfig(smart_config_obj);
            DELETE(smart_config_obj, cling_ap_para);
            smart_config_obj = NULL;
        }
        break;

    case EVENT_FOTA_START: {
        char *s ="123";
        CLING_DEBUG("enter ota process\n");
        cling_fota_obj->update_start(cling_fota_obj, s);
    }
    break;
    case EVENT_OBJECT_RESET:

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

    case EVENT_FOTA_CONFIG: {
        struct espconn *p = (struct espconn*)e->par;
        char *t = p->proto.tcp->remote_ip;
        cling_fota_obj->config_server(cling_fota_obj, p);
		ble_fota_obj->config_server(ble_fota_obj, p);
        CLING_DEBUG("MISC TASK server ip %d:%d:%d:%d\n", t[0], t[1], t[2], t[3]);
        os_free(p);
    }
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

    char send_buffer[jsonSize];
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

    char send_buffer[jsonSize];
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


CLASS(rtc_date) *date_temp;

void ICACHE_FLASH_ATTR
cling_task_misc_process_init(void)
{

    char buffer[2048]= {0};
    messge_queue = (os_event_t *)os_malloc(sizeof(os_event_t)*TASK_MISC_QUEUE_LEN);
    system_os_task(cling_misc_task_data_process, SELF_TASK_ID, messge_queue, TASK_MISC_QUEUE_LEN);
    CLING_DEBUG("SDK version:%s\n", system_get_sdk_version());
    CLING_DEBUG("start smart config\n");
    NEW(smart_config_obj, cling_ap_para);
    smart_config_obj->register_callback(smart_config_obj, smart_config_call_back);
    smart_config_obj->start_smartconfig(smart_config_obj);
    NEW(cling_fota_obj, cling_fota);
    cling_fota_obj->register_upgrade_startup_callback(cling_fota_obj, upgrade_startup_callback);
    cling_fota_obj->register_upgrade_finish_callback(cling_fota_obj, upgrade_finish_callback);
	NEW(ble_fota_obj, ble_fota);
	//ble_fota_obj->register_upgrade_startup_callback(ble_fota_obj, upgrade_startup_callback);
	ble_fota_obj->register_upgrade_finish_callback(ble_fota_obj, upgrade_finish_callback);
} //1970-01-01 00:00:00.


/******************************************************************************
 * FunctionName : upgrade_startup_callback
 * Description  : upgrade startup callback
 * Parameters   : arg :struct station_config
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
upgrade_startup_callback()
{

    CLING_DEBUG("enter into ota upgrade startup user call back\n");
    system_os_post(WEB_CLIIENT_TASK_ID,  IPC_EVENT(EVENT_OBJECT_RESET), 0);
    system_os_post(MISC_TASK_ID,  IPC_EVENT(EVENT_OBJECT_RESET), 0);

}

/******************************************************************************
 * FunctionName : upgrade_finish_callback
 * Description  : upgrade finished callback
 * Parameters   : arg :struct station_config
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
upgrade_finish_callback(struct upgrade_server_info server)
{

    CLING_DEBUG("enter into ota upgrade user call back\n");
    /*when upgrade finished, post EVENT_FOTA_FINISHED event with upgrade status*/
    //system_os_post(SELF_TASK_ID,  IPC_EVENT(EVENT_FOTA_FINISHED), server.upgrade_flag);
    /*upgrade sucessfully*/
    if (server.upgrade_flag == TRUE) {
        CLING_DEBUG("upgrade successfully reboot now\n");
		//os_delay_us(200000000);
        system_upgrade_reboot();
    } else {

        CLING_DEBUG("upgrade failed\n");
    }


}



/******************************************************************************
 * FunctionName : smart_config_call_back
 * Description  : smart config finished callback
 * Parameters   : arg :struct station_config
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
smart_config_call_back(void *arg)
{

    CLING_DEBUG("enter into smartconfig user call back\n");
    /*assign station config to local varied*/
    station_conf = *(struct station_config*)arg;
    /*when smart config finished ,start thread*/
    system_os_post(SELF_TASK_ID,  IPC_EVENT(EVENT_GET_AP_INF), 'a');

}


