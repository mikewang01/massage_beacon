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
#include "mem.h"
#include "cling_log_interface.h"
#include "spi_flash.h"
#include "user_esp_platform.h"
#include "osapi.h"
/*********************************************************************
* MACROS
*/

/*********************************************************************
* TYPEDEFS
*/
struct log_layout{
	uint16 log_pos;/*record the lenth of log that has already been saved*/
	uint16 reset_times;/*record the times system reset*/
	bool   sync_flag;
	char   a[0]; /*preseved for future use*/
};
	
/*********************************************************************
* GLOBAL VARIABLES
*/
/*********************************************************************
* LOCAL VARIABLES
*/

LOCAL bool delete_cling_log_interface(CLASS(cling_log_interface) *arg);

LOCAL struct log_layout log_detailed_inf = {
	.log_pos  = 0,
	.reset_times = 0,
	.sync_flag = FALSE,
};
/******************************************************************************
 * FunctionName : print_log
 * Description  : printf log to specific matiariakl
 * Parameters   : 

 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
print_log(char* p_log){

	assert(NULL != p_log);

}
#endif
/******************************************************************************
 * FunctionName : read_log
 * Description  : read log from specific adress
 * Parameters   : char* p_log: buffer where data stored
 				  size_t lenth : log legth it gonna read
 * Returns      :  amount of bytes 
*******************************************************************************/
#if 1
LOCAL int  ICACHE_FLASH_ATTR
read_log(char* p_log, size_t lenth){

	assert(NULL != p_log);

	return TRUE;

}
#endif


/******************************************************************************
 * FunctionName : init_cling_fota
 * Description  : prepare environment for fota service
 * Parameters   : pbuffer -- The received data from the server

 * Returns      : none
*******************************************************************************/
#if 1
bool ICACHE_FLASH_ATTR
init_cling_log_interface(CLASS(cling_log_interface) *arg)
{
	assert(NULL != arg);
	NEW(arg->p_para_op, cling_para_interface);
	arg->init = init_cling_log_interface;
	arg->de_init = delete_cling_log_interface;
	arg->print_log = print_log;
	arg->read_log =  read_log;

	return TRUE;
}
#endif
/******************************************************************************
 * FunctionName : delete_cling_fota
 * Description  : delete cling related  data buffer
 * Parameters   : CLASS(cling_fota) *arg : object pointer

 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
delete_cling_log_interface(CLASS(cling_log_interface) *arg)
{
	DELETE(arg->p_para_op, cling_para_interface);
	assert(NULL != arg);
	/*malloc corresponed parameter buffer*/
	os_free(arg);
	return TRUE;
}
#endif




