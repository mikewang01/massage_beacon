/* 
 * copyright (c) Espressif System 2010
 * 
 */

#ifndef SPI_COMM_H
#define SPI_COMM_H
#include "C_types.h"
#include "oop_hal.h"

void spi_send_byes(char *s, uint16 lenth);

/*object prototype declaration*/
DEF_CLASS(spi_comm)
	bool (*init)    	(CLASS(spi_comm) *arg); 			/*initiate uart object*/
	bool (*de_init) 	(CLASS(spi_comm) *arg);				/*delete uart object*/
	bool (*send) 		(CLASS(spi_comm) *arg, char *pbuffer, size_t size);			/*data send   function*/
	bool (*task_register) 	(CLASS(spi_comm) *arg, uint16 task_id);			/*register  object*/
	bool (*recv_callback_register) 	(CLASS(spi_comm) *arg, int (*callback)(uint8, RcvMsgBuff *));/*register  object*/
	void *user_data;/*point to user private data*/
END_DEF_CLASS(spi_comm)



#endif
