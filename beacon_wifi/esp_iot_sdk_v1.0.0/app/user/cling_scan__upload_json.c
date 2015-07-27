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
#include "uart_protocol/uart_protocol_cmd.h"

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

#define  CLING_SCAN_DATA_RANDOMNUM  18459

/*********************************************************************
* TYPEDEFS
*/
/*cling device information related strcutor*/
struct cling_health_inf {
        char btid[JOSN_CLING_BTID_SIZE]; /*device id*/
        char btmac[JOSN_CLING_BTMAC_SIZE];/*device mac*/
        char btrssi[JOSN_CLING_BTRSSI_SIZE]; /*device rssi signal*/
        struct cling_inf_rev *ptr; /*cling inf passed from uart*/
        struct cling_health_inf *pnext;	/*next inf to send*/
};
/*upload information struct*/
struct cling_inf_upload_json {

        char udid[JOSN_MAC_SIZE];/**/
        char version[JOSN_VERSION_SIZE];
        uint32 timestamp;
        char timestamp_str[JOSN_TIMESTAMP_SIZE];
        char signature[JOSN_SIGNATURE_SIZE];
        struct cling_health_inf *pinf;
        uint16 cling_inf_chain_lenth;
        char request_body[JOSN_REQUEST_BODY_SIZE];
        char enpryption_text[4+JOSN_TIMESTAMP_SIZE + JOSN_MAC_SIZE + JOSN_VERSION_SIZE + JOSN_TIMESTAMP_SIZE + JOSN_REQUEST_BODY_SIZE + sizeof("&blue=") + JOSN_CLING_BTID_SIZE + JOSN_CLING_BTMAC_SIZE];
};

/*********************************************************************
* GLOBAL VARIABLES
*/


/*********************************************************************
* LOCAL VARIABLES
*/
/*used to share the pointer with json ws*/
LOCAL struct cling_health_inf *json_cling_inf = NULL;

//LOCAL char *enpryption_text = NULL;
LOCAL struct cling_inf_upload_json *upload_request = NULL;
LOCAL CLASS(hmac_sha1) *signature_obj = NULL;
LOCAL uint32 time_stamp_buffer = 0;


LOCAL bool delete_cling_inf_json(CLASS(cling_inf_upload) *arg);

LOCAL bool get_cling_inf_json(CLASS(cling_inf_upload) *arg, char *pbuffer);


LOCAL bool get_timestamp_from_json(CLASS(cling_inf_upload) *arg, uint32 *ptime_stamp, char *js_contex);
LOCAL bool get_timestamp_request_json(CLASS(cling_inf_upload) *arg, char *pbuffer);

LOCAL bool set_cling_information(CLASS(cling_inf_upload) *arg, char *pdev_id, char *pdev_mac, char rssi, uint32 time_stamp);



#if 1

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
#if 0					/*for debugging*/
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

        } else if (os_strncmp(path, "btid", 4) == 0) {

                /*write specific data into bufferS*/
                jsontree_write_string(js_ctx, json_cling_inf->btid);

        } else if (os_strncmp(path, "btmac", 5) == 0) {


                jsontree_write_string(js_ctx, json_cling_inf->btmac);

        } else if (os_strncmp(path, "btrssi", 6) == 0) {

                jsontree_write_int(js_ctx, json_cling_inf->btrssi[0]|0xffffff00);

        } else if (os_strncmp(path, "request_body", 12) == 0) {


                jsontree_write_string(js_ctx, upload_request->request_body);

#if 0
                /*the last string to full fill which mean signature is needed here*/
                struct jsontree_value *uuid_tree_value = find_json_path(js_ctx , "request_body");
                CLING_DEBUG("udid value type =%d\n", ((struct jsontree_string*)uuid_tree_value)->type);
#endif


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

JSONTREE_OBJECT(cling_upload_body,
                JSONTREE_PAIR("btid", &cling_inf_upload_callback),
                JSONTREE_PAIR("btmac", &cling_inf_upload_callback),
                JSONTREE_PAIR("btrssi", &cling_inf_upload_callback));

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
init_cling_inf_upload(CLASS(cling_inf_upload) *arg)
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

        arg->init = init_cling_inf_upload;

        arg->de_init = delete_cling_inf_json;

        arg->get_request_js = get_cling_inf_json;

        arg->set_cling_inf = set_cling_information;

        //arg->get_timestamp_js = get_timestamp_from_json;

        arg->user_data = cling_inf_data;

}
#endif

/******************************************************************************
 * FunctionName : set_cling_information
 * Description  : internal used to initiate time stamp object object
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/

#if 1
bool ICACHE_FLASH_ATTR
set_cling_information(CLASS(cling_inf_upload) *arg, char *pdev_id, char *pdev_mac, char rssi, uint32 time_stamp)
{
#define CLING_DEVICE_ID_LENTH 	4
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
                struct cling_health_inf *p = cling_inf_data->pinf;
                /*this is the first data attatched to the list header*/
                if (p == NULL) {
                        cling_inf_data->pinf = (struct cling_health_inf*)os_malloc(sizeof(struct cling_health_inf));
                        p = cling_inf_data->pinf;
                } else {
                        /*search to the end of chain list*/
                        for(; p->pnext != NULL; p = p->pnext);
                        p->pnext =  (struct cling_health_inf*)os_malloc(sizeof(struct cling_health_inf));
                        p = p->pnext;
                }

                assert(NULL != p);
                /*one more new member has been added to list*/
                cling_inf_data->cling_inf_chain_lenth++;
                p->pnext = NULL;
                os_memcpy((char *)(p->btid), pdev_id, CLING_DEVICE_ID_LENTH);
                p->btid[CLING_DEVICE_ID_LENTH] = 0;
                os_memcpy((char *)(p->btmac), pdev_mac, CLING_DEVICE_MAC_LENTH);
                p->btmac[CLING_DEVICE_MAC_LENTH] = 0;
                p->btrssi[0] = rssi;

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
delete_cling_inf_json(CLASS(cling_inf_upload) *arg)
{
        assert(NULL != arg);
        /*reinitiate global upload json request*/
        upload_request = NULL;

        struct cling_inf_upload_json *cling_inf_data = ((struct cling_inf_upload_json*)(arg->user_data));

	   /*free private data*/
        os_free(cling_inf_data);

        os_free(arg);
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
                char time_stamp[16];
                os_sprintf(time_stamp,"%d",upload_request->timestamp);
                os_memcpy(upload_request->timestamp_str, time_stamp, os_strlen(time_stamp)+1);

        }
#if 0
        /*device rmac related data */
        {
                char mac_adrr[8] = {0};
                char mac_adrr_str[16] = {0};
                /*convert mac adress to string*/
                os_sprintf(mac_adrr_str, MACSTR, MAC2STR(upload_request->cl_inf_i.btmac));
                /*change mac adress to new format*/
                os_memcpy(upload_request->cl_inf_i.btmac, mac_adrr_str, os_strlen(mac_adrr_str)+1);
                CLING_DEBUG("cling mac string = %s\n", upload_request->cl_inf_i.btmac);

        }
#endif
        /*enpryption_text fullfill*/
        {
#define REQUEST_BODY "{\"action\":\"time\"}"
#define TEST_ENPRYPTION	"v1.0.2t45772(a)18:fe:34:9b:b4:85{\"action\":\"time\"}88888888&blue=CLING E35931:32:33:34:35:36"
                os_memcpy(upload_request->request_body, REQUEST_BODY, os_strlen(REQUEST_BODY)+1);

                //os_memcpy(upload_request->enpryption_text, TEST_ENPRYPTION, os_strlen(TEST_ENPRYPTION)+1);
#if 1
                /*assemble enpryption text*/
                os_sprintf(upload_request->enpryption_text, "%s%s%d%d%s%s",  upload_request->version,
                           upload_request->udid, check_sum(upload_request->request_body, os_strlen(upload_request->request_body)), CLING_SCAN_DATA_RANDOMNUM,
                           upload_request->timestamp_str,"&blue=");

                CLING_DEBUG("enpryption_header_text = %s\n", upload_request->enpryption_text);
#endif


        }
}




/******************************************************************************
 * FunctionName : cling_json_blue_prepared
 * Description  : set up the timer request s as a JSON format
 * Parameters   : js_ctx -- A pointer to a JSON set up
 * Returns      : result
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_json_blue_prepared( struct cling_health_inf *p, uint16 number)
{

        /*device rmac related data */
        char mac_adrr[8] = {0};
        char mac_adrr_str[16] = {0};
        /*convert mac adress to string*/
        os_sprintf(mac_adrr_str, MACSTR, MAC2STR(p->btmac));
        /*change mac adress to new format*/
        os_memcpy(p->btmac, mac_adrr_str, os_strlen(mac_adrr_str)+1);

        /*only first information is gonna be added to enpryption test*/
        if (number == 1) {
                os_sprintf(upload_request->enpryption_text + os_strlen(upload_request->enpryption_text), "%s%s", p->btid, p->btmac);
                CLING_DEBUG("upload_request->enpryption_text = %s\n", upload_request->enpryption_text);
        }


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
 * FunctionName : get_cling_inf_json
 * Description  : deinitiate time stamp object
 * Parameters   : pbuffer -- time stamp request json buffer
 *				  arg -- timestamp object  pointer
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
get_cling_inf_json(CLASS(cling_inf_upload) *arg, char *pbuffer)
{

        struct cling_inf_upload_json *cling_inf_data = ((struct cling_inf_upload_json*)(arg->user_data));

        assert(NULL != arg && NULL != pbuffer);

        struct cling_health_inf *p = cling_inf_data->pinf;
        char *pscanbuf = pbuffer;
        uint16 i = 0;
        //cling_json_prepared();

        /*head json prepared*/
        cling_json_head_prepared();
        /*blue jaosn prepared*/
        for (json_cling_inf = p; json_cling_inf != NULL; json_cling_inf = json_cling_inf->pnext) {
                i++;
                cling_json_blue_prepared(json_cling_inf, i);
        }

        /*if there are more than two element in the chain then*/
        if (cling_inf_data->cling_inf_chain_lenth) {

                JSONTREE_OBJECT(cling_upload_head_tree,
                                JSONTREE_PAIR("udid", &cling_inf_upload_callback),
                                JSONTREE_PAIR("version", &cling_inf_upload_callback),
                                JSONTREE_PAIR("timestamp", &cling_inf_upload_callback),
                                JSONTREE_PAIR("signature", &cling_inf_upload_callback));

                json_ws_send((struct jsontree_value *)&cling_upload_head_tree, "cling_upload_head_tree", pscanbuf);

                char *s = strstr(pscanbuf, "signature");
                s = strstr(pscanbuf, "}");

                /*pointer to }*/
                pscanbuf = s;
                //pscanbuf += os_strlen(pscanbuf); /*remove } from buffer*/
                os_sprintf(pscanbuf -1, ",\n\"blue\": [\n");
                pscanbuf += os_strlen(pscanbuf);
                /*assign cilng inf pointer*/
                for (json_cling_inf = p; json_cling_inf != NULL;) {

                        json_ws_send((struct jsontree_value *)&cling_upload_body, "cling_upload_body", pscanbuf);

                        char *s = strstr(pscanbuf, "btrssi");
                        s = strstr(pscanbuf, "}");
                        pscanbuf = s + 1;
                        /*move to the end of buffer*/
                        // pscanbuf += os_strlen(pscanbuf);
                        os_sprintf(pscanbuf, ",\n");
                        pscanbuf += os_strlen(pscanbuf);

                        cling_inf_data->cling_inf_chain_lenth --;
                        /*assign the poitner need to be released*/
                        p = json_cling_inf;
                        json_cling_inf = json_cling_inf->pnext;
                        os_free(p);

                }

                os_sprintf(pscanbuf - 2, "]\n");


                os_sprintf(pscanbuf + os_strlen(pscanbuf)-1, ",\n\"request_body\":\"{\\\"action\\\":\\\"time\\\"}\"}");

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
get_timestamp_from_json(CLASS(cling_inf_upload) *arg, uint32 *ptime_stamp, char *pjs_contex)
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




