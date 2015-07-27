#include "driver/spi_comm.h"
#include "driver/spi.h"
#include "driver/spi_user.h"
#include "io_assignment.h"
#include "gpio.h"
#include <mem.h>

/*********************************************************************
* MACROS
*/

/*********************************************************************
* TYPEDEFS
*/


/*********************************************************************
* GLOBAL VARIABLES
*/


/*********************************************************************
* LOCAL VARIABLES
*/

LOCAL int  spic_transfer(const u8 *cmd, int n_cmd, u8 *buf, int n_buf, int flag);


unsigned long raspi_init(void);


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
spi_comm_init()
{

    PIN_FUNC_SELECT(CLING_FLASH_CS_IO_MUX, CLING_FLASH_CS_IO_FUNC);
    PIN_PULLUP_EN(CLING_FLASH_CS_IO_MUX);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);

#ifdef HSPI_OVERLAP
    //WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105|(clock_div_flag<<9)); //Set bit 9 if 80MHz sysclock required
    hapi_overlap_init();
#endif

#if 1
    //init SPI bus
    spi_init_gpio(SPI_DEV, SPI_CLK_USE_DIV);
    spi_clock(SPI_DEV, 2, 2); //10MHz
    spi_tx_byte_order(SPI_DEV, SPI_BYTE_ORDER_HIGH_TO_LOW);
    spi_rx_byte_order(SPI_DEV, SPI_BYTE_ORDER_HIGH_TO_LOW);
    SET_PERI_REG_MASK(SPI_USER(SPI_DEV), SPI_CS_SETUP|SPI_CS_HOLD);
    CLEAR_PERI_REG_MASK(SPI_USER(SPI_DEV), SPI_FLASH_MODE);
#endif


    return TRUE;
}



