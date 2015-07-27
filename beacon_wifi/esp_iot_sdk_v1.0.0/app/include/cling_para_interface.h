#ifndef __CLING_PARA_INTERFACE_H__
#define __CLING_PARA_INTERFACE_H__
#include "oop_hal.h"


DEF_CLASS(cling_para_interface)
	bool (*init)    	(CLASS(cling_para_interface) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(cling_para_interface) *arg);/*delete http object*/
	bool (*save_parameter)(uint32 adress, char* para, uint16 lenth);/*set smart config led state*/
	bool (*load_parameter)(uint32 adress, char para[], uint16 lenth);/*set smart config led state*/
END_DEF_CLASS(cling_para_interface)

 

#endif
