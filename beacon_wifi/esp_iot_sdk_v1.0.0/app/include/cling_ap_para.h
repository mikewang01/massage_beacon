#ifndef __CLING_AP_PARA_H__
#define __CLING_AP_PARA_H__
#include "cling_para_interface.h"



DEF_CLASS(cling_ap_para)
	CLASS(cling_para_interface) *op_para_p;
	bool (*init)    	(CLASS(cling_ap_para) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(cling_ap_para) *arg);/*delete timestamp object*/
	bool (*start_smartconfig) 	(CLASS(cling_ap_para) *arg);/*delete timestamp object*/
	bool (*stop_smartconfig)	(CLASS(cling_ap_para) *arg); /*method to get data from http layer*/
	bool (*get_sta_conf)(CLASS(cling_ap_para) *arg, struct station_config *sta_conf); /*register specific task id which is gonna recieve the message from http layer*/
	bool (*register_callback)(CLASS(cling_ap_para) *arg, void (*smart_config_call_back)(void*));
	void *user_data;/*point to user private data*/
END_DEF_CLASS(cling_ap_para)



#endif
