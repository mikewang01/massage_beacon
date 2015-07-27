#ifndef __BLE_FOTA_H__
#define __BLE_FOTA_H__
#include "oop_hal.h"
#include "upgrade_ble.h"
#include "upgrade.h"
#include "uart_protocol/uart_protocol_mac.h"
#include "driver/spi_user_flash.h"

DEF_CLASS(ble_fota)

	CLASS(uart_implement) *user_flow;
	CLASS(spi_user_flash) *p_user_flash;
	bool (*init)    	(CLASS(ble_fota) *arg); /*initiate http object*/
	bool (*register_upgrade_startup_callback)(CLASS(ble_fota) *arg, void (*upgrade_startup_callback)());
	bool (*register_upgrade_finish_callback)(CLASS(ble_fota) *arg, void (*upgrade_callback)( struct upgrade_server_info));
	bool (*de_init) 	(CLASS(ble_fota) *arg);/*delete http object*/
	bool (*config)		(CLASS(ble_fota) *arg, const char *ssid, const char *password);/*delete  http object*/
	bool (*disconnect)	(CLASS(ble_fota) *arg);/*disconnect from server*/
	bool (*config_server)	(CLASS(ble_fota) *arg, void *pesconn);/*start update fw from serverr*/
	bool (*update_start)	(CLASS(ble_fota) *arg, void *pbuffer);/*start update fw from serverr*/
	bool (*set_user_flow)	(CLASS(ble_fota) *arg, bool (*pfunc)(char*, size_t));/*start update fw from serverr*/ 
	void *user_data;/*point to user private data*/
END_DEF_CLASS(ble_fota)

 

#endif
