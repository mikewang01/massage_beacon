/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_webserver.c
 *
 * Description: The web server mode configration.
 *              Check your hardware connection with the host while use this mode.
 * Modification history:
 *     2014/3/12, v1.0 create this file.
*******************************************************************************/

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "ble_fota.h"
#include "ip_addr.h"
#include "user_esp_platform.h"
#include "espconn.h"
#include "user_iot_version.h"
#include "ble_fota.h"
#include "driver/key.h"
#include "io_assignment.h"
/*********************************************************************
* MACROS
*/
#define FOTA_PACKAGE_SIZE   (2 * 1024)

#define UPGRADE_FRAME  "{\"path\": \"/v1/messages/\", \"method\": \"POST\", \"meta\": {\"Authorization\": \"token %s\"},\
\"get\":{\"action\":\"%s\"},\"body\":{\"pre_rom_version\":\"%s\",\"rom_version\":\"%s\"}}\n"
#define PHEADERBUFFER "Connection: keep-alive\r\n\
	Cache-Control: no-cache\r\n\
	User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
	Accept: */*\r\n\
	Accept-Encoding: gzip,deflate,sdch\r\n\
	Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"
	
#define RPC_RESPONSE_FRAME  "{\"status\": 200, \"nonce\": %d, \"deliver_to_device\": true}\n"
	

#define FOTA_SSL_SERVER_PORT  	443
#define FOTA_NOSSL_SERVER_PORT	8086
/*********************************************************************
* TYPEDEFS
*/
struct fota_paramater{
	struct espconn *pespconn;
	struct upgrade_server_info *pserver;
	void   (*upgrade_startup_callback)();
	void   (*upgrade_accompolished_callback)(struct upgrade_server_info);

};

struct upgrade_param {
    uint32 fw_bin_addr;

    uint8 fw_bin_sec;
    uint8 fw_bin_sec_num;

    uint8 fw_bin_sec_earse;

    uint8 extra;

    uint8 save[4];

    uint8 *buffer;
};	
/*********************************************************************
* GLOBAL VARIABLES
*/
/*set to 1 to open the key trigger function */
#define BLE_FOTA_TRAGGERED_KEY 1
#define FOTA_BLINK_PERIOAD 100	
/*********************************************************************
* LOCAL VARIABLES
*/
LOCAL struct esp_platform_saved_param esp_param;
LOCAL CLASS(ble_fota) *this = NULL;
LOCAL bool cling_fota_start(CLASS(ble_fota) *arg, void* pbuffer);

#if BLE_FOTA_TRAGGERED_KEY
LOCAL struct keys_param keys;
LOCAL struct single_key_param *single_key[CLING_KEY_NUM];
#endif

LOCAL os_timer_t fota_led_timer;
/*USED TO TOGGLE LED STATE*/
LOCAL uint8 link_led_level = 0;

LOCAL struct upgrade_param *upgrade;

/******************************************************************************
 * FunctionName : cling_smartconfig_led_related
 * Description  : control the output of smartconfig led
 * Parameters   : level : output level of pin 
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_fota_led_output(uint8 level)
{
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FOTA_LED_IO_NUM), level);
}


LOCAL void ICACHE_FLASH_ATTR
cling_fota_led_timer_cb(void)
{
    link_led_level = (~link_led_level) & 0x01;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FOTA_LED_IO_NUM), link_led_level);
	
}


LOCAL void ICACHE_FLASH_ATTR
cling_fota_led_init(void)
{

	PIN_FUNC_SELECT(CLING_FOTA_LED_IO_MUX, CLING_FOTA_LED_IO_FUNC);
    os_timer_disarm(&fota_led_timer);
    os_timer_setfn(&fota_led_timer, (os_timer_func_t *)cling_fota_led_timer_cb, NULL);
    os_timer_arm(&fota_led_timer, FOTA_BLINK_PERIOAD, 1);
    link_led_level = 0;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FOTA_LED_IO_NUM), link_led_level);
}

LOCAL void ICACHE_FLASH_ATTR
cling_fota_led_stop(void)
{
	CLING_DEBUG("fota led bink stop\n");
    os_timer_disarm(&fota_led_timer);
    link_led_level = 0;
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FOTA_LED_IO_NUM), link_led_level);
}

#if BLE_FOTA_TRAGGERED_KEY
/******************************************************************************
 * FunctionName : user_plug_long_press
 * Description  : key's long press function, needed to be installed
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
user_plug_long_press(void)
{
	struct fota_paramater *fota_data = (struct fota_paramater*)(this->user_data);

	if (fota_data->upgrade_startup_callback != NULL){
		fota_data->upgrade_startup_callback();
	}
	cling_fota_start(this, NULL);
}

/******************************************************************************
 * FunctionName : start_upgrade_trigger_key_install
 * Description  :  install coressponed trigerr key used to triger a upgrade 
 * Parameters   : none
 * Returns      : none
*******************************************************************************/

LOCAL void ICACHE_FLASH_ATTR
start_upgrade_trigger_key_install(void)
{

     key_add_single(BLE_KEY_UPGRADE_IO_NUM, BLE_KEY_UPGRADE_IO_MUX, BLE_KEY_UPGRADE_IO_FUNC,
                                    user_plug_long_press, NULL);
}
#endif

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_cb
 * Description  : Processing the downloaded   data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
*******************************************************************************/


LOCAL void ICACHE_FLASH_ATTR
cling_platform_upgrade_rsp(void *arg)
{
    struct upgrade_server_info *server = arg;
    struct espconn *pespconn = server->pespconn;
    uint8 devkey[41] = {0};
    uint8 *pbuf = NULL;
    char *action = NULL;
	/*get user data*/
	struct fota_paramater *fota_data = (struct fota_paramater*)(this->user_data);

    os_memcpy(devkey, esp_param.devkey, 40);
	  
    pbuf = (char *)os_zalloc(FOTA_PACKAGE_SIZE);

    if (server->upgrade_flag == true) {
        CLING_DEBUG("user_esp_platform_upgarde_successfully\n");
#if 0
        action = "device_upgrade_success";
        os_sprintf(pbuf, UPGRADE_FRAME, devkey, action, server->pre_version, server->upgrade_version);
        CLING_DEBUG("%s\n",pbuf);

#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif


#endif
        if (pbuf != NULL) {
            os_free(pbuf);
            pbuf = NULL;
        }
    } else {
        CLING_DEBUG("user_esp_platform_upgrade_failed\n");
        action = "device_upgrade_failed";
        os_sprintf(pbuf, UPGRADE_FRAME, devkey, action,server->pre_version, server->upgrade_version);
        CLING_DEBUG("%s\n",pbuf);
#if 0
#ifdef CLIENT_SSL_ENABLE
        espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
        espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif
#endif

        if (pbuf != NULL) {
            os_free(pbuf);
            pbuf = NULL;
        }
    }
	/*after upgrade process finished,stop blinking no matter it is successfullt or not*/
	cling_fota_led_stop();
	/*call back user function when finish  upgade
	maybe successfully or fialed indicated by flag*/
	if (NULL != fota_data->upgrade_accompolished_callback){
		fota_data->upgrade_accompolished_callback(*server);
	}
	
	/*free memory of */
    os_free(server->url);
    server->url = NULL;
    os_free(server);
    server = NULL;
}

/******************************************************************************
 * FunctionName : user_esp_platform_upgrade_begin
 * Description  : Processing the received data from the server
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                server -- upgrade param
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_platform_upgrade_begin(struct espconn *pespconn, struct upgrade_server_info *server)
{
    uint8 user_bin[9] = {0};
    uint8 devkey[41] = {0};
	//uint8 ip[] ={192,168,2,25};
    server->pespconn = pespconn;

    os_memcpy(devkey, esp_param.devkey, 40);
	//os_memcpy(server->ip, ip, 4);
    os_memcpy(server->ip, pespconn->proto.tcp->remote_ip, 4);

#ifdef UPGRADE_SSL_ENABLE
    server->port = FOTA_SSL_SERVER_PORT;
#else
    server->port = FOTA_NOSSL_SERVER_PORT;
#endif

    server->check_cb = cling_platform_upgrade_rsp;
    server->check_times = 120000;

    if (server->url == NULL) {
        server->url = (uint8 *)os_zalloc(512);
    }

    if (system_upgrade_userbin_check() == UPGRADE_FW_BIN1) {
        os_memcpy(user_bin, "user2.bin", 10);
    } else if (system_upgrade_userbin_check() == UPGRADE_FW_BIN2) {
        os_memcpy(user_bin, "user1.bin", 10);
    }
	
	os_sprintf(server->url, "GET /firmware/Files/%s HTTP/1.0\r\nHost: "IPSTR":%d\r\n"PHEADERBUFFER"",
				   user_bin, IP2STR(server->ip),
				  server->port);
    CLING_DEBUG("%s\n",server->url);

	/*start blinking indicating that the foat is in process*/
	cling_fota_led_init();
#ifdef UPGRADE_SSL_ENABLE

    if (ble_upgrade_start_ssl(server) == false) {
#else

    if (ble_upgrade_start(server) == false) {
#endif
        CLING_DEBUG("ble upgrade is already started\n");
    }
}


/******************************************************************************
 * FunctionName : user_platform_rpc_set_rsp
 * Description  : response the message to server to show setting info is received
 * Parameters   : pespconn -- the espconn used to connetion with the host
 *                nonce -- mark the message received from server
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_platform_rpc_set_rsp(struct espconn *pespconn, int nonce)
{
    char *pbuf = (char *)os_zalloc(FOTA_PACKAGE_SIZE);

    if (pespconn == NULL) {
        return;
    }

    os_sprintf(pbuf, RPC_RESPONSE_FRAME, nonce);
    CLING_DEBUG("%s\n", pbuf);
#ifdef CLIENT_SSL_ENABLE
    espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
#else
    espconn_sent(pespconn, pbuf, os_strlen(pbuf));
#endif
    os_free(pbuf);
}

/******************************************************************************
 * FunctionName : user_task_data_process
 * Description  : process the data posted to this task
 * Parameters   : port -- server port
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
cling_fota_start(CLASS(ble_fota) *arg, void* pbuffer){

	  assert(arg != NULL);
	  /*get corresponed parameter buffer*/
	  struct fota_paramater *fota_data = (struct fota_paramater*)(arg->user_data);
	  /*check if parameter valid or not*/
	  assert(NULL != fota_data && NULL != fota_data->pespconn && NULL != fota_data->pserver);
	  struct espconn *pespconn = fota_data->pespconn;
	  struct upgrade_server_info *server = fota_data->pserver;
	  char *pstr;
	  cling_platform_upgrade_begin(pespconn, server);
#if 0
	  /*search for key words*/
	  if ((pstr = (char *)os_strstr(pbuffer, "\"action\": \"sys_upgrade\"")) != NULL) {
            if ((pstr = (char *)os_strstr(pbuffer, "\"version\":")) != NULL) {
                
                int nonce = user_esp_platform_parse_nonce(pbuffer);
                cling_platform_rpc_set_rsp(pespconn, nonce);

  				os_memcpy(server->upgrade_version, pstr + 12, 16);
                server->upgrade_version[15] = '\0';
                os_sprintf(server->pre_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
                    	IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
                cling_platform_upgrade_begin(pespconn, server);
            }
	

	}
#endif
}

/******************************************************************************
 * FunctionName : init_cling_fota
 * Description  : prepare environment for fota service
 * Parameters   : pbuffer -- The received data from the server

 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
cling_config_server(CLASS(ble_fota) *arg, void *pesconn)
{
	assert(NULL != arg && NULL != pesconn);
	/*get corresponed parameter buffer*/
	struct fota_paramater *fota_data = (struct fota_paramater*)(arg->user_data);
	/*incase reconfig the server parameter resulting in memory leak*/
	if (fota_data->pespconn != NULL){
		return TRUE;
	}
	/*this is the fisrt time to configure foat server ip related*/
	struct espconn *p = (struct espconn*)os_malloc(sizeof(struct espconn));
	*p = *((struct espconn*)pesconn);
	fota_data->pespconn = p;
	
#if BLE_FOTA_TRAGGERED_KEY
	/*after successfully config the fota server can the trigger key be installed*/
	start_upgrade_trigger_key_install();
#endif
	return TRUE;
}
#endif

LOCAL bool delete_cling_fota(CLASS(ble_fota) *arg);

/******************************************************************************
 * FunctionName : register_upgrade_finish_callback
 * Description  : register call back functuion when finished sucess or failed
 * Parameters   : 

 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool upgrade_finish_callback(CLASS(ble_fota) *arg, void (*upgrade_accompolished_callback)(struct upgrade_server_info)){
	struct fota_paramater *fota_data = (struct fota_paramater*)(arg->user_data);
	/*check  if user data is valid or not*/
	assert(NULL != fota_data && NULL != upgrade_accompolished_callback);

	fota_data->upgrade_accompolished_callback = upgrade_accompolished_callback;

	return TRUE;

}
#endif
/******************************************************************************
 * FunctionName : register_upgrade_startup_callback
 * Description  : register call back functuion when finished sucess or failed
 * Parameters   : 

 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool upgrade_startup_callback(CLASS(ble_fota) *arg, void (*upgrade_startup_callback)()){
	struct fota_paramater *fota_data = (struct fota_paramater*)(arg->user_data);
	/*check  if user data is valid or not*/
	assert(NULL != fota_data && NULL != upgrade_startup_callback);

	fota_data->upgrade_startup_callback = upgrade_startup_callback;

	return TRUE;

}
#endif
/******************************************************************************
 * FunctionName : set user flow
 * Description  :set fota downstream direction
 * Parameters   : CLASS(ble_fota) *arg : object pointer

 * Returns      : true :success
 * 				: false: failed
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
set_user_stream_flow(CLASS(ble_fota) *arg, bool (*pfunc)(char*, size_t))
{
	assert(NULL != arg && pfunc != NULL);
	return ble_upgrade_set_user_flow(pfunc);
}
#endif
/******************************************************************************
 * FunctionName : set_default_stream_flow
 * Description  :set fota downstream direction
 * Parameters   : CLASS(ble_fota) *arg : object pointer

 * Returns      : true :success
 * 				: false: failed
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
set_default_stream_flow(CLASS(ble_fota) *arg, bool (*pfunc)(char*, size_t))
{
	assert(NULL != arg && pfunc != NULL);
	CLING_DEBUG("set_default_stream_flow\n");
	return ble_upgrade_set_user_flow(pfunc);
}
#endif

/******************************************************************************
 * FunctionName : ble_upgrade_internal
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
ble_upgrade_internal(struct upgrade_param *upgrade, uint8 *data, uint16 len)
{
    bool ret = false;
	CLING_DEBUG("=\n");
    if(data == NULL || len == 0)
    {
    	return true;
    }
    upgrade->buffer = (uint8 *)os_zalloc(len + upgrade->extra);

    os_memcpy(upgrade->buffer, upgrade->save, upgrade->extra);
    os_memcpy(upgrade->buffer + upgrade->extra, data, len);

    len += upgrade->extra;
    upgrade->extra = len & 0x03;
    len -= upgrade->extra;
	CLING_DEBUG(".\n");
    os_memcpy(upgrade->save, upgrade->buffer + len, upgrade->extra);

    do {
        if (upgrade->fw_bin_addr + len >= (upgrade->fw_bin_sec + upgrade->fw_bin_sec_num) * SPI_FLASH_SEC_SIZE) {
            break;
        }

        if (len > SPI_FLASH_SEC_SIZE) {

        } else {
//			os_printf("%x %x\n",upgrade->fw_bin_sec_earse,upgrade->fw_bin_addr);
            /* earse sector, just earse when first enter this zone */
            if (upgrade->fw_bin_sec_earse != (upgrade->fw_bin_addr + len) >> 12) {
                upgrade->fw_bin_sec_earse = (upgrade->fw_bin_addr + len) >> 12;
                this->p_user_flash->erase(upgrade->fw_bin_sec_earse * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE); 
			   //spi_flash_erase_sector(upgrade->fw_bin_sec_earse);
//				os_printf("%x\n",upgrade->fw_bin_sec_earse);
            }
        }
			CLING_DEBUG("*\n");
        if (this->p_user_flash->write(upgrade->fw_bin_addr, upgrade->buffer, len) < 0) {
            break;
        }
        ret = true;
        upgrade->fw_bin_addr += len;
    } while (0);

    os_free(upgrade->buffer);
    upgrade->buffer = NULL;
    return ret;
}

/******************************************************************************
 * FunctionName : ble_upgrade
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
ble_upgrade(char *data, size_t len)
{
    bool ret;

    ret = ble_upgrade_internal(upgrade, data, len);

    return ret;
}

/******************************************************************************
 * FunctionName : system_upgrade_init
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
ble_upgrade_init(void)
{
    uint32 user_bin2_start;
    uint8 flash_buf[4];
    uint8 high_half;

    if (upgrade == NULL) {
        upgrade = (struct upgrade_param *)os_zalloc(sizeof(struct upgrade_param));
    }

    upgrade->fw_bin_sec = 1;
	upgrade->fw_bin_sec_num = 30;
    upgrade->fw_bin_addr = upgrade->fw_bin_sec * SPI_FLASH_SEC_SIZE;
	/*set default stream flow direction to write to flash*/
	set_default_stream_flow(this, ble_upgrade);
}

/******************************************************************************
 * FunctionName : system_upgrade_deinit
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
ble_upgrade_deinit(void)
{
    os_free(upgrade);
    upgrade = NULL;
}


/******************************************************************************
 * FunctionName : init_cling_fota
 * Description  : prepare environment for fota service
 * Parameters   : pbuffer -- The received data from the server

 * Returns      : none
*******************************************************************************/
#if 1
bool ICACHE_FLASH_ATTR
init_ble_fota(CLASS(ble_fota) *arg)
{
	assert(NULL != arg);
	arg->user_flow = NULL;
	/*construct interface inherated by child object*/
	NEW(arg->user_flow, uart_implement);
	//NEW(arg->p_user_flash, spi_user_flash);
	//ble_upgrade_set_user_flow(arg->user_flow->send_data);
	/*malloc corresponed parameter buffer*/
	struct fota_paramater *fota_data = (struct fota_paramater*)os_malloc(sizeof(struct fota_paramater));
	assert(NULL != fota_data);
	/*upgrade related information structor*/
	struct upgrade_server_info *pserver = (struct upgrade_server_info*)os_malloc(sizeof(struct upgrade_server_info));
	assert(NULL != pserver);

	 /*assign private data to user data*/
	fota_data->pespconn = NULL;
	fota_data->pserver  = pserver;
	fota_data->upgrade_accompolished_callback = NULL;
	fota_data->upgrade_startup_callback = NULL;
	//fota_data->upgrade_accompolished_callback = NULL;
	/*clear firmware request url*/
	pserver->url =NULL;
	arg->user_data = (void*)fota_data;
	arg->init = init_ble_fota;
	arg->de_init = delete_cling_fota;
	arg->config_server = cling_config_server;
	arg->update_start = cling_fota_start;
	arg->register_upgrade_finish_callback = upgrade_finish_callback;
	arg->register_upgrade_startup_callback = upgrade_startup_callback;
	//arg->set_user_flow = set_user_stream_flow;
	/*assign object pointer to this ptr*/
	this = arg;
	//ble_upgrade_init();

}
#endif
/******************************************************************************
 * FunctionName : delete_cling_fota
 * Description  : delete cling related  data buffer
 * Parameters   : CLASS(ble_fota) *arg : object pointer

 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
delete_cling_fota(CLASS(ble_fota) *arg)
{
	assert(NULL != arg);
	
	/*malloc corresponed parameter buffer*/
	struct fota_paramater *fota_data = (struct fota_paramater*)(arg->user_data);
	DELETE(arg->user_flow, uart_implement);
	DELETE(arg->p_user_flash, spi_user_flash);
	os_free(fota_data->pespconn);
	os_free(fota_data->pserver);
	os_free(fota_data);
	os_free(arg);
	//ble_upgrade_deinit();
	/*free this ptr*/
	this = NULL;
}
#endif




