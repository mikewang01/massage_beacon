/* 
 * copyright (c) Espressif System 2010
 * 
 */
#ifndef SPI_USER_FLASH_H
#define SPI_USER_FLASH_H
#include "oop_hal.h"
#include "c_types.h"


DEF_CLASS(spi_user_flash)
	bool (*init)    	(CLASS(spi_user_flash) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(spi_user_flash) *arg);/*delete http object*/
	bool (*erase)		(uint32 start_addr, size_t lenth);/*set smart config led state*/
	int (*write)		(uint32 start_addr, char* pbuffer, size_t lenth);/*set smart config led state*/
	int (*read)		(uint32 start_addr, char* pbuffer, size_t lenth);/*set smart config led state*/
END_DEF_CLASS(spi_user_flash)
	

#endif
