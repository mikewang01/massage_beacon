#ifndef __CLING_HEALTH_JSON_H__
#define __CLING_HEALTH_JSON_H__

#include"cling_rtc.h"
/*health related data*/
struct health_data_inf{
	uint8   date;
	uint16 	total_steps;
	uint16 	walk_steps;
	uint16 	run_steps; 
	uint16 	total_distance;
	uint16  sport_time_total;
	uint16	calories_total;
	uint16	calories_sports;
	uint16	calories_mentablisim;
	uint16	deep_sleep;
	uint16	light_sleep;
	uint16  sleep_total;
	uint16  sleep_efficient;
	uint16  wakeup_times;
	uint16  heart_rate;
	uint16	skin_temp;
	uint32  ble_timestamp;
	uint32  beacon_timestamp;
};

DEF_CLASS(cling_health_data)
	CLASS(rtc_date) 	*base_rtc;
	bool (*init)    	(CLASS(cling_health_data) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(cling_health_data) *arg);/*delete timestamp object*/
	bool (*set_health_inf)(CLASS(cling_health_data) *arg,struct health_data_inf health, char *pdev_id, char *pdev_mac, uint32 time_stamp);
	bool (*get_request_js)	(CLASS(cling_health_data) *arg,  char *pbuffer); /*method to get data from http layer*/
	bool (*get_timestamp_js)(CLASS(cling_health_data) *arg, uint32 *ptime_stamp, char *js_contex); /*register specific task id which is gonna recieve the message from http layer*/
	void *user_data;/*point to user private data*/
END_DEF_CLASS(cling_health_data)



#endif
