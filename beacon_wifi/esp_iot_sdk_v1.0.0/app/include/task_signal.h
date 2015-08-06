#ifndef __TASK_SIAGNAL_H__
#define __TASK_SIAGNAL_H__

enum msg_types{
	MSG_IPC = 1, /*this is a IPC message categroy*/
	MSG_HTTP,    /*this ia  a  http messgafe category from http layer */
	MSG_TIMER,	/*this ia  a  http messgafe category from 	TIMER*/
	MSG_TIMESTAMP,
	MSG_UART
};

/*ipc event types in category of MSG_HTTP*/
enum  ipc_event{
	 EVENT_INIT  = 0,
	 EVENT_GET_AP_INF,
	 EVENT_AP_CONNECTED, 
	 EVENT_RESET,
	 EVENT_FOTA_START,
	 EVENT_CLING_HEALTH_RECIEVED,
	 EVENT_FOTA_FINISHED,
	 EVENT_OBJECT_RESET
};


/*http event types in category of MSG_HTTP*/
enum  http_event{
	 EVENT_SERVER_CONNECTED = 1,
	 EVENT_SERVER_RECIEVED,
	 EVENT_FOTA_CONFIG,
};

/*ipc event types in category of MSG_HTTP*/
enum  timer_event{
	 EVENT_HTTP_FIFO_CHECK  = 0,
	 EVENT_HEALTH_FIFO_CHECK  = 1
};


/*timestamp event types in category of MSG_HTTP*/
enum  timestamp_event{
	 EVENT_TIMESTAMP_OUTOFDATE  = 0,	
};

/*timestamp event types in category of MSG_HTTP*/
enum  device_msg_uart{
	 EVENT_UART_RX  = 0,
	 EVENT_UART_TX  = 1,
	 EVENT_UART_RX_LOCATION ,
	 EVENT_UART_RX_HEALTH,
	 EVENT_UART_RX_CMD,
};

#define GET_MSG_TYPE(__x)	((__x&0xFFFF0000) >> 16)
#define GET_EVENT_TYPE(__x)	((__x&0x0000FFFF))

#define MESSGE_EVENT(__x, __y) ((__x << 16)|(__y))



/*Mmessage send types macros */
#define  HTTP_EVENT(__x)		(MESSGE_EVENT(MSG_HTTP, __x))
#define  TIMER_EVENT(__x)	(MESSGE_EVENT(MSG_TIMER, __x))
#define  IPC_EVENT(__x)	(MESSGE_EVENT(MSG_IPC, __x))
#define  TIMESTAMP_EVENT(__x)	(MESSGE_EVENT(MSG_TIMESTAMP, __x))
#define  UART_EVENT(__x)	(MESSGE_EVENT(MSG_UART, __x))


#endif


