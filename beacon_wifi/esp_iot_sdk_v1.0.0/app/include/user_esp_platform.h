#ifndef __USER_PLATFORM_H__
#define __USER_PLATFORM_H__

#include "oop_hal.h"
#include "ip_addr.h"
/*choose memory capacitor used in system*/
#define CLING_PLATFORM_FLASH_8MBIT
//#define CLING_PLATFORM_FLASH_4MBNIT
/* NOTICE---this is for 512KB spi flash.
 * you can change to other sector if you use other size spi flash. */
#ifdef  CLING_PLATFORM_FLASH_8MBIT
#define CLING_PARAM_START_SEC		0x7C
#endif

#ifdef CLING_PLATFORM_FLASH_4MBNIT
#define CLING_PARAM_START_SEC		0x3C
#endif


#define CLING_PARAM_AP   	 1
#define CLING_PARAM_LOG      2
#define ESP_PARAM_FLAG       3

#define packet_size   (2 * 1024)

#define token_size 41

struct esp_platform_saved_param {
    uint8 devkey[40];
    uint8 token[40];
    uint8 activeflag;
    uint8 pad[3];
};

struct esp_platform_sec_flag_param {
    uint8 flag; 
    uint8 pad[3];
};

enum {
    DEVICE_CONNECTING = 40,
    DEVICE_ACTIVE_DONE,
    DEVICE_ACTIVE_FAIL,
    DEVICE_CONNECT_SERVER_FAIL
};



struct dhcp_client_info {
	ip_addr_t ip_addr;
	ip_addr_t netmask;
	ip_addr_t gw;
	uint8 flag;
	uint8 pad[3];
};

enum http_state{
	HTTP_DISCONNECT, /*http disconeccted*/
	HTTP_CONNECTING, /*htp connecting*/
	HTTP_SENDING_DATA, /*http data sending */
	HTTP_SENDDED_SUCCESSFULLY, /*data has been sended succesfully*/
	HTTP_RECIEVED_SUCCESSFULLY, /*data has been sended succesfully*/

};
DEF_CLASS(http_service)
	bool (*init)    	(CLASS(http_service) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(http_service) *arg);/*delete http object*/
	bool (*config)		(CLASS(http_service) *arg, const char *ssid, const char *password);/*delete  http object*/
	bool (*connect_ap)		(CLASS(http_service) *arg);/*connect to server*/
	bool (*disconnect)	(CLASS(http_service) *arg);/*disconnect from server*/
	bool (*sleep)		(CLASS(http_service) *arg);/*notice http service to sleep*/
	bool (*send)		(CLASS(http_service) *arg, uint8 *buffer, size_t size); /*method to send data*/
	bool (*get)			(CLASS(http_service) *arg, uint8 *buffer, size_t size); /*method to get data from http layer*/
	bool (*msgrev_task_register)(CLASS(http_service) *arg, uint16 ip_obtain_task, uint16 http_connect_task, uint16 http_recieved_task); /*register specific task id which is gonna recieve the message from http layer*/
	bool (*set_reset_callback)(CLASS(http_service) *arg, void (*call_back)(uint8 state)); 
	bool (*set_start_connect_to_server_callback)(CLASS(http_service) *arg, void (*call_back)()); 
	bool (*get_current_connection_state)(CLASS(http_service) *arg, uint8 *connection_state);
	bool (*get_http_state)(CLASS(http_service) *arg, uint8 *http_state);
	bool (*get_station_state)(CLASS(http_service) *arg, uint8 *station_state);
	bool (*get_current_connection_mode)(CLASS(http_service) *arg, uint8 *station_mode);
	void *user_data;/*point to user private data*/
END_DEF_CLASS(http_service)


#endif
