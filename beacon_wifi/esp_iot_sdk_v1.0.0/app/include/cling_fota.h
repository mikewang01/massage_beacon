#ifndef __CLING_FOTA_H__
#define __CLING_FOTA_H__
#include "oop_hal.h"
#include "upgrade.h"



DEF_CLASS(cling_fota)
	bool (*init)    	(CLASS(cling_fota) *arg); /*initiate http object*/
	bool (*register_upgrade_startup_callback)(CLASS(cling_fota) *arg, void (*upgrade_startup_callback)());
	bool (*register_upgrade_finish_callback)(CLASS(cling_fota) *arg, void (*upgrade_callback)( struct upgrade_server_info));
	bool (*de_init) 	(CLASS(cling_fota) *arg);/*delete http object*/
	bool (*config)		(CLASS(cling_fota) *arg, const char *ssid, const char *password);/*delete  http object*/
	bool (*disconnect)	(CLASS(cling_fota) *arg);/*disconnect from server*/
	bool (*config_server)	(CLASS(cling_fota) *arg, void *pesconn);/*start update fw from serverr*/
	bool (*update_start)	(CLASS(cling_fota) *arg, void *pbuffer);/*start update fw from serverr*/
	 void *user_data;/*point to user private data*/
END_DEF_CLASS(cling_fota)

 

#endif
