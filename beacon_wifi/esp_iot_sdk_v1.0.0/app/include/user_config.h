#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define ESP_PLATFORM        1
#define LEWEI_PLATFORM      0

#define HiCling_PLATFORM    1

#define USE_OPTIMIZE_PRINTF


#if HiCling_PLATFORM


#define LOCATION_DEVICE 			1


#if LOCATION_DEVICE
#define  POSITION_DEVICE   1
#endif


#if   	POSITION_DEVICE

#define LOCATION_DEEP_SLEEP_TIME   10000000

#endif


//#define USE_DNS
#ifdef USE_DNS
#define HICLING_DOMAIN	"m.test.hicling.com"
#define HICLING_TIME_PATH "/time"
#define HICLING_SERVER_PORT 8086
#else
	
/*internet related config*/
#define HICLING_DOMAIN	"101.231.188.66"
#define HICLING_TIME_PATH "/time"
#define HICLING_HEALTH_PATH "/data/day"
#define HICLING_SERVER_PORT 9992//8086
#define INTERNET_TIMEOUT  1000 //ms

#define HICLING_IP		  101,231,188,66

#endif

/*HTTP UART TIME STAMP MANAGE TASK*/
#define  WEB_CLIIENT_TASK_ID  USER_TASK_PRIO_0
#define  MISC_TASK_ID		  USER_TASK_PRIO_1
#define  MASSAGE_TASK_ID	  USER_TASK_PRIO_2

/*encryption key*/
#define 	SHA1_KEY  "5EHdd8_334dyUjjddleqH6YHHm"

#define    RTC_TIMESTAMP_MEMORY_POSITION 64

#endif




//#define SERVER_SSL_ENABLE
//#define CLIENT_SSL_ENABLE
//#define UPGRADE_SSL_ENABLE


/*disable soft ap function*/
//#define SOFTAP_ENCRYPT

#ifdef SOFTAP_ENCRYPT
#define PASSWORD	"123456789"
#endif

#define SENSOR_DEVICE 0
#if SENSOR_DEVICE
#define SENSOR_DEEP_SLEEP

#if HUMITURE_SUB_DEVICE
#define SENSOR_DEEP_SLEEP_TIME    30000000
#elif FLAMMABLE_GAS_SUB_DEVICE
#define SENSOR_DEEP_SLEEP_TIME    60000000
#endif
#endif


#define AP_CACHE           0

#if AP_CACHE
#define AP_CACHE_NUMBER    5
#endif


#endif

