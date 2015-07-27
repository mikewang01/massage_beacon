#ifndef __CLING_FOTA_H__
#define __CLING_FOTA_H__
#include "oop_hal.h"
#include "upgrade.h"


DEF_CLASS(cling_led)
	bool (*init)    	(CLASS(cling_fota) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(cling_fota) *arg);/*delete http object*/
	bool (*config)		(CLASS(cling_fota) *arg, const char *ssid, const char *password);/*delete  http object*/
	bool (*disconnect)	(CLASS(cling_fota) *arg);/*disconnect from server*/
	bool (*config_server)	(CLASS(cling_fota) *arg, void *pesconn);/*start update fw from serverr*/
	bool (*update_start)	(CLASS(cling_fota) *arg, void *pbuffer);/*start update fw from serverr*/
	bool (*set_smartconfig_led)(CLASS(cling_fota) *arg, uint8 mode, uint16 arg);/*set smart config led state*/
	bool (*get_smartconfig_led_status)(CLASS(cling_fota) *arg, uint8 *status);/*set smart config led state*/
	bool (*set_fota_led)(CLASS(cling_fota) *arg, uint8 mode, uint16 arg);/*set smart config led state*/
	bool (*get_fota_led_status)(CLASS(cling_fota) *arg, uint8 *status);/*set smart config led state*/
	
	void *user_data;/*point to user private data*/
END_DEF_CLASS(cling_led)

 

#endif
