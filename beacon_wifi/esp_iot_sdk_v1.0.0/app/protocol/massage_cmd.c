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
#define zFootAirLed         zDisplay[0].bit1.sb0 //   				����ѹ��ʾ
#define zCalfAirLed         zDisplay[0].bit1.sb1 //   				����ѹ��ʾ
#define zSeatAirLed         zDisplay[0].bit1.sb2  //  				����ѹ��ʾ
//#define zWaistAirLed      zDisplay[0].bit1.sb3				
#define zArmAirLed          zDisplay[0].bit1.sb4 //   				����ѹ��ʾ
				
#define zCalfKneadLed       zDisplay[0].bit1.sb6//				�Ȳ��ද��ʾ
//===============================================				
#define zLegUp              zDisplay[1].bit1.sb0  //  				���Ƹ�����
#define zLegDown            zDisplay[1].bit1.sb1 //   				���Ƹ��½�
#define zLegUpLim           zDisplay[1].bit1.sb2 //   				���Ƹ�����λ
#define zLegDownLim         zDisplay[1].bit1.sb3 //   				���Ƹ�����λ
#define zFootUp             zDisplay[1].bit1.sb4 //   				�ŵ׹�����ǰ
#define zFootDown           zDisplay[1].bit1.sb5 //   				�ŵ׹������
#define zTwoBeep            zDisplay[1].bit1.sb6//				������
//===============================================				
#define zBackUp             zDisplay[2].bit1.sb0  //  				���Ƹ�����
#define zBackDown           zDisplay[2].bit1.sb1  //  				���Ƹ˵���
#define zBackUpLim          zDisplay[2].bit1.sb2  //  				���Ƹ�������λ
#define zBackDownLim        zDisplay[2].bit1.sb3   // 				���Ƹ˵�����λ
#define zFootUpLim          zDisplay[2].bit1.sb4   // 				�ŵ׹���ǰ��λ
#define zFootDownLim        zDisplay[2].bit1.sb5   // 				�ŵ׹�������λ
//=============================================== 				
#define zSHUp               zDisplay[3].bit1.sb0   // 				�Ȳ��쳤
#define zSHDown             zDisplay[3].bit1.sb1    //				�Ȳ�����
#define zSHUpLim            zDisplay[3].bit1.sb2  //  				�Ȳ��쳤��λ
#define zSHDownLim          zDisplay[3].bit1.sb3   // 				�Ȳ�������λ
#define zIntensity0         zDisplay[3].bit1.sb4   // 				b6b5b4=0b001ǿ��1����b6b5b4=0b010ǿ��2����b6b5b4=0b011ǿ��3��������û���õ�
#define zIntensity1         zDisplay[3].bit1.sb5    				
#define zIntensity2         zDisplay[3].bit1.sb6    				
//=============================================== 				
#define zReset              zDisplay[4].bit1.sb0    //				���帴λ
#define zShape              zDisplay[4].bit1.sb1    //				jShape=1,zShape =0:����Զ���⿪ʼ��jShape=0,zShape =1:����Զ����������ֶ�������ʼ��jShape=0,zShape =0:��߼�����
				
				
				
#define zSeatVibLed         zDisplay[4].bit1.sb6	//			������
//===============================================				
#define jWalkUpLim          jDisplay[0].bit1.sb0    //				��������λ
#define jWalkDownLim        jDisplay[0].bit1.sb1  	//			��������λ
#define j3DUpLim            jDisplay[0].bit1.sb2    //				3D����λ
#define j3DDownLim          jDisplay[0].bit1.sb3    //				3D����λ
#define jWidth0             jDisplay[0].bit1.sb4    //				b5b4=0b00�أ�b5b4=0b01��b5b4=0b10�У�b5b4=0b11խ��
#define jWidth1             jDisplay[0].bit1.sb5    				
#define jKneadFX            jDisplay[0].bit1.sb6    //				���ࣽ0�����ࣽ1
//=====================================.bit1====				
#define jKnockSpeed0        jDisplay[1].bit1.sb0	//			b2b1b0=0b000ǿ�ȹأ�b2b1b0=0b001ǿ��1����b2b1b0=0b010ǿ��2����b2b1b0=0b011ǿ��3����b2b1b0=0b100ǿ��4����b2b1b0=0b101ǿ��5��������û���õ�
#define jKnockSpeed1        jDisplay[1].bit1.sb1	//			
#define jKnockSpeed2        jDisplay[1].bit1.sb2	//			
				
#define jKnockMode0         jDisplay[1].bit1.sb4    //				b5b4=0b00�û���b5b4=0b01�Ĵ�b5b4=0b10ָѹ��b5b4=0b11��
#define jKnockMode1         jDisplay[1].bit1.sb5    				
#define jPressShoulder      jDisplay[1].bit1.sb6    				
//=====================================.bit1===- 				
#define jDotPosition0       jDisplay[2].bit1.sb0    //				b3b2b1b0=0b0000�������µ㡣����b3b2b1b0=0b1001�������ϵ�
#define jDotPosition1       jDisplay[2].bit1.sb1    				
#define jDotPosition2       jDisplay[2].bit1.sb2    				
#define jDotPosition3       jDisplay[2].bit1.sb3    				
#define j3DPosition0        jDisplay[2].bit1.sb4    //				b6b5b4=0b000��D���㡣����b6b5b4=0b100��D��ǰ��
#define j3DPosition1        jDisplay[2].bit1.sb5    				
#define j3DPosition2        jDisplay[2].bit1.sb6    				
//=====================================.bit1==== 				
				
				
				
				
#define jKneadSpeed0        jDisplay[3].bit1.sb4    	//			b6b5b4=0b000ǿ�ȹأ�b6b5b4=0b001ǿ��1����b6b5b4=0b010ǿ��2����b6b5b4=0b011ǿ��3����b6b5b4=0b100ǿ��4����b6b5b4=0b101ǿ��5��������û���õ�
#define jKneadSpeed1        jDisplay[3].bit1.sb5    				
#define jKneadSpeed2        jDisplay[3].bit1.sb6    				
//=====================================.bit1=====				
#define jReset              jDisplay[4].bit1.sb0    //				��о��λ
#define jShape              jDisplay[4].bit1.sb1    //				jShape=1,zShape =0:����Զ���⿪ʼ��jShape=0,zShape =1:����Զ����������ֶ�������ʼ��jShape=0,zShape =0:��߼�����
#define jWalkErr            jDisplay[4].bit1.sb2    //				����������
#define jKneadErr           jDisplay[4].bit1.sb3	//			���ߵ������
				
#define jWKerr              jDisplay[4].bit1.sb5    //				������λ����
#define j3Derr              jDisplay[4].bit1.sb6    //				3D��λ����
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
	MASSAGE_VABRATING_MOTOR_ON_LEVEL5
};



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
	arg->send_data(arg, &command, 1);
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
	uint8 command = MASSAGE_POWER_OFF;
	arg->send_data(arg, &command, 1);
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

		/*massage instruction communication method*/
		arg->power_on = power_on;
		arg->power_off = power_off;
		
        arg->enable_recieving = enable_recieving;
        arg->disable_recieving = disable_recieving;
		
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



