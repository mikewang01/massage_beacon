#ifndef __CLING_LOG_INTERFACE_H__
#define __CLING_LOG_INTERFACE_H__
#include "oop_hal.h"
#include "upgrade.h"
#include "cling_para_interface.h"


DEF_CLASS(cling_log_interface)
	CLASS(cling_para_interface) *p_para_op;
	bool (*init)    	(CLASS(cling_log_interface) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(cling_log_interface) *arg);/*delete http object*/
	bool (*print_log)(char * p_log);/*set smart config led state*/
	int  (*read_log)(char * p_log, size_t lenth);/*set smart config led state*/
END_DEF_CLASS(cling_log_interface)


#endif
