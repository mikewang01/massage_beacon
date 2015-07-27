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
#include "cling_ap_para.h"
#include "user_json.h"
#include "driver/uart.h"

#include "uart_protocol/uart_protocol_cmd.h"
#include "cling_fifo.h"
#include "smartconfig.h"
#include "io_assignment.h"
#include "gpio.h"

/*********************************************************************
* MACROS
*/

#define SMARTCONFIG_BLINK_PERIOAD   500

/*********************************************************************
* TYPEDEFS
*/
struct interact{
	uint16 reg_task_id; /*接收相关消息的注册任务ID*/
	void (*smart_config_call_back)(void *arg);/*任务完成回调注册函数*/
};

	
/*********************************************************************
* GLOBAL VARIABLES
*/
	
	
/*********************************************************************
* LOCAL VARIABLES
*/
LOCAL CLASS(cling_ap_para) *this = NULL;/*http service layer object*/
LOCAL struct station_config *sta_conf = NULL;

LOCAL bool delete_cling_ap_para(CLASS(cling_ap_para) *arg);
LOCAL bool cling_ap_para_start_smartconfig(CLASS(cling_ap_para) *arg);
LOCAL bool cling_ap_para_stop_smartconfig(CLASS(cling_ap_para) *arg);
LOCAL uint16 task_id_reg = USER_TASK_PRIO_MAX + 1;
LOCAL  os_timer_t smart_key_long_press;
LOCAL  bool get_data_by_smartconfig = FALSE;
LOCAL os_timer_t link_led_timer;
/*USED TO TOGGLE LED STATE*/
LOCAL uint8 link_led_level = 0;


/******************************************************************************
 * FunctionName : cling_smartconfig_led_related
 * Description  : control the output of smartconfig led
 * Parameters   : level : output level of pin 
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_smartconfig_led_output(uint8 level)
{
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_SMARTCONFIG_LED_IO_NUM), level);
}


LOCAL void ICACHE_FLASH_ATTR
cling_smartconfig_led_timer_cb(void)
{
    link_led_level = (~link_led_level) & 0x01;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_SMARTCONFIG_LED_IO_NUM), link_led_level);
	
}


LOCAL void ICACHE_FLASH_ATTR
cling_smartconfig_led_init(void)
{

	PIN_FUNC_SELECT(CLING_SMARTCONFIG_LED_IO_MUX, CLING_SMARTCONFIG_LED_IO_FUNC);
    os_timer_disarm(&link_led_timer);
    os_timer_setfn(&link_led_timer, (os_timer_func_t *)cling_smartconfig_led_timer_cb, NULL);
    os_timer_arm(&link_led_timer, SMARTCONFIG_BLINK_PERIOAD, 1);
    link_led_level = 0;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_SMARTCONFIG_LED_IO_NUM), link_led_level);
}

LOCAL void ICACHE_FLASH_ATTR
cling_smartconfig_led_stop(void)
{
	CLING_DEBUG("smart config led bink stop\n");
    os_timer_disarm(&link_led_timer);
    link_led_level = 0;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_SMARTCONFIG_LED_IO_NUM), link_led_level);
}




/******************************************************************************
 * FunctionName : cling_load_ap_param
 * Description  : load ap parameter from flash,e.
 * Parameters   : 
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
cling_load_ap_param()
{
    struct station_config config = {0};
	
	this->op_para_p->load_parameter(CLING_PARAM_AP, (char *)&config, sizeof(struct station_config));
#if 0
    spi_flash_read((CLING_PARAM_START_SEC + CLING_PARAM_AP) * SPI_FLASH_SEC_SIZE,
                   (uint32 *)&config, sizeof(struct station_config));
#endif
	/*this means there is a ap parameter stored in flash*/
	if ('0'<=config.ssid[0]<='9' && 'a'<=config.ssid[0]<='z' && 'A'<=config.ssid[0]<='Z'){
			sta_conf = (struct station_config *)os_malloc(sizeof(struct station_config));
			if (NULL != sta_conf){
			*sta_conf = config;
			return TRUE;
		}
	}
		return FALSE;  
}

/******************************************************************************
 * FunctionName : cling_load_ap_param
 * Description  : load ap parameter from flash,e.
 * Parameters   : 
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
cling_save_ap_param(struct station_config *p)
{
	if (NULL != p){
		this->op_para_p->save_parameter(CLING_PARAM_AP, (char *)p, sizeof(struct station_config));
#if 0
		spi_flash_erase_sector(CLING_PARAM_START_SEC + CLING_PARAM_AP);
		/*smart config suceceed*/
		spi_flash_write((CLING_PARAM_START_SEC + CLING_PARAM_AP) * SPI_FLASH_SEC_SIZE,
						   (uint32 *)p, sizeof(struct station_config));
#endif
			
		}
  
}

/******************************************************************************
 * FunctionName : smartconfig_done
 * Description  : smart config finished call back
 * Parameters   : arg : station paramater pointer
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
smartconfig_done(void *arg)
{
	
	sta_conf = (struct station_config *)os_malloc(sizeof(struct station_config));
	/*smart config sucessfully*/
	cling_smartconfig_led_stop();
	if (NULL != sta_conf){
		*sta_conf = *((struct station_config*)arg);
		CLING_DEBUG("ssid = %s\n", sta_conf->ssid);
		cling_save_ap_param(arg);
		/*this means ssid existed in flash memory*/
		struct interact * register_inf = (struct interact*)(this->user_data);
		if (NULL != register_inf){
				/*if user callback task existed*/
			if (NULL != register_inf->smart_config_call_back){
				register_inf->smart_config_call_back(sta_conf);
			}
		}
	}
	get_data_by_smartconfig = TRUE;
	
	/*load timestamp parameter from flash*/
	//load_sys_timestamp_parameter(arg);
}
/******************************************************************************
 * FunctionName : get_ap_paramater
 * Description  : smart config finished call back
 * Parameters   : arg : station paramater pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
get_ap_paramater(CLASS(cling_ap_para) *arg, struct station_config *psta)
{
	if (psta != NULL){
		*psta = *sta_conf;
		return TRUE;
	}
	return FALSE;
	/*load timestamp parameter from flash*/
	//load_sys_timestamp_parameter(arg);
}
/******************************************************************************
 * FunctionName : register_cling_ap_taskid
 * Description  : initiate init_cling_ap_para object
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
bool ICACHE_FLASH_ATTR
register_cling_ap_taskid(CLASS(cling_ap_para) *arg, uint16 task_id)
{
	assert(NULL != arg);
	if (IS_TASK_VALID(task_id)){
		task_id_reg = task_id;
	}
	return TRUE;
	/*load timestamp parameter from flash*/
//	load_sys_timestamp_parameter(arg);
}

/******************************************************************************
 * FunctionName : register_cling_ap_callbackfunc
 * Description  : register smartconfig done user callback function
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
bool ICACHE_FLASH_ATTR
register_cling_ap_callbackfunc(CLASS(cling_ap_para) *arg, void (*smart_config_call_back)(void*))
{
	assert(NULL != arg && NULL != smart_config_call_back);
	
	struct interact * register_inf = (struct interact*)(arg->user_data);
	assert(NULL != register_inf);
	register_inf->smart_config_call_back = smart_config_call_back;

	return TRUE;
}

/******************************************************************************
 * FunctionName : smart_config_key_init
 * Description  : initiate smart config input key
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

bool ICACHE_FLASH_ATTR
smart_config_key_init(){
	 PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	/*Use MTDI pin as GPIO13*/
	/*set key gpio as input*/
	 gpio_output_set(0, 0, 0, CLING_KEY_0_IO_NUM);
}

/******************************************************************************
 * FunctionName : cling_smart_config_start
 * Description  : 
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

bool ICACHE_FLASH_ATTR
cling_smart_config_start(sc_type type, sc_callback_t cb){
	cling_smartconfig_led_init();
	smartconfig_start(type, cb);
}



/******************************************************************************
 * FunctionName : smart_config_long_press_callback
 * Description  : when press the key longer than 5s ,this is gonna be called
 * Parameters   : parameter arg
 * Returns      : none
*******************************************************************************/

bool ICACHE_FLASH_ATTR
smart_config_long_press_callback(CLASS(cling_ap_para) *arg){
	/*this means customer press the key for a long time enter smart config mode*/
	if (GPIO_INPUT_GET(CLING_KEY_0_IO_NUM) == CLING_KEY_0_ACTIVE_LEVEL){
		CLING_DEBUG("long press entered \n");
		wifi_set_opmode(STATION_MODE);
		/*indicate that device has entered smart config mode*/
		//cling_smartconfig_led_init();
		cling_smart_config_start(SC_TYPE_ESPTOUCH, smartconfig_done);
	}else{
		CLING_DEBUG("short press entered \n");
		/*customer press the button for a short time*/
		/*no ap parameter existed in flash*/
		if (cling_load_ap_param() == FALSE){
			wifi_set_opmode(STATION_MODE);
			cling_smart_config_start(SC_TYPE_ESPTOUCH, smartconfig_done);
		}else{
			/*this means ssid existed in flash memory*/
			struct interact * register_inf = (struct interact*)(arg->user_data);
			assert(NULL != register_inf);
			/*if user callback task existed*/
			if (NULL != register_inf->smart_config_call_back){
				register_inf->smart_config_call_back(sta_conf);
			}
			CLING_DEBUG("flash ssid = %s\n", sta_conf->ssid);
		}

	}

}

/******************************************************************************
 * FunctionName : init_cling_ap_para
 * Description  : initiate init_cling_ap_para object
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
bool ICACHE_FLASH_ATTR
init_cling_ap_para(CLASS(cling_ap_para) *arg)
{
	assert(NULL != arg);
	smart_config_key_init();
	/*initiate  registered information*/
	struct interact * register_inf = (struct interact*)os_malloc(sizeof(struct interact));
	assert(NULL != register_inf);

	/*struct parameter operation*/
	NEW(arg->op_para_p ,cling_para_interface); 
		
	register_inf->reg_task_id = USER_TASK_PRIO_MAX + 1;
	register_inf->smart_config_call_back =NULL;
	arg->user_data = register_inf;
	
	arg->init = init_cling_ap_para;
	arg->de_init = delete_cling_ap_para;
	arg->start_smartconfig = cling_ap_para_start_smartconfig;
	arg->stop_smartconfig = cling_ap_para_stop_smartconfig;
	arg->get_sta_conf = get_ap_paramater;
	arg->register_callback = register_cling_ap_callbackfunc;
	
	this = arg;
	
	return TRUE;
	/*load timestamp parameter from flash*/
//	load_sys_timestamp_parameter(arg);
}



/******************************************************************************
 * FunctionName : cling_ap_para_start_smartconfig
 * Description  : start smart config operation
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
cling_ap_para_start_smartconfig(CLASS(cling_ap_para) *arg)
{
	assert(NULL != arg);
	/*customer press the key at power up*/
   if (GPIO_INPUT_GET(CLING_KEY_0_IO_NUM) == CLING_KEY_0_ACTIVE_LEVEL){
		CLING_DEBUG("key pressed\n");
		/**/
		os_timer_disarm(&smart_key_long_press);
		os_timer_setfn(&smart_key_long_press, (os_timer_func_t *)smart_config_long_press_callback, arg);
    	os_timer_arm(&smart_key_long_press, 3000, 0);


   }else{
   			CLING_DEBUG("no key pressed\n");
			/*no ap parameter existed in flash*/
		if (cling_load_ap_param() == FALSE){
			wifi_set_opmode(STATION_MODE);
			cling_smart_config_start(SC_TYPE_ESPTOUCH, smartconfig_done);
		}else{
			/*this means ssid existed in flash memory*/
			struct interact * register_inf = (struct interact*)(arg->user_data);
			assert(NULL != register_inf);
			/*if user callback task existed*/
			if (NULL != register_inf->smart_config_call_back){
				register_inf->smart_config_call_back(sta_conf);
			}
			CLING_DEBUG("flash ssid = %s\n", sta_conf->ssid);
		}
	}
	
	/*load timestamp parameter from flash*/
	return TRUE;
}

/******************************************************************************
 * FunctionName : cling_ap_para_start_smartconfig
 * Description  : start smart config operation
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
cling_ap_para_stop_smartconfig(CLASS(cling_ap_para) *arg)
{
	assert(NULL != arg);
	/*obtain ssid inf through smartconfig so is it neccasarry to delete the data here*/
	if (get_data_by_smartconfig == TRUE){
		CLING_DEBUG("stop smart config\n");
		smartconfig_stop();
	/*load timestamp parameter from flash*/
	}
	
	//load_sys_timestamp_parameter(arg);
	return TRUE;
}

/******************************************************************************
 * FunctionName : cling_ap_para_start_smartconfig
 * Description  : start smart config operation
 * Parameters   : arg : object pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
delete_cling_ap_para(CLASS(cling_ap_para) *arg)
{
	assert(NULL != arg);
	DELETE(arg->op_para_p ,cling_para_interface);
	os_free(arg->user_data);
	os_free(sta_conf);
	os_free(arg);
	/*reset this flag*/
	get_data_by_smartconfig = FALSE;
	this = NULL;
	/*load timestamp parameter from flash*/
	//load_sys_timestamp_parameter(arg);
	return TRUE;
}



