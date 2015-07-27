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
/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
bool  spi_user_flash_init();
int raspi_read(char *buf, unsigned int from, int len);
int raspi_erase_write(char *buf, unsigned int offs, int count);
unsigned long raspi_init(void);
u16 SPI_Flash_ReadID(void);
int raspi_erase(unsigned int offs, int len);
int raspi_write(char *buf, unsigned int to, int len);
uint32 spi_user_flash_get_id(void);

void user_init(void)
{

		char *a = "00003141592653";
		char temp[20] = {0};
		wifi_set_opmode(STATION_MODE);
        CLING_WIFI_INDICATOR_LED_INSTALL();
        //user_devicefind_init();
        user_task_data_process_init();
        cling_task_misc_process_init();
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


