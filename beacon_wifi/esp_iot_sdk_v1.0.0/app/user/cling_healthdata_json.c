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
#include "cling_upload_json.h"
#include "protocol/protocol_cmd.h"
#include "cling_healthdata_json.h"
/*********************************************************************
* MACROS
*/
#define JOSN_MAC_SIZE		 	20
#define JOSN_VERSION_SIZE	 	32
#define JOSN_TIMESTAMP_SIZE	 	16
#define JOSN_SIGNATURE_SIZE   	32
#define JOSN_REQUEST_BODY_SIZE 	32


#define JOSN_CLING_BTID_SIZE	20
#define JOSN_CLING_BTMAC_SIZE	20
#define JOSN_CLING_BTRSSI_SIZE	1


#define CLING_INF_LENTH_LIMITATION  8

#define  CLING_HEALTH_DATA_RANDOMNUM  18459
/*********************************************************************
* TYPEDEFS
*/
/*cling device information related strcutor*/
struct cling_inf {
    char btid[JOSN_CLING_BTID_SIZE]; /*device id*/
    char btmac[JOSN_CLING_BTMAC_SIZE];/*device mac*/
    char btrssi[JOSN_CLING_BTRSSI_SIZE]; /*device rssi signal*/
    struct health_data_inf health;
    char timestamp_ble[JOSN_TIMESTAMP_SIZE];
    struct cling_inf_rev *ptr; /*cling inf passed from uart*/
    struct cling_inf *pnext;	/*next inf to send*/
};
/*upload information struct*/
struct cling_inf_upload_json {

    char udid[JOSN_MAC_SIZE];/**/
    char version[JOSN_VERSION_SIZE];
    uint32 timestamp;
    char timestamp_str[JOSN_TIMESTAMP_SIZE];
    char signature[JOSN_SIGNATURE_SIZE];
    struct cling_inf *pinf;
    uint16 cling_inf_chain_lenth;
    char request_body[JOSN_REQUEST_BODY_SIZE];
    uint16 check_sum;
    char enpryption_text[4+JOSN_TIMESTAMP_SIZE + JOSN_MAC_SIZE + JOSN_VERSION_SIZE + JOSN_TIMESTAMP_SIZE + JOSN_REQUEST_BODY_SIZE + sizeof("&blue=") + JOSN_CLING_BTID_SIZE + JOSN_CLING_BTMAC_SIZE];
};

/*********************************************************************
* GLOBAL VARIABLES
*/


/*********************************************************************
* LOCAL VARIABLES
*/
/*used to share the pointer with json ws*/
LOCAL struct cling_inf *json_cling_inf = NULL;

//LOCAL char *enpryption_text = NULL;
LOCAL struct cling_inf_upload_json *upload_request = NULL;
LOCAL CLASS(hmac_sha1) *signature_obj = NULL;


LOCAL uint32 time_stamp_buffer = 0;


LOCAL bool delete_cling_health_json(CLASS(cling_health_data) *arg);

LOCAL bool get_cling_inf_json(CLASS(cling_health_data) *arg, char *pbuffer);


LOCAL bool get_timestamp_from_json(CLASS(cling_health_data) *arg, uint32 *ptime_stamp, char *js_contex);
LOCAL bool get_timestamp_request_json(CLASS(cling_health_data) *arg, char *pbuffer);

LOCAL bool set_cling_information(CLASS(cling_health_data) *arg,struct health_data_inf health, char *pdev_id, char *pdev_mac, uint32 time_stamp);

LOCAL CLASS(cling_health_data) *this = NULL;




#if 1

/******************************************************************************
 * FunctionName : timer_get
 * Description  : set up the timer request s as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
cling_inf_get(struct jsontree_context *js_ctx)
{


    const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
    static count =0;

    if (os_strncmp(path, "udid", 4) == 0) {

        jsontree_write_string(js_ctx, upload_request->udid);


    } else if (os_strncmp(path, "version", 7) == 0) {

        jsontree_write_string(js_ctx, upload_request->version);

    } else if (os_strncmp(path, "timestamp", 9) == 0) {


        jsontree_write_string(js_ctx, upload_request->timestamp_str);
    } else if (os_strncmp(path, "signature", 9) == 0) {

        /*write versin string to jsonv structor*/
        if (upload_request->enpryption_text[0] == 0) {

            //jsontree_write_string(js_ctx, "8888888888");

        } else { /*if the buffer exists calculate signature*/
            /*create a hmac_sha1 object*/
            char i[22] = {0};
            char m[64] = {0};
            CLING_DEBUG("signature_obj start.....\n");
#if 1
            size_t lenth = 0;
            NEW(signature_obj, hmac_sha1);
            /*set enpryption key*/
            signature_obj->set_key(signature_obj, SHA1_KEY, os_strlen(SHA1_KEY));
            signature_obj->set_text(signature_obj, upload_request->enpryption_text, os_strlen(upload_request->enpryption_text));
            signature_obj->process(signature_obj, i, &lenth);
#endif
#if 0/*for debugging*/
            uint16 temp=0;
            for(temp=0; temp < 20; temp++) {
                CLING_DEBUG("0x%02x\n", i[temp]);

            }
#endif
            CLING_DEBUG("base64_encode_start.....\n");
            /*signalture to base64 conversion peocess */
            signature_obj->base64_encode(signature_obj, i, lenth, m);
            CLING_DEBUG("inf CODE64 = %s\n", m);
            /*delete object*/
            DELETE(signature_obj, hmac_sha1);
            jsontree_write_string(js_ctx, m);
        }

    } else if (os_strncmp(path, "blue", 7) == 0) {

        /*write specific data into bufferS*/
        jsontree_write_string(js_ctx, "");

    }

    return 0;
}


/******************************************************************************
 * FunctionName : timer_get
 * Description  : set up the timer request s as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL int ICACHE_FLASH_ATTR
cling_health_data_get(struct jsontree_context *js_ctx)
{

    const char *path = jsontree_path_name(js_ctx, js_ctx->depth - 1);
    static count =0;

    //CLING_DEBUG("enter cling json get peocess\n");
    if (os_strncmp(path, "MAC", 3) == 0) {
        CLING_DEBUG("enter cling json get mac\n");
        char mac_string[10];
        /*convert mac adress to string*/
        os_sprintf(mac_string, MACSTR, MAC2STR(json_cling_inf->btmac));
        /*write specific data into bufferS*/
        jsontree_write_string(js_ctx, mac_string);

    } else if (os_strncmp(path, "U", 1) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.beacon_timestamp);
    } else if (os_strncmp(path, "T", 1) == 0) {
    	struct rtc_time i;
    	this->base_rtc->to_beijing_tm (upload_request->timestamp, &i);
		/*merge current year month with mday transmitted from ble device*/
		CLING_DEBUG("beijign year =%d , month=%d  date = %d\n",i.tm_year, i.tm_mon,json_cling_inf->health.date);
		json_cling_inf->health.ble_timestamp = this->base_rtc->make_beijing_time (i.tm_year, i.tm_mon,
																				   json_cling_inf->health.date,
																				   0, 0, 0);
        jsontree_write_int(js_ctx, json_cling_inf->health.ble_timestamp);
    } else if (os_strncmp(path, "D", 1) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.total_distance);
    } else if (os_strncmp(path, "WS", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.walk_steps);
    } else if (os_strncmp(path, "RS", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.run_steps);
    } else if (os_strncmp(path, "ST", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.total_steps);
    } else if (os_strncmp(path, "SP", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.sport_time_total);
    } else if (os_strncmp(path, "HR", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.heart_rate);
    } else if (os_strncmp(path, "CF", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.skin_temp);
    } else if (os_strncmp(path, "PT", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.sleep_total);
    } else if (os_strncmp(path, "PE", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.sleep_efficient);
    } else if (os_strncmp(path, "WT", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.wakeup_times);
    } else if (os_strncmp(path, "CT", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.calories_total);
    } else if (os_strncmp(path, "CS", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.calories_sports);
    } else if (os_strncmp(path, "CM", 2) == 0) {
        jsontree_write_int(js_ctx, json_cling_inf->health.calories_mentablisim);
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
cling_inf_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
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



            } else if (jsonparse_strcmp_value(parser, "status_code") == 0) {

            }
        }
    }
    CLING_DEBUG("timer_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)\n");
#endif
    return 0;
}

LOCAL struct jsontree_callback cling_inf_upload_callback =
    JSONTREE_CALLBACK(cling_inf_get, NULL);


LOCAL struct jsontree_callback cling_request_body_callback =
    JSONTREE_CALLBACK(cling_health_data_get, NULL);

JSONTREE_OBJECT(cling_upload_body,
                JSONTREE_PAIR("btid", &cling_inf_upload_callback),
                JSONTREE_PAIR("btmac", &cling_inf_upload_callback),
                JSONTREE_PAIR("btrssi", &cling_inf_upload_callback));

JSONTREE_OBJECT(request_body_data,
                JSONTREE_PAIR("MAC", &cling_request_body_callback),
                JSONTREE_PAIR("U", 	 &cling_request_body_callback),
                JSONTREE_PAIR("T", &cling_request_body_callback),
                JSONTREE_PAIR("D", &cling_request_body_callback),
                JSONTREE_PAIR("WS", &cling_request_body_callback),
                JSONTREE_PAIR("RS", &cling_request_body_callback),
                JSONTREE_PAIR("ST", &cling_request_body_callback),
                JSONTREE_PAIR("SP", &cling_request_body_callback),
                JSONTREE_PAIR("HR", &cling_request_body_callback),
                JSONTREE_PAIR("CF", &cling_request_body_callback),
                JSONTREE_PAIR("PT", &cling_request_body_callback),
                JSONTREE_PAIR("WT", &cling_request_body_callback),
                JSONTREE_PAIR("CT", &cling_request_body_callback),
                JSONTREE_PAIR("CS", &cling_request_body_callback),
                JSONTREE_PAIR("CM", &cling_request_body_callback));

JSONTREE_OBJECT(request_body,
                JSONTREE_PAIR("data", &request_body_data),
                JSONTREE_PAIR("type", &cling_inf_upload_callback),
                JSONTREE_PAIR("userid", &cling_inf_upload_callback),
                JSONTREE_PAIR("caloriesTotal", &cling_inf_upload_callback));

JSONTREE_OBJECT(cling_upload_tree,
                JSONTREE_PAIR("udid", &cling_inf_upload_callback),
                JSONTREE_PAIR("version", &cling_inf_upload_callback),
                JSONTREE_PAIR("timestamp", &cling_inf_upload_callback),
                JSONTREE_PAIR("signature", &cling_inf_upload_callback),
                JSONTREE_PAIR("blue", &cling_upload_body),
                JSONTREE_PAIR("request_body", &cling_inf_upload_callback));

#if 0
LOCAL struct jsontree_callback backtime_callback =
    JSONTREE_CALLBACK(NULL, cling_inf_set);

JSONTREE_OBJECT(backtime_body,
                JSONTREE_PAIR("status_code", &cling_inf_upload_callback),
                JSONTREE_PAIR("backtime", &cling_inf_upload_callback));
JSONTREE_OBJECT(back_status_time_tree,
                JSONTREE_PAIR("backtime_js", &cling_inf_upload_callback));
#endif

/******************************************************************************
 * FunctionName : init_timestamp_request
 * Description  : internal used to initiate time stamp object object
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/


#if 1
bool ICACHE_FLASH_ATTR
init_cling_health_data(CLASS(cling_health_data) *arg)
{

    /*malloc corresponed dparameter buffer*/
    struct cling_inf_upload_json *cling_inf_data = (struct cling_inf_upload_json*)os_malloc(sizeof(struct cling_inf_upload_json));
    assert(NULL != cling_inf_data);

    /*clear cling information chain list ptr*/
    cling_inf_data->pinf= NULL;
    cling_inf_data->cling_inf_chain_lenth = 0;

    upload_request = cling_inf_data;
    /*clear enpryption data buffer*/
    os_memset(cling_inf_data->enpryption_text, 0, sizeof(cling_inf_data->enpryption_text));

    arg->init = init_cling_health_data;

    arg->de_init = delete_cling_health_json;

    arg->get_request_js = get_cling_inf_json;

    arg->set_health_inf = set_cling_information;

    //arg->get_timestamp_js = get_timestamp_from_json;
    arg->user_data = cling_inf_data;
	
	/*construct parent object*/
	NEW(arg->base_rtc, rtc_date);

	this = arg;
}
#endif

/******************************************************************************
 * FunctionName : set_cling_information
 * Description  : internal used to initiate time stamp object object
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
set_cling_information(CLASS(cling_health_data) *arg,struct health_data_inf health, char *pdev_id, char *pdev_mac, uint32 time_stamp)
{
#define CLING_DEVICE_ID_LENTH 	10
#define CLING_DEVICE_MAC_LENTH	6
    assert((NULL != pdev_id) && (NULL != pdev_mac));
    /*malloc corresponed dparameter buffer*/
    struct cling_inf_upload_json *cling_inf_data = ((struct cling_inf_upload_json*)(arg->user_data));
    assert(NULL != cling_inf_data);

    /*assign specific  to user data aerea*/
    cling_inf_data->timestamp = time_stamp;
    /*if chain list is full*/
    if (cling_inf_data->cling_inf_chain_lenth > CLING_INF_LENTH_LIMITATION) {

        return FAIL;

    } else {
        /*get cling inf pointer*/
        struct cling_inf *p = cling_inf_data->pinf;
        /*this is the first data attatched to the list header*/
        if (p == NULL) {
            cling_inf_data->pinf = (struct cling_inf*)os_malloc(sizeof(struct cling_inf));
            p = cling_inf_data->pinf;
        } else {
            /*search to the end of chain list*/
            for(; p->pnext != NULL; p = p->pnext);
            p->pnext =  (struct cling_inf*)os_malloc(sizeof(struct cling_inf));
            p = p->pnext;
        }

        assert(NULL != p);
        /*one more new member has been added to list*/
        cling_inf_data->cling_inf_chain_lenth++;
        p->pnext = NULL;
        p->health = health;
        os_memcpy((char *)(p->btid), pdev_id, CLING_DEVICE_ID_LENTH);
        p->btid[CLING_DEVICE_ID_LENTH] = 0;
        os_memcpy((char *)(p->btmac), pdev_mac, CLING_DEVICE_MAC_LENTH);
        p->btmac[CLING_DEVICE_MAC_LENTH] = 0;
        //p->btrssi[0] = rssi;

    }

    /*clear enpryption data buffer*/
    os_memset(cling_inf_data->enpryption_text, 0, sizeof(cling_inf_data->enpryption_text));

}
#endif

/******************************************************************************
 * FunctionName : delete_cling_inf_json
 * Description  : deinitiate time stamp object
 * Parameters   : pbuffer -- The received data from the server

 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
delete_cling_health_json(CLASS(cling_health_data) *arg)
{
    assert(NULL != arg);
    /*reinitiate global upload json request*/
    upload_request = NULL;

    struct cling_inf_upload_json *cling_inf_data = ((struct cling_inf_upload_json*)(arg->user_data));

	DELETE(arg->base_rtc, rtc_date);
    /*free private data*/
    os_free(cling_inf_data);

    os_free(arg);
	this = NULL;
    return TRUE;

}
#endif


/******************************************************************************
 * FunctionName : cling_json_head_prepared
 * Description  : set up the timer request s as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_json_head_prepared(void)
{
    /*preapre mac related information*/
    {
        char mac_adrr[8] = {0};
        char mac_adrr_str[16] = {0};
        wifi_get_macaddr(STATION_IF , mac_adrr);
        /*convert mac adress to string*/
        os_sprintf(mac_adrr_str, MACSTR, MAC2STR(mac_adrr));

        os_memcpy(upload_request->udid, mac_adrr_str, os_strlen(mac_adrr_str)+1);
    }

    {
        /*used to store sofware version*/
        char iot_version[64] = {0};
        os_sprintf(iot_version,"%s%d.%d.%dt%d(%s)",VERSION_TYPE,IOT_VERSION_MAJOR,\
                   IOT_VERSION_MINOR,IOT_VERSION_REVISION,device_type,UPGRADE_FALG);
        /*copy version to user buffer*/
        os_memcpy(upload_request->version, iot_version, os_strlen(iot_version)+1);

    }
    /*timestamp related string data*/
    {

        os_sprintf(upload_request->timestamp_str, "%d", upload_request->timestamp);

    }

#if 1
    /*assemble enpryption text*/
    os_sprintf(upload_request->enpryption_text, "%s%s%d%d%s%s",  upload_request->version,
               upload_request->udid,upload_request->check_sum, CLING_HEALTH_DATA_RANDOMNUM,
               upload_request->timestamp_str,"&blue=");

    CLING_DEBUG("enpryption_header_text = %s\n", upload_request->enpryption_text);
#endif


}




/******************************************************************************
 * FunctionName : cling_json_health_data_prepared
 * Description  : set up the timer request s as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_json_health_data_prepared( struct cling_inf *p)
{

    /*device rmac related data */
    char mac_adrr[8] = {0};
    char mac_adrr_str[16] = {0};
    /*convert mac adress to string*/
    os_sprintf(mac_adrr_str, MACSTR, MAC2STR(p->btmac));
    /*change mac adress to new format*/
    os_memcpy(p->btmac, mac_adrr_str, os_strlen(mac_adrr_str)+1);
    return;

}

/******************************************************************************
 * FunctionName : cling_json_tail_prepared
 * Description  : set up the timer request s as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_json_tail_prepared(void)
{

    return ;

}
/******************************************************************************
 * FunctionName : check_sum
 * Description  : check sum to validate data
 * Parameters   : unsigned char *addr, int count
 * Returns      : sum
*******************************************************************************/
LOCAL uint16 ICACHE_FLASH_ATTR
check_sum(unsigned char *addr, int count)
{
    /* Compute Internet Checksum for "count" bytes
    * beginning at location "addr".
    */
    uint32 checksum = 0;
    uint32 sum = 0;
#if 1
    while( count > 1 ) {
        /* This is the inner loop */
        sum += * ((uint16*) addr);
        addr += 2;
        count -= 2;
    }

    /* Add left-over byte, if any */
    if( count > 0 )
        sum += * (unsigned char *) addr;

    /* Fold 32-bit sum to 16 bits */
    while (sum>>16)
        sum = (sum & 0xffff) + (sum >> 16);
    checksum = ~sum;
#endif

    return checksum;
}


/******************************************************************************
 * FunctionName : get_cling_inf_json
 * Description  : deinitiate time stamp object
 * Parameters   : pbuffer -- time stamp request json buffer
 *				  arg -- timestamp object  pointer
 * Returns      : none
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
convert_string_to_char_flow(char *psrc)
{
	
		char pbuffer[1024] = {0};
		/*get rid of first "*/
		char *p_new = psrc;
		char *p_last = psrc;
		char *i = pbuffer;
		while ((p_new = (char*)os_strstr(p_new,"\"" )) != NULL){
			os_memcpy(i, p_last, p_new - p_last);
			//CLING_DEBUG("\"position = %d",p_new - p_last);
			i += (p_new - p_last);
			*i++ = '\\';
			p_last = p_new;
			/*move to the next charactor to void search the same tim eby time*/
			p_new++;
			
		}

		os_memcpy(i, p_last, os_strlen(p_last) + 1);
		os_memcpy(psrc , pbuffer, os_strlen(pbuffer) + 1);
	




}



/******************************************************************************
 * FunctionName : get_cling_inf_json
 * Description  : deinitiate time stamp object
 * Parameters   : pbuffer -- time stamp request json buffer
 *				  arg -- timestamp object  pointer
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
get_cling_inf_json(CLASS(cling_health_data) *arg, char *pbuffer)
{

    struct cling_inf_upload_json *cling_inf_data = ((struct cling_inf_upload_json*)(arg->user_data));
    char health_json[1024];
    assert(NULL != arg && NULL != pbuffer);

    struct cling_inf *p = cling_inf_data->pinf;
    char *pscanbuf = health_json;
    uint16 i = 0;

    /*head json prepared*/
    //cling_json_head_prepared();
    /*blue jaosn prepared*/

    os_sprintf(pscanbuf, "[\n");
    pscanbuf += os_strlen(pscanbuf);

    CLING_DEBUG("enter health data json process chain lenth = %d\n", cling_inf_data->cling_inf_chain_lenth);
    if (cling_inf_data->cling_inf_chain_lenth) {
        /*assign cilng inf pointer*/
        for (json_cling_inf = p; json_cling_inf != NULL;) {

            json_ws_send((struct jsontree_value *)&request_body_data, "request_body_data2", pscanbuf);

            char *s = strstr(pscanbuf, "\"CM\":");
            s = strstr(s, "}");
            pscanbuf = s + 1;
            /*move to the end of buffer*/
            os_sprintf(pscanbuf, ",\n");
            pscanbuf += os_strlen(pscanbuf);
            cling_inf_data->cling_inf_chain_lenth --;
            /*assign the poitner need to be released*/
            p = json_cling_inf;
            json_cling_inf = json_cling_inf->pnext;
            os_free(p);

        }

        /*ereove to the last , syntax and add \n sytax*/
        os_sprintf(pscanbuf - 2, "\n]\n");
		//CLING_DEBUG("health_data=%s\n", health_json)
		/*exclude \n syntax from the string*/
        cling_inf_data->check_sum = check_sum(health_json, os_strlen(health_json) - 1);		
		convert_string_to_char_flow(health_json);

       // CLING_DEBUG("health_data=%s\n", health_json);
        /*prepare head section and enprytion text*/
        cling_json_head_prepared();
        pscanbuf = pbuffer;
        /*if there are more than two element in the chain then*/
        JSONTREE_OBJECT(cling_upload_head_tree,
                        JSONTREE_PAIR("udid", &cling_inf_upload_callback),
                        JSONTREE_PAIR("version", &cling_inf_upload_callback),
                        JSONTREE_PAIR("timestamp", &cling_inf_upload_callback),
                        JSONTREE_PAIR("signature", &cling_inf_upload_callback),
                        JSONTREE_PAIR("blue", &cling_inf_upload_callback));

        json_ws_send((struct jsontree_value *)&cling_upload_head_tree, "cling_upload_head_tree", pscanbuf);

        char *s = strstr(pscanbuf, "blue");
        s = strstr(pscanbuf, "}");
        /*pointer to }*/
        pscanbuf = s;
        //pscanbuf += os_strlen(pscanbuf); /*remove } from buffer*/
        os_sprintf(pscanbuf -1, ",\n\"request_body\":\"");
        pscanbuf += os_strlen(pscanbuf);
        /*attach health data to  the json header*/
        os_memcpy(pscanbuf, health_json, os_strlen(health_json) + 1);
        pscanbuf += os_strlen(pscanbuf);
        /*assert body content here*/
        //os_sprintf(pscanbuf , "]\n");
#if 1


        os_sprintf(pscanbuf + os_strlen(pscanbuf)-1, "\"}");
#endif

    }

#if 0
    else if (cling_inf_data->cling_inf_chain_lenth == 1) {
        json_cling_inf = p;
        json_ws_send((struct jsontree_value *)&cling_upload_tree, "cling_upload_tree", pscanbuf);
        cling_inf_data->cling_inf_chain_lenth --;
        os_free(p);
    }
#endif
    /*clean buffer pointer*/
    cling_inf_data->pinf = NULL;
    json_cling_inf = NULL;
    CLING_DEBUG("start json ws write..\n");

   // CLING_DEBUG("json data =%s\n", pbuffer);

 	// CLING_DEBUG("hello mike\n");
    //json_ws_send((struct jsontree_value *)&cling_upload_tree, "Inf_upload_js", pbuffer);

    return TRUE;

}
#endif

/******************************************************************************
 * FunctionName : get_timestamp_from_json
 * Description  : deinitiate time stamp object
 * Parameters   : pbuffer -- time stamp request json buffer
 *				  arg -- timestamp object  pointer
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
get_timestamp_from_json(CLASS(cling_health_data) *arg, uint32 *ptime_stamp, char *pjs_contex)
{
    struct jsontree_context js;

    assert(NULL != arg && NULL != ptime_stamp && NULL != pjs_contex);

    /*parse json string to obtain information of timestamp*/
    //jsontree_setup(&js, (struct jsontree_value *)&back_status_time_tree, json_putchar);
    //json_parse(&js, pjs_contex);

    //*ptime_stamp = time_stamp_buffer;
    return TRUE;

}
#endif




#endif




