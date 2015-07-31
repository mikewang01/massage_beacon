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
#define  CLING_CLING_UART_FIFO_POLL_PERIOAD 50


#define MASSAGE_POWER_ON 
/*=====================================================================massage related command==================*/


/*********************************************************************
* TYPEDEFS
*/

struct cling_protocol_data {
    uint8 task_id_reg;
    os_timer_t uart_timer;
};

/*********************************************************************
* GLOBAL VARIABLES
*/


/*********************************************************************
* LOCAL VARIABLES
*/
LOCAL CLASS(massage_protocol) *this = NULL;


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

        arg->enable_recieving = enable_recieving;
        arg->disable_recieving = disable_recieving;

        //uart_init(BIT_RATE_115200, BIT_RATE_115200);

        set_massage_recieved_cmd_call_back(cling_cmd_rev_callback);
        /*restart flag updating progress*/
        //  os_timer_disarm(&(private_data->uart_timer));
        // os_timer_setfn(&(private_data->uart_timer), (os_timer_func_t *)cling_data_recieved_poll, arg);
        //  os_timer_arm(&(private_data->uart_timer), CLING_CLING_UART_FIFO_POLL_PERIOAD, 1);
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

    struct mac_layer_payload_rev *i = NULL;
    /*malloc corresponed dparameter buffer*/
    struct cling_protocol_data *private_data = (struct cling_protocol_data*)(arg->user_data);
    assert(private_data);

    /*if obtain a message from fifo succesfully,return true*/
    if (obtain_payload_from_revlist(&i) == TRUE) {
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

    massage_mac_send_payload((char*)pinf, lenth, PACKAGE_HIGH_FIVEBYTES);

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



