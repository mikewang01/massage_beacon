#ifndef __SYS_TIMESTAMP_MGR_H__
#define __SYS_TIMESTAMP_MGR_H__


enum time_stamp_status{
	TIMESTAMP_SYNCHRONIZED = 0, /*means timestamp has been synchronized in 30m*/
	TIMESTAMP_OUTOFDATE =1,/*indicates timestamp has been out of date*/
};


DEF_CLASS(sys_timestamp)
	bool (*init)    	(CLASS(sys_timestamp) *arg); /*initiate http object*/
	bool (*de_init) 	(CLASS(sys_timestamp) *arg);/*delete http object*/
	bool (*msgrev_register)(CLASS(sys_timestamp) *arg, uint8 task_id);
	bool (*set_current_timestamp) 	(CLASS(sys_timestamp) *arg, uint32  timestamp_counter);/*get current timestamps of system*/
	bool (*get_current_timestamp) 	(CLASS(sys_timestamp) *arg, uint32 *ptimestamp);/*get current timestamps of system*/
	bool (*get_sync_flag) 	(CLASS(sys_timestamp) *arg,  enum time_stamp_status *pstatus);/*get_sync_flag to judge current timestamp is sychronized or not*/
	bool (*load_timestamp_from_flash)(CLASS(sys_timestamp) *arg); /*load time stamp data from flssh stored before last sleep*/
	bool (*save_timestamp_to_flash)(CLASS(sys_timestamp) *arg); /*save the latest timestamps into flash*/
	void *user_data;/*point to user private data*/
END_DEF_CLASS(sys_timestamp)




#endif
