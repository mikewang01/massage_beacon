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
#include "user_interface.h"
#include "Hmac_sha1.h"	
#include "task_signal.h"

#include "user_iot_version.h"
#include "espconn.h"
#include "user_json.h"
#include "user_esp_platform.h"
#include "user_webclient.h"
#include "user_json.h"
#include "time_sync_json.h"
#include "pthread.h"
/*********************************************************************
* MACROS
*/

/*********************************************************************
* TYPEDEFS
*/

struct timer_req_json{
	char *udid;
	char *version;
	char *timestamp;
	char *signature;
	char *request_body;
};


/*********************************************************************
* GLOBAL VARIABLES
*/
	
	
/*********************************************************************
* LOCAL VARIABLES
*/
LOCAL char *enpryption_text = NULL;
LOCAL struct timer_req_json timer_request = {0};
LOCAL CLASS(hmac_sha1) *signature_obj = NULL;
LOCAL uint32 time_stamp_buffer = 0;


LOCAL bool delete_timestamp_request(CLASS(timestamp_json) *arg);
LOCAL bool get_timestamp_from_json(CLASS(timestamp_json) *arg, uint32 *ptime_stamp, char *js_contex);
LOCAL bool get_timestamp_request_json(CLASS(timestamp_json) *arg, char *pbuffer);


/******************************************************************************
 * FunctionName : os_atoi
 * Description  : vconvert string to inter
 * Parameters   : pstr -- string pointer
 * Returns      : converted integer
*******************************************************************************/

int ICACHE_FLASH_ATTR
os_atoi(char* pstr)  
{  
    int ret_integer = 0;  
    int integer_sign = 1;  
      
 	assert(NULL != pstr);
    /*skip spce charactor*/
    while(*pstr == ' ')  
    {  
        pstr++;  
    }  
      
    /* check if it indicates a negative number or positive one*/  
    if(*pstr == '-')  
    {  
        integer_sign = -1;  
    }  
    if(*pstr == '-' || *pstr == '+')  
    {  
        pstr++;  
    }  
      
    /* 
    * convert every charactor to number consequently
    */  
    while(*pstr >= '0' && *pstr <= '9')  
    {  
        ret_integer = ret_integer * 10 + *pstr - '0';  
        pstr++;  
    }  
    ret_integer = integer_sign * ret_integer;  
     
    return ret_integer;  
}  



#if 1

/******************************************************************************
 * FunctionName : timer_get
 * Description  : set up the timer request s as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
request_json_get(struct jsontree_context *js_ctx)
{
	 

	   const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
	   static count =0;
		
		if (os_strncmp(path, "udid", 4) == 0) {
			
			char mac_adrr[8] = {0};
			char mac_adrr_str[16] = {0};
			wifi_get_macaddr(STATION_IF , mac_adrr);
			/*convert mac adress to string*/
		 	os_sprintf(mac_adrr_str, MACSTR, MAC2STR(mac_adrr));
			/*malloc buffer to udif area*/
			if (timer_request.udid == NULL){
				
				timer_request.udid = (char*)os_malloc(os_strlen(mac_adrr_str)+1);
				assert(timer_request.udid != NULL);

			}else{/*if the buffer exists do nothing*/

			}

			os_memcpy(timer_request.udid, mac_adrr_str, os_strlen(mac_adrr_str)+1);
			jsontree_write_string(js_ctx, mac_adrr_str);
			
		} else if (os_strncmp(path, "version", 7) == 0) {
		
			/*used to store sofware version*/
	   		char iot_version[64] = {0};
			os_sprintf(iot_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
			IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
			/*write versin string to jsonv structor*/
			if (timer_request.version  == NULL){
				
				timer_request.version = (char*)os_malloc(os_strlen(iot_version)+1);
				assert(timer_request.version != NULL);

			}else{/*if the buffer exists do nothing*/

			}
			
			os_memcpy(timer_request.version, iot_version, os_strlen(iot_version)+1);
			jsontree_write_string(js_ctx, iot_version);
			
		} else if (os_strncmp(path, "timestamp", 9) == 0) {

			char *i = "88888888";
			/*write versin string to jsonv structor*/
			if (timer_request.timestamp  == NULL){
				
				timer_request.timestamp = (char*)os_malloc(os_strlen(i)+1);
				assert(timer_request.timestamp != NULL);

			}else{/*if the buffer exists do nothing*/

			}
			
			os_memcpy(timer_request.timestamp, i, os_strlen(i)+1);
			
			jsontree_write_string(js_ctx, i);
		} else if (os_strncmp(path, "signature", 9) == 0) {
		
			/*write versin string to jsonv structor*/
			if (enpryption_text  == NULL){
				
				jsontree_write_string(js_ctx, "8888888888");

			}else{/*if the buffer exists calculate signature*/
					/*create a hmac_sha1 object*/
				char i[22] = {0};
				char m[64] = {0};
				
				size_t lenth = 0;
				NEW(signature_obj, hmac_sha1);
				/*set enpryption key*/
				signature_obj->set_key(signature_obj, SHA1_KEY, os_strlen(SHA1_KEY));
				signature_obj->set_text(signature_obj, enpryption_text, os_strlen(enpryption_text));
				signature_obj->process(signature_obj, i, &lenth);

				#if 0 /*for debugging*/
				uint16 temp=0;
				for(temp=0; temp < 20; temp++){
					CLING_DEBUG("0x%02x\n", i[temp]);

				}
				#endif

				os_free(enpryption_text);
				/*signalture to base64 conversion peocess */
				signature_obj->base64_encode(signature_obj, i, lenth, m);
				CLING_DEBUG("CODE64 = %s\n", m);
				/*delete object*/
				DELETE(signature_obj, hmac_sha1);
				jsontree_write_string(js_ctx, m);
			}
			
		} else if (os_strncmp(path, "request_body", 12) == 0) {

			#define REQUEST_BODY "{\"action\":\"time\"}"
			/*write versin string to json structor*/
			if (timer_request.request_body  == NULL){
	 
				timer_request.request_body = (char*)os_malloc(os_strlen(REQUEST_BODY)+1);
				assert(timer_request.request_body != NULL);

			}else{/*if the buffer exists do nothing*/
				
				
			}
			
			os_memcpy(timer_request.request_body, REQUEST_BODY, os_strlen(REQUEST_BODY)+1);
			jsontree_write_string(js_ctx, REQUEST_BODY);
			
			#if 0
				/*the last string to full fill which mean signature is needed here*/
				struct jsontree_value *uuid_tree_value = find_json_path(js_ctx , "request_body");
				CLING_DEBUG("udid value type =%d\n", ((struct jsontree_string*)uuid_tree_value)->type);
			#endif

			/*allocate specific space for enpryption_text if NULL*/
			if (enpryption_text == NULL){
				/*this is the last element to fullfill  */
				enpryption_text  = (char*)os_malloc(os_strlen(timer_request.udid) 
												+ os_strlen(timer_request.version) 
												+ os_strlen(timer_request.timestamp) 
												+ os_strlen(timer_request.request_body)+1);
				assert(NULL != enpryption_text);
			
			}
			os_sprintf(enpryption_text, "%s%s%s%s",  timer_request.version, timer_request.udid, timer_request.request_body, timer_request.timestamp);
			CLING_DEBUG("enpryption_text = %s\n", enpryption_text);

			
		}
	
    return 0;
}

/******************************************************************************
 * FunctionName : timer_set
 * Description  : parse the timer  parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
request_json_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
#if 1

    uint8 station_tree;
	int type;
    while ((type = jsonparse_next(parser)) != 0) {
        if (type == JSON_TYPE_PAIR_NAME) {
            char buffer[64];
            os_bzero(buffer, 64);

                if (jsonparse_strcmp_value(parser, "backtime") == 0) {
                    jsonparse_next(parser);
                    jsonparse_next(parser);
                    jsonparse_copy_value(parser, buffer, sizeof(buffer));
#if 0
					CLING_DEBUG("timestamp_string = %s\n", buffer);
#endif
					/*convert string to integer*/
					time_stamp_buffer = os_atoi(buffer);
                  

            }else if (jsonparse_strcmp_value(parser, "status_code") == 0) {

			}
        }
    }
	CLING_DEBUG("timer_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)\n");
#endif
	return 0;
}

/******************************************************************************
 * FunctionName : timer_set
 * Description  : parse the timer  parmer as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 *                parser -- A pointer to a JSON parser state
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
request_json_btid_get(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
#if 1 

    uint8 station_tree;
	int type;
    while ((type = jsonparse_next(parser)) != 0) {
        if (type == JSON_TYPE_PAIR_NAME) {
            char buffer[64];
            os_bzero(buffer, 64);

                if (jsonparse_strcmp_value(parser, "backtime") == 0) {
                    jsonparse_next(parser);
                    jsonparse_next(parser);
                    jsonparse_copy_value(parser, buffer, sizeof(buffer));
#if 0
					CLING_DEBUG("timestamp_string = %s\n", buffer);
#endif
					/*convert string to integer*/
					time_stamp_buffer = os_atoi(buffer);
                  

            }else if (jsonparse_strcmp_value(parser, "status_code") == 0) {

			}
        }
    }
	CLING_DEBUG("timer_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)\n");
#endif
	return 0;
}


LOCAL struct jsontree_callback status_callback =
    JSONTREE_CALLBACK(request_json_get, NULL);

JSONTREE_OBJECT(timer_request_body,
				JSONTREE_PAIR("udid", &status_callback),
				JSONTREE_PAIR("version", &status_callback),
				JSONTREE_PAIR("timestamp", &status_callback),
				JSONTREE_PAIR("signature", &status_callback),
				JSONTREE_PAIR("blue", &status_callback),				
				JSONTREE_PAIR("blue", &status_callback),
	 			JSONTREE_PAIR("request_body", &status_callback));				
JSONTREE_OBJECT(timer_request_tree,
                JSONTREE_PAIR("request_js", &timer_request_body));

LOCAL struct jsontree_callback backtime_callback =
    JSONTREE_CALLBACK(NULL, request_json_set);

LOCAL struct jsontree_callback request_json_btid_get_callback =
    JSONTREE_CALLBACK(NULL, request_json_btid_get);


JSONTREE_OBJECT(backtime_body,
				JSONTREE_PAIR("status_code", &backtime_callback),
				JSONTREE_PAIR("backtime", &backtime_callback),
				JSONTREE_PAIR("backtime", &backtime_callback),
				);

JSONTREE_OBJECT(back_json_data,
				JSONTREE_PAIR("userid", &request_json_btid_get_callback),
				JSONTREE_PAIR("btcode", &request_json_btid_get_callback),
				JSONTREE_PAIR("btid", &request_json_btid_get_callback),
				);

JSONTREE_OBJECT(back_status_time_tree,
                JSONTREE_PAIR("backtime_js", &backtime_body));




/******************************************************************************
 * FunctionName : init_timestamp_request
 * Description  : internal used to initiate time stamp object object
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/

#if 1
bool ICACHE_FLASH_ATTR
init_timestamp_json(CLASS(timestamp_json) *arg)
{
	
	/*malloc corresponed dparameter buffer*/

	
	arg->init = init_timestamp_json;
	
	arg->de_init = delete_timestamp_request;

	arg->get_request_js = get_timestamp_request_json;
	
	arg->get_timestamp_js = get_timestamp_from_json;
	
	arg->user_data = NULL;

}
#endif

/******************************************************************************
 * FunctionName : delete_http_service
 * Description  : deinitiate time stamp object
 * Parameters   : pbuffer -- The received data from the server
 
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
delete_timestamp_request(CLASS(timestamp_json) *arg)
{
	assert(NULL != arg);
	
	struct timer_req_json *timestamp_data = ((struct timer_req_json*)(arg->user_data));

	
	/*free private data*/
	os_free(timestamp_data);
	
#if 1	
	/*free requested_body space*/
	
	os_free(timer_request.request_body);
	
	

		/*free signature space*/
	
	os_free(timer_request.signature);
	

	
		/*free signature space*/

	os_free(timer_request.timestamp);
	
	
		/*free udid space*/
	
	os_free(timer_request.udid);
	
	
				/*free udid space*/
	
	os_free(timer_request.version);
	
#endif	
	/*delete object*/
	os_free(arg);
	return TRUE;

}
#endif

/******************************************************************************
 * FunctionName : get_timestamp_request_json
 * Description  : deinitiate time stamp object
 * Parameters   : pbuffer -- time stamp request json buffer
 *				  arg -- timestamp object  pointer
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
get_timestamp_request_json(CLASS(timestamp_json) *arg, char *pbuffer)
{
	#define  END_STR "}\"\n}"
	assert(NULL != arg && NULL != pbuffer);


	json_ws_send((struct jsontree_value *)&timer_request_tree, "request_js", pbuffer);
	json_ws_send((struct jsontree_value *)&timer_request_tree, "request_js", pbuffer);
	/*add a null chractor to the end of json string*/
	char *s = (char*)os_strstr(pbuffer, END_STR);
	/*check if specific string  found or not */
	if (s != NULL){
		*(s + os_strlen(END_STR)) = '\0';
	}else{
		os_printf("string not found\n");
		return FALSE;
	}
	
	return TRUE;

}
#endif

/******************************************************************************
 * FunctionName : get_timestamp_request_json
 * Description  : deinitiate time stamp object
 * Parameters   : pbuffer -- time stamp request json buffer
 *				  arg -- timestamp object  pointer
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
get_timestamp_from_json(CLASS(timestamp_json) *arg, uint32 *ptime_stamp, char *pjs_contex)
{
	struct jsontree_context js;
	
	assert(NULL != arg && NULL != ptime_stamp && NULL != pjs_contex);

	/*parse json string to obbtain information of timestamp*/
	jsontree_setup(&js, (struct jsontree_value *)&back_status_time_tree, json_putchar);
	json_parse(&js, pjs_contex);
	
	*ptime_stamp = time_stamp_buffer;
	return TRUE;

}
#endif




#endif



	
