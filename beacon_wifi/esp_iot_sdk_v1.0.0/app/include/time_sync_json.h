#ifndef __TIME_SYNC_JSON_H__
#define __TIME_SYNC_JSON_H__


DEF_CLASS(timestamp_json)
	bool (*init)    	(CLASS(timestamp_json) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(timestamp_json) *arg);/*delete timestamp object*/
	bool (*get_request_js)	(CLASS(timestamp_json) *arg,  char *pbuffer); /*method to get data from http layer*/
	bool (*get_timestamp_js)(CLASS(timestamp_json) *arg, uint32 *ptime_stamp, char *js_contex); /*register specific task id which is gonna recieve the message from http layer*/
	void *user_data;/*point to user private data*/
END_DEF_CLASS(timestamp_json)



#endif
