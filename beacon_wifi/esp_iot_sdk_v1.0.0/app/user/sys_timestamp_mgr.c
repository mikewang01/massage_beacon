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

#include "user_iot_version.h"
#include "espconn.h"
#include "user_json.h"
#include "user_esp_platform.h"
#include "user_webclient.h"
#include "user_json.h"
#include "sys_timestamp_mgr.h"



/*********************************************************************
* MACROS
*/
#define  TIMESTAMP_FLAGUPDATE_PERIOD (30000) /*1800000MS= 30MINUTES*/
#define  IS_TASK_REGISTERED(__X) ((__X >= USER_TASK_PRIO_0)) && (__X <= USER_TASK_PRIO_MAX)
/*********************************************************************
* TYPEDEFS
*/
struct timestamp_record{
	uint32  timestamp_cnt;/*record latest synced time stamp*/
	uint32  rtc_count; /*record rtc value when ssynchronize timestamp_cnt*/
	uint32  sync_flag; /*flag to indicate if the timestamp has been synced in time*/
};


/*********************************************************************
* GLOBAL VARIABLES
*/
LOCAL uint8 timestamp_registered_taskid = USER_TASK_PRIO_MAX +1;
LOCAL os_timer_t outofdate_timer; /*used to update timestamp status timer*/	
/*********************************************************************
* LOCAL VARIABLES
*/

LOCAL bool delete_sys_timestamp(CLASS(sys_timestamp) *arg);
LOCAL bool load_sys_timestamp_parameter(CLASS(sys_timestamp) *arg);
LOCAL bool save_sys_timestamp_parameter(CLASS(sys_timestamp) *arg);
LOCAL bool get_current_timestamp_mgr(CLASS(sys_timestamp) *arg, uint32 *ptimestamp);
LOCAL bool set_current_timestamp_mgr(CLASS(sys_timestamp) *arg, uint32 timestamp);
LOCAL bool get_current_timestamp_sync_flag(CLASS(sys_timestamp) *arg, enum time_stamp_status *pstatus);
LOCAL bool current_timestamp_task_register(CLASS(sys_timestamp) *arg, uint8 task_id);

LOCAL bool timestamp_outofdate_callback(CLASS(sys_timestamp) *arg);

/******************************************************************************
 * FunctionName : init_sys_timestamp
 * Description  : initiate time stamp object
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
bool ICACHE_FLASH_ATTR
init_sys_timestamp(CLASS(sys_timestamp) *arg)
{
	assert(NULL != arg);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)os_malloc(sizeof(struct timestamp_record));
	assert(NULL != timestamp_data);
	/*initiate */
	arg->user_data = (void*)timestamp_data;
	arg->init = init_sys_timestamp;
	arg->de_init = delete_sys_timestamp;

	arg->msgrev_register = current_timestamp_task_register;
	arg->set_current_timestamp = set_current_timestamp_mgr;
	arg->get_current_timestamp = get_current_timestamp_mgr;
	arg->get_sync_flag = get_current_timestamp_sync_flag;
	arg->load_timestamp_from_flash = load_sys_timestamp_parameter;
	arg->save_timestamp_to_flash = save_sys_timestamp_parameter;

	/*load timestamp parameter from flash*/
	load_sys_timestamp_parameter(arg);
	/*start flag updating progress*/
	os_timer_disarm(&(outofdate_timer));
	os_timer_setfn(&(outofdate_timer), (os_timer_func_t *)timestamp_outofdate_callback, (void*)arg);
    os_timer_arm(&(outofdate_timer), TIMESTAMP_FLAGUPDATE_PERIOD, 0);
}

/******************************************************************************
 * FunctionName : delete_sys_timestamp
 * Description  : delete time stamp object and free occupied memory
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
delete_sys_timestamp(CLASS(sys_timestamp) *arg)
{
	assert(NULL != arg);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)(arg->user_data);
	if (NULL !=timestamp_data ) {
		/*save parameter to flash*/
		save_sys_timestamp_parameter(arg);
		os_free(timestamp_data);
	}

	os_free(arg);
}
/******************************************************************************
 * FunctionName : load_sys_timestamp_parameter
 * Description  : load  sys timestamp parameter from flash
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
load_sys_timestamp_parameter(CLASS(sys_timestamp) *arg)
{
	assert(NULL != arg);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)(arg->user_data);
	assert(NULL != timestamp_data);
	/*
	some user data in RTC memory. Only ¡°user data¡± area can be used by user.
	|_ _ _ _ _system data _ _ _ _ _|_ _ _ _ _ _ _ _ _ user data _ _ _ _ _ _ _ _ _|
	| 256 bytes                    | 512 bytes  |
	Note: RTC memory is 4 bytes aligned for read and write operations. Parameter
	¡°des_addr¡± means block number(4 bytes per block). So, if we want to save
	some data at the beginning of user data area, ¡°des_addr¡± will be 256/4 = 64,
	¡°save_size¡± will be data length.*/

    /*load timestamp parameter inti flash*/
	system_rtc_mem_read(RTC_TIMESTAMP_MEMORY_POSITION, timestamp_data, sizeof(struct timestamp_record));

	CLING_DEBUG("load timestamp_data.timestamp_count=%d, timestamp_data.rtc_count=%d\n, timestamp_data.syn_flg=%d\n",timestamp_data->timestamp_cnt, timestamp_data->rtc_count, timestamp_data->sync_flag);
	return TRUE;
}


/******************************************************************************
 * FunctionName : save_sys_timestamp_parameter
 * Description  : dave sys timestamp parameter to flash
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
save_sys_timestamp_parameter(CLASS(sys_timestamp) *arg)
{
	assert(NULL != arg);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)(arg->user_data);
	assert(NULL != timestamp_data);

    /*load timestamp parameter inti flash*/
	CLING_DEBUG("saved timestamp_data.timestamp_count=%d, timestamp_data.rtc_count=%d\n, timestamp_data.syn_flg=%d\n",timestamp_data->timestamp_cnt, timestamp_data->rtc_count, timestamp_data->sync_flag);
	return system_rtc_mem_write(RTC_TIMESTAMP_MEMORY_POSITION, timestamp_data, sizeof(struct timestamp_record));
	
}

/******************************************************************************
 * FunctionName : get_current_timestamp
 * Description  : get current time stamp
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
get_current_timestamp_mgr(CLASS(sys_timestamp) *arg, uint32 *ptimestamp)
{
	assert(NULL != arg && NULL != ptimestamp);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)(arg->user_data);
	assert(NULL != timestamp_data);
	/*If system_get_rtc_time returns 10 (means 10 RTC cycles),
	system_rtc_clock_cali_proc returns 5 (means 5us per RTC cycle), then real
	time is 10 x 5 = 50 us.*/
	*ptimestamp = timestamp_data->timestamp_cnt + (((system_get_rtc_time() - timestamp_data->rtc_count) * (system_rtc_clock_cali_proc()>>12))/ 1000000);
	/*cpmpemsate 2seconds every minutes*/
	*ptimestamp += ((*ptimestamp - timestamp_data->timestamp_cnt)/60);
#if 0
	CLING_DEBUG("get_current_timestamp_mgr system_get_rtc_time() = %d\n", system_get_rtc_time());
#endif 
	return TRUE ;

}

/******************************************************************************
 * FunctionName : set_current_timestamp
 * Description  : get current time stamp internally used
 * Parameters   : arg : object pointer
 *				  timestamp : current timestamps scraped from server
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
set_current_timestamp_mgr(CLASS(sys_timestamp) *arg, uint32 timestamp)
{
	assert(NULL != arg);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)(arg->user_data);
	assert(NULL != timestamp_data);
#if 1
		CLING_DEBUG("set_current_timestamp_mgr system_get_rtc_time() = %d\n", system_get_rtc_time());
#endif
	/*save current rtc time when set timestamp*/
	timestamp_data->rtc_count = system_get_rtc_time();
	
	timestamp_data->timestamp_cnt = timestamp;
	timestamp_data->sync_flag = TIMESTAMP_SYNCHRONIZED;

	/*restart flag updating progress*/
	os_timer_disarm(&(outofdate_timer));
	os_timer_setfn(&(outofdate_timer), (os_timer_func_t *)timestamp_outofdate_callback, (void*)arg);
    os_timer_arm(&(outofdate_timer), TIMESTAMP_FLAGUPDATE_PERIOD, 0);
	
	return TRUE;
	
}


/******************************************************************************
 * FunctionName : get_current_timestamp_sync_flag
 * Description  : get current time stamp update flag
 * Parameters   : arg : object pointer pstatus: status 
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
get_current_timestamp_sync_flag(CLASS(sys_timestamp) *arg, enum time_stamp_status *pstatus)
{
	assert(NULL != arg && NULL != pstatus);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)(arg->user_data);
	assert(NULL != timestamp_data);

	*pstatus = timestamp_data->sync_flag;

	return TRUE;
	
}

/******************************************************************************
 * FunctionName : current_timestamp_task_register
 * Description  : get current time stamp
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
current_timestamp_task_register(CLASS(sys_timestamp) *arg, uint8 task_id)
{
	assert(NULL != arg);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)(arg->user_data);
	assert(NULL != timestamp_data);

	/*check taskid is illegal or not*/
	if (IS_TASK_REGISTERED(task_id)){
		timestamp_registered_taskid = task_id;
	}
	

	return TRUE;
	
}



/******************************************************************************
 * FunctionName : timestamp_outofdate_callback
 * Description  : get current time stamp
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
timestamp_outofdate_callback(CLASS(sys_timestamp) *arg)
{
	assert(NULL != arg);
	struct timestamp_record *timestamp_data = (struct timestamp_record *)(arg->user_data);
	assert(NULL != timestamp_data);
	/*indicate the stamp flag is not updated intime*/
	timestamp_data->sync_flag = TIMESTAMP_OUTOFDATE;
	/*if there is a task registored ,then send a timestamp outofdate message*/
	if (IS_TASK_REGISTERED(timestamp_registered_taskid)){

		system_os_post(timestamp_registered_taskid, TIMESTAMP_EVENT(EVENT_TIMESTAMP_OUTOFDATE), 0);
		//system_os_post(timestamp_registered_taskid, HTTP_EVENT(EVENT_RX), 'a');
	}else{

		CLING_DEBUG("timestamp no task registered\n");
	}
	
	return TRUE;
	
}




