/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"


#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"

#include "user_webclient.h"


#include "driver/uart.h"


#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif

#ifdef SERVER_SSL_ENABLE
#include "ssl/cert.h"
#include "ssl/private_key.h"
#else
#ifdef CLIENT_SSL_ENABLE
unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;
#endif
#endif
#include "cling_ap_para.h"
#include "io_assignment.h"
#include "oop_hal.h"
#include "protocol/protocol_cmd.h"
#include "protocol/massage_cmd.h"

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/


bool spi_sync_pin_interrupt_init();
bool spi_comm_init();
void spi_send_byes(char *s, uint16 lenth);
void get_valid_snyc_signal_short(void);
void uart_init(UartBautRate uart0_br, UartBautRate uart1_br);

void user_init(void)
{
		CLASS(cling_protocol) *cling_uart_obj;
		CLASS(massage_protocol) *pp;
		NEW(pp, massage_protocol);
		NEW(cling_uart_obj, cling_protocol);
		//char *a = "00003141592653";
		char temp[20] = {0};
		//char b[64]={0x7d, 0x00, 0x7e,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
		wifi_set_opmode(STATION_MODE);
        CLING_WIFI_INDICATOR_LED_INSTALL();
		uart_init(BIT_RATE_115200,BIT_RATE_115200);
		ETS_UART_INTR_ENABLE();
			//while(1){
		char a[]={1,2,3,4};
		cling_uart_obj->send_data(cling_uart_obj, a , 4);
		pp->send_data(pp, "1" , 1);
	   //	os_delay_us(10000000);
		CLING_DEBUG("DATA SENDED\n");
  	//}
	

#if 0
        //user_devicefind_init();
        spi_sync_pin_interrupt_init();
		spi_comm_init();
       
		uart_init(BIT_RATE_115200,BIT_RATE_115200);
		while(1){
			//spi_send_byes(b, 64);
			get_valid_snyc_signal_short();		
			os_delay_us(1000);
		}
#endif
	//	user_task_data_process_init();
	//	cling_task_misc_process_init();
		
#if 0
		spi_user_flash_init();
		
		//spi_user_flash_init();
		//while(1)
	   // os_printf("flash_id = %04x\n", spi_user_flash_get_id());//raspi_init();
		//	SPI_Flash_ReadID();
		raspi_erase(0, 4*1024);
		raspi_write(a, 0, os_strlen(a));
		
		//raspi_erase_write(a, 0, os_strlen(a));
		    while(1){
				int i =0;
		raspi_read(temp, 0, 19);
		CLING_DEBUG("read from flash =");
		for (i =0 ;i< 10; i++)
		CLING_DEBUG(" 0x%02x", temp[i]);
		CLING_DEBUG("\n");
		
		    	}
#endif
}


