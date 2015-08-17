/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"

#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"

#include "user_webclient.h"

#include "driver/uart.h"
#include "protocol/massage_mac.h"
#include "protocol/massage_cmd.h"
#include "task_signal.h"
/*********************************************************************
* MACROS
*/
#define  MASSAGE_FIFO_POLL_PERIOAD 50

//===============================================				
#define zFootAirLed         zDisplay[0].bit1.sb0 //   				脚气压显示
#define zCalfAirLed         zDisplay[0].bit1.sb1 //   				腿气压显示
#define zSeatAirLed         zDisplay[0].bit1.sb2  //  				座气压显示
//#define zWaistAirLed      zDisplay[0].bit1.sb3				
#define zArmAirLed          zDisplay[0].bit1.sb4 //   				手气压显示
				
#define zCalfKneadLed       zDisplay[0].bit1.sb6//				腿部揉动显示
//===============================================				
#define zLegUp              zDisplay[1].bit1.sb0  //  				腿推杆上升
#define zLegDown            zDisplay[1].bit1.sb1 //   				腿推杆下降
#define zLegUpLim           zDisplay[1].bit1.sb2 //   				腿推杆上限位
#define zLegDownLim         zDisplay[1].bit1.sb3 //   				腿推杆下限位
#define zFootUp             zDisplay[1].bit1.sb4 //   				脚底滚动向前
#define zFootDown           zDisplay[1].bit1.sb5 //   				脚底滚动向后
#define zTwoBeep            zDisplay[1].bit1.sb6//				响两声
//===============================================				
#define zBackUp             zDisplay[2].bit1.sb0  //  				背推杆升背
#define zBackDown           zDisplay[2].bit1.sb1  //  				背推杆倒背
#define zBackUpLim          zDisplay[2].bit1.sb2  //  				背推杆升背限位
#define zBackDownLim        zDisplay[2].bit1.sb3   // 				背推杆倒背限位
#define zFootUpLim          zDisplay[2].bit1.sb4   // 				脚底滚动前限位
#define zFootDownLim        zDisplay[2].bit1.sb5   // 				脚底滚动后限位
//=============================================== 				
#define zSHUp               zDisplay[3].bit1.sb0   // 				腿部伸长
#define zSHDown             zDisplay[3].bit1.sb1    //				腿部缩回
#define zSHUpLim            zDisplay[3].bit1.sb2  //  				腿部伸长限位
#define zSHDownLim          zDisplay[3].bit1.sb3   // 				腿部缩回限位
#define zIntensity0         zDisplay[3].bit1.sb4   // 				b6b5b4=0b001强度1档，b6b5b4=0b010强度2档，b6b5b4=0b011强度3档，其他没有用到
#define zIntensity1         zDisplay[3].bit1.sb5    				
#define zIntensity2         zDisplay[3].bit1.sb6    				
//=============================================== 				
#define zReset              zDisplay[4].bit1.sb0    //				主板复位
#define zShape              zDisplay[4].bit1.sb1    //				jShape=1,zShape =0:肩高自动检测开始，jShape=0,zShape =1:肩高自动检测结束，手动调整开始，jShape=0,zShape =0:肩高检测完成
				
				
				
#define zSeatVibLed         zDisplay[4].bit1.sb6	//			座部震动
//===============================================				
#define jWalkUpLim          jDisplay[0].bit1.sb0    //				行走上限位
#define jWalkDownLim        jDisplay[0].bit1.sb1  	//			行走下限位
#define j3DUpLim            jDisplay[0].bit1.sb2    //				3D上限位
#define j3DDownLim          jDisplay[0].bit1.sb3    //				3D下限位
#define jWidth0             jDisplay[0].bit1.sb4    //				b5b4=0b00关，b5b4=0b01宽，b5b4=0b10中，b5b4=0b11窄，
#define jWidth1             jDisplay[0].bit1.sb5    				
#define jKneadFX            jDisplay[0].bit1.sb6    //				正揉＝0，反揉＝1
//=====================================.bit1====				
#define jKnockSpeed0        jDisplay[1].bit1.sb0	//			b2b1b0=0b000强度关，b2b1b0=0b001强度1档，b2b1b0=0b010强度2档，b2b1b0=0b011强度3档，b2b1b0=0b100强度4档，b2b1b0=0b101强度5档，其他没有用到
#define jKnockSpeed1        jDisplay[1].bit1.sb1	//			
#define jKnockSpeed2        jDisplay[1].bit1.sb2	//			
				
#define jKnockMode0         jDisplay[1].bit1.sb4    //				b5b4=0b00敲击，b5b4=0b01拍打，b5b4=0b10指压，b5b4=0b11空
#define jKnockMode1         jDisplay[1].bit1.sb5    				
#define jPressShoulder      jDisplay[1].bit1.sb6    				
//=====================================.bit1===- 				
#define jDotPosition0       jDisplay[2].bit1.sb0    //				b3b2b1b0=0b0000背部最下点。。。b3b2b1b0=0b1001背部最上点
#define jDotPosition1       jDisplay[2].bit1.sb1    				
#define jDotPosition2       jDisplay[2].bit1.sb2    				
#define jDotPosition3       jDisplay[2].bit1.sb3    				
#define j3DPosition0        jDisplay[2].bit1.sb4    //				b6b5b4=0b000三D最后点。。。b6b5b4=0b100三D最前点
#define j3DPosition1        jDisplay[2].bit1.sb5    				
#define j3DPosition2        jDisplay[2].bit1.sb6    				
//=====================================.bit1==== 				
				
				
				
				
#define jKneadSpeed0        jDisplay[3].bit1.sb4    	//			b6b5b4=0b000强度关，b6b5b4=0b001强度1档，b6b5b4=0b010强度2档，b6b5b4=0b011强度3档，b6b5b4=0b100强度4档，b6b5b4=0b101强度5档，其他没有用到
#define jKneadSpeed1        jDisplay[3].bit1.sb5    				
#define jKneadSpeed2        jDisplay[3].bit1.sb6    				
//=====================================.bit1=====				
#define jReset              jDisplay[4].bit1.sb0    //				机芯复位
#define jShape              jDisplay[4].bit1.sb1    //				jShape=1,zShape =0:肩高自动检测开始，jShape=0,zShape =1:肩高自动检测结束，手动调整开始，jShape=0,zShape =0:肩高检测完成
#define jWalkErr            jDisplay[4].bit1.sb2    //				揉捏电机故障
#define jKneadErr           jDisplay[4].bit1.sb3	//			行走电机故障
				
#define jWKerr              jDisplay[4].bit1.sb5    //				行走限位故障
#define j3Derr              jDisplay[4].bit1.sb6    //				3D限位故障
//========================================= 				

/*=====================================================================massage related command==================*/


/*********************************************************************
* TYPEDEFS
*/
union massage_display{
	uint8 tmp;
	struct{
		uint8 sb7:1;
		uint8 sb6:1;
		uint8 sb5:1;
		uint8 sb4:1;
		uint8 sb3:1;
		uint8 sb2:1;
		uint8 sb1:1;
		uint8 sb0:1;
		
	}bit1;
};

struct cling_protocol_data {
    uint8 task_id_reg;
    os_timer_t uart_timer;
};

enum massage_order{
	MASSAGE_POWER_OFF =0X10,
	MASSAGE_POWER_ON,
	MASSAGE_SHUTDOWN_URGENTLY,
	MASSAGE_TIMEUP,
	MASSAGE_AUTOMATICX_OFF,
	MASSAGE_AUTOPROGRAM_OFF,
	MASSAGE_READ_ACC_TIME_OFF,
	MASSAGE_READ_ACC_TIME_ON,
	MASSAGE_TEST_ON,
	MASSAGE_AGING_ON,
	MASSAGE_PAUSE_OFF,
	MASSAGE_PAUSE_ON,
	MASSAGE_EXPERIENCE_OFF,
	MASSAGE_EXPERIENCE_ON,
	MASSAGE_EXPERIENCE_FATIGUE_RECOVERY_SLIM_ON,
	MASSAGE_EXPERIENCE_NECK_SLIM_ON,
	MASSAGE_EXPERIENCE_WAIST_SLIM_ON,
	MASSAGE_EXPERIENCE_STRENCH_SLIM_ON,
	MASSAGE_EXPERIENCE_FATIGUE_RECOVERY_NORMAL_ON,
	MASSAGE_EXPERIENCE_NECK_NORMAL_ON,
	MASSAGE_EXPERIENCE_WAIST_NORMAL_ON,
	MASSAGE_EXPERIENCE_STRENCH_NORMAL_ON,
	MASSAGE_EXPERIENCE_FATIGUE_RECOVERY_FAT_ON,
	MASSAGE_EXPERIENCE_NECK_FAT_ON,
	MASSAGE_EXPERIENCE_WAIST_FAT_ON,
	MASSAGE_EXPERIENCE_STRENCH_FAT_ON,


	
	MASSAGE_KNEADING_RESERSE_ON = 0x31,
	MASSAGE_KNEADING_OFF,
	MASSAGE_KNEADING_FORWARD_ON,
	MASSAGE_BEATING_OFF,
	MASSAGE_BEATING_ON,
	MASSAGE_TAPING_OFF,
	MASSAGE_TAPING_ON,
	MASSAGE_SHIATSU_OFF,
	MASSAGE_SHIATSU_ON,
	MASSAGE_KNEADING_PERSUSSION_OFF,
	MASSAGE_KNEADING_PERSUSSION_ON,
	MASSAGE_BACK_3D_POINT0,
	MASSAGE_BACK_3D_POINT1,
	MASSAGE_BACK_3D_POINT2,
	MASSAGE_BACK_3D_POINT3,
	MASSAGE_BACK_3D_POINT4,
	MASSAGE_WHOLE_BODY,
	MASSAGE_SHOULDER_PART,
	MASSAGE_BACK_PART,
	MASSAGE_WAIST_PART,
	MASSAGE_SPECIFIC_PART,  
	MASSAGE_WANDER_UP,
	MASSAGE_WANDER_OFF,
	MASSAGE_WANDER_DOWN ,
	MASSAGE_3DPLUS_FORWARD,
	MASSAGE_3D_STOP,
	MASSAGE_3DMINUS_STOP,
	MASSAGE_WIDE_SIZE,
	MASSAGE_NORMAL_SIZE,
	MASSAGE_NARROW_SIZE,
	
	MASSAGE_BACK_SPPED_LEVEL1 = 0x50,
	MASSAGE_BACK_SPPED_LEVEL2,
	MASSAGE_BACK_SPPED_LEVEL3,
	MASSAGE_BACK_SPPED_LEVEL4,
	MASSAGE_BACK_SPPED_LEVEL5,
	
	MASSAGE_BACK_HEIGHT_LEVEL0,
	MASSAGE_BACK_HEIGHT_LEVEL1,
	MASSAGE_BACK_HEIGHT_LEVEL2,
	MASSAGE_BACK_HEIGHT_LEVEL3,
	MASSAGE_BACK_HEIGHT_LEVEL4,
	MASSAGE_BACK_HEIGHT_LEVEL5,
	MASSAGE_BACK_HEIGHT_LEVEL6,
	MASSAGE_BACK_HEIGHT_LEVEL7,
	MASSAGE_BACK_HEIGHT_LEVEL8,
	MASSAGE_BACK_HEIGHT_LEVEL9,

	
	MASSAGE_PRESSURE_HANDPART_OFF = 0x64,
	MASSAGE_PRESSURE_HANDPART_ON,
	MASSAGE_PRESSURE_WAISTPART_OFF,
	MASSAGE_PRESSURE_WAISTPART_ON,
	MASSAGE_PRESSURE_HIPPART_OFF,
	MASSAGE_PRESSURE_HIPPART_ON,
	MASSAGE_PRESSURE_LEGPART_OFF,
	MASSAGE_PRESSURE_LEGPART_ON,
	MASSAGE_PRESSURE_FOOTPART_OFF,
	MASSAGE_PRESSURE_FOOTPART_ON,
	MASSAGE_PRESSURE_LEGKNEEDING_OFF,
	MASSAGE_PRESSURE_LEGKNEEDING_ON,
	MASSAGE_PRESSURE_LEVEL1,
	MASSAGE_PRESSURE_LEVEL2,
	MASSAGE_PRESSURE_LEVEL3,
	MASSAGE_PRESSURE_LEVEL4,
	MASSAGE_PRESSURE_LEVEL5,
	
	MASSAGE_ELECTRONIC_POLE_LIFT = 0x7d,
	MASSAGE_ELECTRONIC_POLE_STOP,
	MASSAGE_ELECTRONIC_POLE_BUTTOM,
	MASSAGE_ELECTRONIC_POLE_LEG_ASEND,
	MASSAGE_ELECTRONIC_POLE_LEG_STOP,
	MASSAGE_ELECTRONIC_POLE_LEG_DESEND,
	MASSAGE_ELECTRONIC_POLE_BACK_ASEND,
	MASSAGE_ELECTRONIC_POLE_BACK_STOP,
	MASSAGE_ELECTRONIC_POLE_BACK_DESEND,
	MASSAGE_ELECTRONIC_POLE_LEG_STRETCH,
	MASSAGE_ELECTRONIC_POLE_LEG_ADJ_STOP,
	MASSAGE_ELECTRONIC_POLE_LEG_SHORTER,
	MASSAGE_ELECTRONIC_POLE_STRETCH_ON,
	MASSAGE_ELECTRONIC_POLE_STRETCH_OFF,

	
	MASSAGE_ELECTRONIC_POLE_ZERO_POWER_OFF = 0x8c,
	MASSAGE_ELECTRONIC_POLE_ZERO_POWER_ON,
	
	MASSAGE_VABRATING_MOTOR_OFF = 0x90,
	MASSAGE_VABRATING_MOTOR_ON_LEVEL1,
	MASSAGE_VABRATING_MOTOR_ON_LEVEL2,
	MASSAGE_VABRATING_MOTOR_ON_LEVEL3,
	MASSAGE_VABRATING_MOTOR_ON_LEVEL4,
	MASSAGE_VABRATING_MOTOR_ON_LEVEL5,

	MASSAGE_READ_LOWER_FIVE_STATUS_BYTES = 0XA5,
	MASSAGE_READ_HIGHER_FIVE_STATUS_BYTES = 0XA5,
	
	MASSAGE_AUDIO_INTERACTION_OFF = 0xB0,
	MASSAGE_AUDIO_INTERACTION_ON,
};

//==============================================================
enum{
	cancel_massage_am = 0,
	whole_body_massage_am,
	neck_part_massage_am,
	wrist_part_massage_am,
	strentch_massage_am,
	recover_massage_am,	
};


enum{
	cancel_kneading = 0,
	clock_wise_kneading,
	counter_clock_wise_kneading,
};

enum {
	cancel_knocking = 0,
	knocking_mode_knocking,
	slapping_mode_knocking,
};


enum{
	cancel_shiatsu = 0,
	start_shiatsu
};

enum{
	cancel_shoulder_adj = 0,
	start_shoulder_up,
	start_shoulder_down
	
};

enum{
	cancel_3d_adj = 0,
	start_3d_up,
	start_3d_down
};

enum{
	cancel_seat_vibrate = 0,
	seat_vibrate_level1,
	seat_vibrate_level2,
};

enum{
	cancel_mr = 0,
	mr_whole_body,
	mr_part_mode,
	mr_specific_point_mode
};

enum{
	cancel_massage_arm = 0,
	start_massage_arm
};

enum{
	cancel_massage_seat_qn = 0,
	start_massage_sear_qn
};


enum{
	cancel_massage_leg_qn = 0,
	start_massage_leg_qn
};

enum{
	cancel_massage_foot_qn = 0,
	start_massage_foot_qn
};


enum{
	cancel_massage_lrn = 0,
	start_massage_lrn_clockwise,
	start_massage_lrn_counterclockwise,
};


#pragma pack(push) 
#pragma pack(1)

/*send data package*/
struct a8a_order_package_send{
/*
	union{
		uint8 lead_uint8;
	}byte0;
*/
	union{
		
		struct{
			/*low bits*/			
			uint8 am:3;
			uint8 mmzy:1;
			uint8 mmqd:2;
			uint8 mmrn:2;
			/*high bits*/
			
		}order;
		uint8 byte;
	} byte1;
	
	union{
		struct{
			uint8 lss:2;	
			uint8 lud:2;
			uint8 bud:2;
			uint8 lbud:2;
		}order;
		uint8 byte;
	}byte2;

	union {
		struct{			
			uint8 slf:2;	
			uint8 mr:2;
			uint8 hud:2;
			uint8 three_d:2;			
		}order;
		uint8 byte;
	}byte3;

	union {
		struct{
			uint8 e_heat:1;
			uint8 nothing:1;
			uint8 f_qn:1;			
			uint8 l_qn:1;
			uint8 lrn:2;
			uint8 s_qn:1;
			uint8 arm:1;
		}order;
		uint8 byte;
	}byte4;	
	
	union {
		struct{
			uint8 sq:3;
			uint8 sr:3;
			uint8 s_fr:2;
	 }order;
		uint8 byte;
	}byte5;	

	union {
		struct{
			uint8 wh:2;
			uint8 sh:2;
			uint8 pause:1;
			uint8 pwr:1;
			uint8 res:1;
			uint8 zero:1;
	 
	 	}order;
		uint8 byte;
	}byte6;

	union{
		struct{
			uint8 res7:1;
			uint8 res6:1;
			uint8 res5:1;
			uint8 res4:1;
			uint8 res3:1;
			uint8 res2:1;
			uint8 res1:2;
		}order;
		uint8 byte;
	}byte7;

	
/*	
	union{
		uint8 checksum;
	}byte8;
*/
};
#pragma pack(pop) 

struct a8a_order_package_rec{
	union{
		char lead_char;
	}byte0;

	
	union{
		
		struct{
			char mmrn:2;
			char mmqd:2;
			char mmzy:1;
			char am:3;
			
		}order;
		char byte;
	} byte1;
	
	union{
		struct{
			char lbud:2;
			char bud:2;
			char lud:2;
			char lss:2;	
		}order;
		char byte;
	}byte2;

	union {
		struct{
			char three_d:2;
			char hud:2;
			char mr:2;
			char slf:2;	
		}order;
		char byte;
	}byte3;

	union {
		struct{
			char arm:2;
			char s_qn:2;
			char lrn:2;
			char l_qn:1;
			char f_qn:1;			
			char nothing:1;
			char e_heat:1;
		}order;
		char byte;
	}byte4;	
	
	union {
		struct{
			char s_fr:2;
			char sr:3;
			char sq:3;
		}order;
		char byte;
	}byte5;	

	union {
		struct{
			char zero:1;
			char res:1;
			char pwr:1;
			char pause:1;
			char sh:2;
			char wh:2;
		}order;
		char byte;
	}byte6;

	union{
		struct{
			char res1:1;
			char q6:1;
			char q5:1;
			char q4:1;
			char q3:1;
			char q2:1;
			char q1:1;
			char q0:1;
		}order;
		char byte;
	}byte7;

	union{
		struct{
			char e_st:1;
			char q13:1;
			char q12:1;
			char q11:1;
			char q10:1;
			char q9:1;
			char q8:1;
			char q7:1;
		}order;
		char byte;
	}byte8;
	
	union{
		struct{
			char amds:1;
			char q13:1;
			char q12:1;
			char q11:1;
			char q10:1;
			char q9:1;
			char q8:1;
			char q7:1;
		}order;
		char byte;
	}byte9;
	
	union{
		struct{
			char e7:1;
			char e6:1;
			char e5:1;
			char e4:1;
			char e3:1;
			char e2:1;
			char e1:1;
			char e0:1;
		}order;
		char byte;
	}byte10;	
		
	union{
		char checksum;
	}byte11;
};




//==========================================================



/*********************************************************************
* GLOBAL VARIABLES
*/


/*********************************************************************
* LOCAL VARIABLES
*/
LOCAL CLASS(massage_protocol) *this = NULL;
LOCAL union massage_display zDisplay[5]; /*low five bytes buffer*/
LOCAL union massage_display jDisplay[5]; /*high five bytes buffer*/
/*********************************************************************
* EXTERNAL VARIABLES
*/


/*********************************************************************
* FUNCTIONS
*/
LOCAL bool delete_massage_protocol(CLASS(massage_protocol) *arg);
LOCAL bool cling_data_recieved_poll(CLASS(massage_protocol) *arg);
LOCAL bool cling_uart_taskid_register(CLASS(massage_protocol) *arg, uint16 task_id);
LOCAL bool cling_data_send(CLASS(massage_protocol) *arg, char *pinf, size_t lenth);
LOCAL bool enable_recieving(CLASS(massage_protocol) *arg);
LOCAL bool disable_recieving(CLASS(massage_protocol) *arg);
LOCAL void cling_cmd_rev_callback(char cmd);






/******************************************************************************
 * FunctionName : fatigue_mode_on
 * Description  : send power on command to massage chair
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
#if 1
bool ICACHE_FLASH_ATTR
fatigue_mode_on(CLASS(massage_protocol) *arg)
{
	assert(arg != NULL);
	uint8 command = MASSAGE_EXPERIENCE_FATIGUE_RECOVERY_NORMAL_ON;
#ifdef SONGYAN_A8L
	arg->send_data(arg, &command, 1);
#else
#ifdef SONGYAN_A8A
	struct a8a_order_package_send i;
	os_memset(&i,0, sizeof(struct a8a_order_package_send));
	i.byte1.order.mmrn = 2;
	i.byte6.order.pwr = 1;
	arg->send_data(arg, (char*)&i, sizeof(struct a8a_order_package_send));
#endif
#endif
	return TRUE;
}
#endif
/******************************************************************************
 * FunctionName : fatigue_mode_on
 * Description  : send power on command to massage chair
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
#if 1
bool ICACHE_FLASH_ATTR
fatigue_mode_off(CLASS(massage_protocol) *arg)
{
	assert(arg != NULL);
	uint8 command = MASSAGE_EXPERIENCE_OFF;
#ifdef SONGYAN_A8L
	arg->send_data(arg, &command, 1);
#else
#ifdef SONGYAN_A8A
	struct a8a_order_package_send i;
	os_memset(&i,0, sizeof(struct a8a_order_package_send));
	i.byte1.order.mmrn = 2;
	i.byte6.order.pwr = 1;
	arg->send_data(arg, (char*)&i, sizeof(struct a8a_order_package_send));
#endif
#endif
	return TRUE;
}
#endif



/******************************************************************************
 * FunctionName : power_on
 * Description  : send power on command to massage chair
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
#if 1
bool ICACHE_FLASH_ATTR
power_on(CLASS(massage_protocol) *arg)
{
	assert(arg != NULL);
	uint8 command = MASSAGE_POWER_ON;
#ifdef SONGYAN_A8L
	arg->send_data(arg, &command, 1);
#else
#ifdef SONGYAN_A8A
	struct a8a_order_package_send i;
	os_memset(&i,0, sizeof(struct a8a_order_package_send));
	i.byte1.order.mmrn = 2;
	i.byte6.order.pwr = 1;
	arg->send_data(arg, (char*)&i, sizeof(struct a8a_order_package_send));
#endif
#endif
	return TRUE;
}
#endif
/******************************************************************************
 * FunctionName : power_off
 * Description  : send power off command to massage chair
 * Parameters   : arg: object pointer
 * Returns      : true: sucessfully false: failed
*******************************************************************************/
#if 1

bool ICACHE_FLASH_ATTR
power_off(CLASS(massage_protocol) *arg)
{
	assert(arg != NULL);
	struct a8a_order_package_send i;
	uint8 command = MASSAGE_POWER_OFF;
#ifdef SONGYAN_A8L
		arg->send_data(arg, &command, 1);
#else
#ifdef SONGYAN_A8A
		os_memset(&i,0, sizeof(struct a8a_order_package_send));
		i.byte1.order.mmrn = 1;
		i.byte6.order.pwr = 0;
		arg->send_data(arg, (char*)&i, sizeof(struct a8a_order_package_send));
#endif
#endif
	return TRUE;
}
#endif

/******************************************************************************
 * FunctionName : send_heart_rate
 * Description  : send heart rate levle to chait
 * Parameters   : arg: object pointer
 * Returns      : true: sucessfully false: failed
*******************************************************************************/
#if 1

bool ICACHE_FLASH_ATTR
send_heart_rate_level(CLASS(massage_protocol) *arg, uint8 heartrate_level)
{
	assert(arg != NULL);

	uint8 command = heartrate_level;
#ifdef SONGYAN_A8L
		arg->send_data(arg, &command, 1);
#else
#ifdef SONGYAN_A8A
		struct a8a_order_package_send i;
		os_memset(&i,0, sizeof(struct a8a_order_package_send));
		i.byte1.order.mmrn = 1;
		i.byte6.order.pwr = 0;
		arg->send_data(arg, (char*)&i, sizeof(struct a8a_order_package_send));
#endif
#endif
	return TRUE;
}
#endif

/******************************************************************************
 * FunctionName : enable_audio_interaction
 * Description  : enbale massage chair audio interaction function
 * Parameters   : arg -- object pointer
 * Returns      : true : sucess false: failed
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
enable_audio_interaction(CLASS(massage_protocol) *arg)
{
   	assert(arg != NULL);
	uint8 command = MASSAGE_AUDIO_INTERACTION_ON;
#ifdef SONGYAN_A8L
		arg->send_data(arg, &command, 1);
#else
#ifdef SONGYAN_A8A
		struct a8a_order_package_send i;
		os_memset(&i,0, sizeof(struct a8a_order_package_send));
		i.byte1.order.mmrn = 1;
		i.byte6.order.pwr = 0;
		arg->send_data(arg, (char*)&i, sizeof(struct a8a_order_package_send));
#endif
#endif
	return TRUE;

}
#endif

/******************************************************************************
 * FunctionName : disable_audio_interaction
 * Description  : disbale massage chair audio interaction function
 * Parameters   : arg -- object pointer
 * Returns      : true : sucess false: failed
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
disable_audio_interaction(CLASS(massage_protocol) *arg)
{
   	assert(arg != NULL);
	uint8 command = MASSAGE_AUDIO_INTERACTION_OFF;
#ifdef SONGYAN_A8L
		arg->send_data(arg, &command, 1);
#else
#ifdef SONGYAN_A8A
		struct a8a_order_package_send i;
		os_memset(&i,0, sizeof(struct a8a_order_package_send));
		i.byte1.order.mmrn = 1;
		i.byte6.order.pwr = 0;
		arg->send_data(arg, (char*)&i, sizeof(struct a8a_order_package_send));
#endif
#endif
	return TRUE;

}
#endif



/******************************************************************************
 * FunctionName : init_cling_uart
 * Description	: uart cling message receive object init
 * Parameters	: none
 * Returns		: none
*******************************************************************************/

bool ICACHE_FLASH_ATTR
init_massage_protocol(CLASS(massage_protocol) *arg)
{


    assert(NULL != arg);
    if (this == NULL) {
        struct cling_protocol_data *private_data = (struct cling_protocol_data*)os_malloc(sizeof(struct cling_protocol_data));
        assert(NULL != private_data);
        /*ini tprivate data*/
        private_data->task_id_reg = USER_TASK_PRIO_MAX + 1;
        arg->user_data = private_data;

        /*initiate cilng command receive object*/
        arg->init    = init_massage_protocol;
        arg->de_init = delete_massage_protocol;
        arg->task_register = cling_uart_taskid_register;

        arg->send_data = cling_data_send;

		arg->send_heart_rate_level = send_heart_rate_level;
		
		arg->fatigue_mode_on = fatigue_mode_on;
		arg->fatigue_mode_off = fatigue_mode_off;
		/*massage instruction communication method*/
		arg->power_on = power_on;
		arg->power_off = power_off;
		
        arg->enable_recieving = enable_recieving;
        arg->disable_recieving = disable_recieving;
		
	 	arg->enable_audio_interaction = enable_audio_interaction;
		arg->disable_audio_interaction = disable_audio_interaction;
        //uart_init(BIT_RATE_115200, BIT_RATE_115200);

        set_massage_recieved_cmd_call_back(cling_cmd_rev_callback);
        /*restart flag updating progress*/
        os_timer_disarm(&(private_data->uart_timer));
        os_timer_setfn(&(private_data->uart_timer), (os_timer_func_t *)cling_data_recieved_poll, arg);
        os_timer_arm(&(private_data->uart_timer), MASSAGE_FIFO_POLL_PERIOAD, 1);
        /*initiate mac layer and physical layer related */
       // init_protocol_mac_layer();
        this = arg;
    } else {

    }
    return TRUE;
}



/******************************************************************************
 * FunctionName : enable_recieving
 * Description  : enable receving
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
enable_recieving(CLASS(massage_protocol) *arg)
{
    /*check object parameter*/
    assert(NULL != arg);
    /*enable receive interrupt enable*/
    //ETS_UART_INTR_ENABLE();

    return TRUE;

}
#endif
/******************************************************************************
 * FunctionName : disable_recieving
 * Description  : dsible uart receving
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
disable_recieving(CLASS(massage_protocol) *arg)
{
    /*check object parameter*/
    assert(NULL != arg);
    /*enable receive interrupt enable*/
    //ETS_UART_INTR_DISABLE();

    return TRUE;

}
#endif

/******************************************************************************
 * FunctionName : delete_uart
 * Description  : internal used to delete uart device
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
delete_massage_protocol(CLASS(massage_protocol) *arg)
{
    /*check object parameter*/
    assert(NULL != arg);


    /*malloc corresponed dparameter buffer*/
    struct cling_protocol_data *private_data = (struct cling_protocol_data*)(arg->user_data);
    //  ETS_UART_INTR_DISABLE();
    /*disarm poll timer*/
    os_timer_disarm(&(private_data->uart_timer));
    os_free(private_data);
    os_free(arg);
    this = NULL;
    return TRUE;

}
#endif


/******************************************************************************
 * FunctionName : uart_taskid_register
 * Description  : internal used to register specific taskdi passed over here
 * Parameters   : arg -- object pointer
 *				  task_id -- task-id who gonna recieve message
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
cling_uart_taskid_register(CLASS(massage_protocol) *arg, uint16 task_id)
{
    /*check object parameter*/
    assert(NULL != arg);

    /*malloc corresponed dparameter buffer*/
    struct cling_protocol_data *callback_data = (struct cling_protocol_data*)(arg->user_data);

    assert(NULL != callback_data);

    /*if taskid passed here is valid*/
    if (IS_TASK_VALID(task_id)) {
        /*register taskid here*/
        callback_data->task_id_reg = task_id;
    }

    return TRUE;


}
#endif

/******************************************************************************
 * FunctionName : cling_cmd_rev_callback
 * Description  : call funcion is prefered for real time purposre
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
cling_cmd_rev_callback(char cmd)
{
    CLING_DEBUG("cmd revecied\n");
    /*malloc corresponed dparameter buffer*/
    struct cling_protocol_data *private_data = (struct cling_protocol_data*)(this->user_data);

    if (IS_TASK_VALID(private_data->task_id_reg)) {
        system_os_post(private_data->task_id_reg, UART_EVENT(EVENT_UART_RX_CMD), cmd);
        /*buffer gonna be freed in user task*/
    }
}

/******************************************************************************
 * FunctionName : cling_data_recieved_poll
 * Description  : uart received poll implementation to get cling imformation
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
#if 1

LOCAL bool ICACHE_FLASH_ATTR
cling_data_recieved_poll(CLASS(massage_protocol) *arg)
{

    struct massage_mac_layer_payload_rev *i = NULL;
    /*malloc corresponed dparameter buffer*/
    struct cling_protocol_data *private_data = (struct cling_protocol_data*)(arg->user_data);
    assert(private_data);

    /*if obtain a message from fifo succesfully,return true*/
    if (massage_obtain_payload_from_revlist(&i) == TRUE) {
		CLING_DEBUG("DATA AOTAINED FROM FIFO \r\n");
    }
    /*release data object transmitted from mac layer*/
    os_free(i);



}
#endif

/******************************************************************************
 * FunctionName : cling_data_send
 * Description  : uart received poll implementation to get cling imformation
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
#if 1

LOCAL bool ICACHE_FLASH_ATTR
cling_data_send(CLASS(massage_protocol) *arg, char *pinf, size_t lenth)
{

    /*malloc corresponed dparameter buffer*/
    struct cling_protocol_data *private_data = (struct cling_protocol_data*)(arg->user_data);
    assert(private_data);

    massage_mac_send_payload((char*)pinf, lenth, PACKAGE_CMD);

}
#endif


/******************************************************************************
 * FunctionName : cling_data_send
 * Description  : uart received poll implementation to get cling imformation
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
#if 0

void ICACHE_FLASH_ATTR
cling_data_init(void)
{

}
#endif



