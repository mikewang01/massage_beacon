
#include "driver/spi.h"
#include "driver/spi_user.h"
#include "io_assignment.h"
#include "gpio.h"
#include <mem.h>
#include "driver/spi_comm.h"

/*********************************************************************
* MACROS
*/

/*********************************************************************
* TYPEDEFS
*/
#define SPI_DEV HSPI
/*open HSPI_OVERLAP function*/
//#define HSPI_OVERLAP
struct spi_comm_private_data {

    uint16 task_id_reg;
    int (*callback_function)(uint8, RcvMsgBuff *);
};

/*********************************************************************
* GLOBAL VARIABLES
*/


/*********************************************************************
* LOCAL VARIABLES
*/
#define SPI_BUFFER_MAX 128
LOCAL RcvMsgBuff spi_rev_buffer = {
    .pRcvMsgBuff = NULL,
};

LOCAL CLASS(spi_comm) *spi_comm_this;

LOCAL int  spic_transfer(const u8 *cmd, int n_cmd, u8 *buf, int n_buf, int flag);
int receive_one_char_callback(uint8 rev_char, RcvMsgBuff *para);

/******************************************************************************
 * FunctionName : hapi_overlap_init
 * Description  : implemetn
 * Parameters   : uint8 spi_no - SPI module number, Only "SPI" and "HSPI" are valid
*******************************************************************************/
#ifdef HSPI_OVERLAP

LOCAL  void  ICACHE_FLASH_ATTR
hapi_overlap_init()
{
    //hspi overlap to spi, two spi masters on cspi
    SET_PERI_REG_MASK(HOST_INF_SEL, REG_CSPI_OVERLAP);

    //set higher priority for spi than hspi
    SET_PERI_REG_MASK(SPI_EXT3(SPI),0x1);
    SET_PERI_REG_MASK(SPI_EXT3(HSPI),0x3);
    SET_PERI_REG_MASK(SPI_USER(HSPI), BIT(5));

//select HSPI CS2 ,disable HSPI CS0 and CS1
    //CLEAR_PERI_REG_MASK(SPI_PIN(HSPI), SPI_CS2_DIS);
    SET_PERI_REG_MASK(SPI_PIN(HSPI), SPI_CS0_DIS |SPI_CS1_DIS |SPI_CS2_DIS);

//SET IO MUX FOR GPIO0 , SELECT PIN FUNC AS SPI CS2
//IT WORK AS HSPI CS2 AFTER OVERLAP(THERE IS NO PIN OUT FOR NATIVE HSPI CS1/2)
    // PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_SPI_CS2);
}
#endif

/******************************************************************************
 * FunctionName : spi_user_flash_init
 * Description  : USER_FLASH_SPI port init
 * Parameters   : TRUE:£successfully FALSE:FAILED
*******************************************************************************/
bool ICACHE_FLASH_ATTR
spi_hardware_init()
{

    if(spi_rev_buffer.pRcvMsgBuff != NULL) {
        CLING_DEBUG("%s reinitiate\r\n ", __FUNCTION__);
        /*this means the initiate routine has been exucuted once*/
        //return TRUE;
    } else {
        /*initiate spi  buffer parameter*/
        spi_rev_buffer.pRcvMsgBuff = (uint8*)os_malloc(SPI_BUFFER_MAX);
        assert(spi_rev_buffer.pRcvMsgBuff != NULL);
        spi_rev_buffer.RcvBuffSize = SPI_BUFFER_MAX;
        spi_rev_buffer.pReadPos = spi_rev_buffer.pRcvMsgBuff;
        spi_rev_buffer.pWritePos = spi_rev_buffer.pRcvMsgBuff;
        spi_rev_buffer.BuffState = EMPTY;
        CLING_DEBUG("%s initiate first time\r\n ", __FUNCTION__);
    }
    // PIN_FUNC_SELECT(CLING_FLASH_CS_IO_MUX, CLING_FLASH_CS_IO_FUNC);
    // PIN_PULLUP_EN(CLING_FLASH_CS_IO_MUX);
    // GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);

#if 1
    //init SPI bus
    spi_init_gpio(SPI_DEV, SPI_CLK_USE_DIV);
    spi_clock(SPI_DEV, 4, 4); //5MHz
    spi_tx_byte_order(SPI_DEV, SPI_BYTE_ORDER_HIGH_TO_LOW);
    spi_rx_byte_order(SPI_DEV, SPI_BYTE_ORDER_HIGH_TO_LOW);
    SET_PERI_REG_MASK(SPI_USER(SPI_DEV), SPI_CS_SETUP|SPI_CS_HOLD);
    CLEAR_PERI_REG_MASK(SPI_USER(SPI_DEV), SPI_FLASH_MODE);
    
#endif
    return TRUE;
}




/******************************************************************************
 * FunctionName : spi_send_byes
 * Description  : send buffer data function
 * Parameters   :   char *s   : buffer pointyer
 *					uint16 lenth :bufferlenth
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
spi_send_byes(char *s, uint16 lenth)
{
    int i = (lenth/sizeof(uint32));
    // GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 0);
#if 0
    for(; i > 0; i --) {
        /*we need to tell if this comes to the end*/
        //spi_tx32(SPI_DEV, *((uint32*)s));
        s += sizeof(uint32);
        lenth -= sizeof(uint32);
    }
    /*there is less than uint32 size byte*/
    if(lenth%sizeof(uint32)) {
        spi_txd(SPI_DEV, (lenth << 3), s);
    }
#endif
	//spi_init_gpio(SPI_DEV, SPI_CLK_USE_DIV);
    spi_txd(SPI_DEV, (lenth << 3), s);
    //  GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);
}
/******************************************************************************
 * FunctionName : get_valid_snyc_sinal_short
 * Description  : get valid shoret pressed sigbal
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
bool ICACHE_FLASH_ATTR
send_buffer(CLASS(spi_comm) *arg, char *s, size_t lenth)
{
    spi_send_byes(s, lenth);
    CLING_DEBUG("send_buffer(CLASS(spi_comm) *arg, char *s, size_t lenth)\r\n");
    return TRUE;
}


/******************************************************************************
 * FunctionName : callbackfunction_register
 * Description  : internal used to register recieve one chatcall back function
 * Parameters   : arg -- object pointer
 *				  task_id -- task-id who gonna recieve message
 * Returns      : none
*******************************************************************************/

#if 1
LOCAL bool ICACHE_FLASH_ATTR
callbackfunction_register(CLASS(spi_comm) *arg, int (*callback)(uint8, RcvMsgBuff *))
{
    /*check object parameter*/
    assert(NULL != arg);
    /*malloc corresponed dparameter buffer*/
    struct spi_comm_private_data *callback_data = (struct spi_comm_private_data*)(arg->user_data);
    assert(NULL != callback_data);

    /*if taskid passed here is valid*/
    if (NULL == callback_data->callback_function) {
        /*register taskid here*/
        callback_data->callback_function = callback;
    } else {
        return FALSE;
    }

    return TRUE;


}
#endif


/******************************************************************************
 * FunctionName : delete_spi_comm
 * Description  : delete spi communicaiton object
 * Parameters   : arg -- object pointer
 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
delete_spi_comm(CLASS(spi_comm) *arg)
{
    /*check object parameter*/
    assert(NULL != arg);

    /*malloc corresponed dparameter buffer*/
    struct spi_comm_private_data *callback_data = (struct spi_comm_private_data*)(arg->user_data);

    os_free(callback_data);
    os_free(arg);
    /*clear local object pointer in case of memory error*/
    spi_comm_this = NULL;
    return TRUE;


}
#endif




/******************************************************************************
 * FunctionName : get_valid_snyc_sinal_short
 * Description  : get valid shoret pressed sigbal
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
get_valid_snyc_signal_short(void)
{

    char rev = 0;
    int n = 0;
    struct spi_comm_private_data *callback_data = (struct spi_comm_private_data*)(spi_comm_this->user_data);
	//spi_init_gpio(SPI_DEV, SPI_CLK_USE_DIV);
    //GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 0);
    //  rev = spi_rx8(SPI_DEV);
    //  CLING_DEBUG("spi read = 0x%02x\r\n",rev);
#if 0
    do {
        rev = spi_rx8(SPI_DEV);
        n++;
        CLING_DEBUG("spi read = 0x%02x\r\n",rev);

        /*when received a sound pakage or exeed the up limitation number of received bytes*/
        if(n > 20 || (callback_data->callback_function == NULL)?1:callback_data->callback_function(rev, &spi_rev_buffer)) {
            break;
        }
        /*if a complete frame package has not been recieved, keeps reciveving*/
    } while(1);
#else
#define  SPI_MAX_BUFFER 24
    char buffer[SPI_MAX_BUFFER];
    spi_multi_bytes_read(SPI_DEV, buffer, SPI_MAX_BUFFER<<3);

    if((callback_data->callback_function == NULL)) {
        return;
    } else {
        int i =0;
        for(; i < SPI_MAX_BUFFER; i++) {
            CLING_DEBUG("0x%02x ",buffer[i]);
            if(callback_data->callback_function(buffer[i], &spi_rev_buffer) == 1)
                break;
        }
        CLING_DEBUG("\r\n");
    }
#endif
    //GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);
    CLING_DEBUG("get sync signal for spai\r\n");
}

/******************************************************************************
 * FunctionName : start_upgrade_trigger_key_install
 * Description  :  install coressponed trigerr key used to triger a upgrade
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
set_up_spi_sync_signal_key(void)
{
    //PIN_PULLUP_EN(CLING_SPI_SYNC_INPUT_IO_MUX);
    key_add_single(CLING_SPI_SYNC_INPUT_IO_NUM, CLING_SPI_SYNC_INPUT_IO_MUX, CLING_SPI_SYNC_INPUT_IO_FUNC,
                   NULL, get_valid_snyc_signal_short);
}

/******************************************************************************
 * FunctionName : spi_sync_pin_interrupt_init
 * Description  : GPIO INTERUPT INITIATE
 * Parameters   : TRUE:£successfully FALSE:FAILED
*******************************************************************************/
bool ICACHE_FLASH_ATTR
spi_sync_pin_interrupt_init()
{
    set_up_spi_sync_signal_key();
    return TRUE;
}


/******************************************************************************
 * FunctionName : init_spi_comm
 * Description  : init spi communication port
 * Parameters   : arg -- object pointer

 * Returns      : none
*******************************************************************************/
#if 1
bool ICACHE_FLASH_ATTR
init_spi_comm(CLASS(spi_comm) *arg)
{

    /*check object parameter*/
    assert(NULL != arg);
    /*this module can only be ccreated once ,so if uart_object is not NULL ,the free object and return false*/
    if (spi_comm_this != NULL) {
        return FALSE;
    }
    /*assign object pointer to local pointer*/
    spi_comm_this = arg;

    /*malloc corresponed dparameter buffer*/
    struct spi_comm_private_data *callback_data = (struct spi_comm_private_data*)(os_malloc(sizeof(struct spi_comm_private_data)));
    assert(NULL != callback_data);
    /*initialize uart device*/

    callback_data->task_id_reg =9;
    callback_data->callback_function = NULL;
    /*assign callback private data to pointer in uart device object*/
    arg->user_data = callback_data;

    /*assign corresponded implementation*/
    arg->init = init_spi_comm;
    arg->de_init = delete_spi_comm;
    arg->task_register = NULL;
    arg->recv_callback_register = callbackfunction_register;
    arg->send = send_buffer;
    spi_hardware_init();
    spi_sync_pin_interrupt_init();
    CLING_DEBUG("init_spi_comm(CLASS(spi_comm) *arg)\r\n");
    return TRUE;

}
#endif
//get_cling_inf_json





