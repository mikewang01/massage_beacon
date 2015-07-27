#ifndef __CLING_UPLOAD_JSON_H__
#define __CLING_UPLOAD_JSON_H__


DEF_CLASS(cling_inf_upload)
	bool (*init)    	(CLASS(cling_inf_upload) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(cling_inf_upload) *arg);/*delete timestamp object*/
	bool (*set_cling_inf)(CLASS(cling_inf_upload) *arg, char *pdev_id, char *pdev_mac, char rssi, uint32 time_stamp);;
	bool (*get_request_js)	(CLASS(cling_inf_upload) *arg,  char *pbuffer); /*method to get data from http layer*/
	bool (*get_timestamp_js)(CLASS(cling_inf_upload) *arg, uint32 *ptime_stamp, char *js_contex); /*register specific task id which is gonna recieve the message from http layer*/
	void *user_data;/*point to user private data*/
END_DEF_CLASS(cling_inf_upload)



#endif
