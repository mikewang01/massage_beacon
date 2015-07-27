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
#include "cling_para_interface.h"
#include "spi_flash.h"
#include "user_esp_platform.h"
#include "osapi.h"
/*********************************************************************
* MACROS
*/

/*********************************************************************
* TYPEDEFS
*/
	
/*********************************************************************
* GLOBAL VARIABLES
*/


/******************************************************************************
 * FunctionName : load_parameter
 * Description  : load specific para from refeed adress
 * Parameters   : 

 * Returns      : none
*******************************************************************************/
#if 1
bool load_parameter(uint32 adress, char* para, uint16 lenth){

	assert(NULL != para);
	spi_flash_read((CLING_PARAM_START_SEC + adress) * SPI_FLASH_SEC_SIZE, (uint32*)para, lenth);
	return TRUE;

}
#endif
/******************************************************************************
 * FunctionName : save_parameter
 * Description  : save specific para from refeed adress
 * Parameters   : 

 * Returns      : none
*******************************************************************************/
#if 1
bool save_parameter(uint32 adress, char* para, uint16 lenth){

	assert(NULL != para);
	spi_flash_erase_sector(CLING_PARAM_START_SEC + adress);
	spi_flash_write((CLING_PARAM_START_SEC + adress) * SPI_FLASH_SEC_SIZE, (uint32*)para, lenth);
	return TRUE;

}
#endif
LOCAL bool delete_cling_para_interface(CLASS(cling_para_interface) *arg);

/******************************************************************************
 * FunctionName : init_cling_fota
 * Description  : prepare environment for fota service
 * Parameters   : pbuffer -- The received data from the server

 * Returns      : none
*******************************************************************************/
#if 1
bool ICACHE_FLASH_ATTR
init_cling_para_interface(CLASS(cling_para_interface) *arg)
{
	assert(NULL != arg);

	arg->init = init_cling_para_interface;
	arg->de_init = delete_cling_para_interface;
	arg->save_parameter = save_parameter;
	arg->load_parameter = load_parameter;

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
delete_cling_para_interface(CLASS(cling_para_interface) *arg)
{
	assert(NULL != arg);
	/*malloc corresponed parameter buffer*/
	os_free(arg);
	return TRUE;
}
#endif




