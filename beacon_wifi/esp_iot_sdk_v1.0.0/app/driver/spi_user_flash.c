#include "driver/spi_user_flash.h"
#include "driver/spi.h"
#include "driver/spi_user.h"
#include <mem.h>
#include "io_assignment.h"
#include "gpio.h"

/*********************************************************************
* MACROS
*/
#define USER_SPI_FLASH_SEC_SIZE      4096

#define HSPI_OVERLAP

#define HOST_INF_SEL 0x3ff00028
#define FUNC_SPI_CS2 1
#define REG_CSPI_OVERLAP (BIT(7))

#define USER_FLASH_SPI  HSPI
#define SPI_DEV HSPI


#define CONFIG_COMMANDS 1
#define CFG_CMD_SPI     1

#if (CONFIG_COMMANDS & CFG_CMD_SPI)

/******************************************************************************
 * SPI FLASH elementray definition and function
 ******************************************************************************/

#define FLASH_DUMMY_WRITE   0Xff
#define FLASH_PAGESIZE		256

//#define NO_4B_ADDRESS_SUPPORT

#ifndef NO_4B_ADDRESS_SUPPORT
#define ADDRESS_4B_MODE
#endif


/* Flash opcodes. */
#define OPCODE_WREN		6	/* Write enable */
#define OPCODE_WRDI		4	/* Write disable */
#define OPCODE_RDSR		5	/* Read status register */
#define OPCODE_WRSR		1	/* Write status register */
#define OPCODE_READ		3	/* Read data bytes */
#define OPCODE_PP		2	/* Page program */
#define OPCODE_SE		0x20	/* Sector erase */
#define OPCODE_RES		0xAB	/* Read Electronic Signature */
#define OPCODE_RDID		0x9F	/* Read JEDEC ID */

#define OPCODE_FAST_READ	0x0B		/* Fast Read */
#define OPCODE_DOR			0x3B	/* Dual Output Read */
#define OPCODE_QOR			0x6B	/* Quad Output Read */
#define OPCODE_DIOR			0xBB	/* Dual IO High Performance Read */
#define OPCODE_QIOR			0xEB	/* Quad IO High Performance Read */
#define OPCODE_READ_ID		0x9F	/* Read Manufacturer and Device ID */

#define OPCODE_P4E			0x20	/* 4KB Parameter Sectore Erase */
#define OPCODE_P8E			0x40	/* 8KB Parameter Sectore Erase */
#define OPCODE_BE			0x60	/* Bulk Erase */
#define OPCODE_BE1			0xC7	/* Bulk Erase */
#define OPCODE_QPP			0x32	/* Quad Page Programing */

#define OPCODE_CLSR			0x30
#define OPCODE_RCR			0x35	/* Read Configuration Register */

#define OPCODE_BRRD			0x16
#define OPCODE_BRWR			0x17


/* Status Register bits. */
#define SR_WIP			1	/* Write in progress */
#define SR_WEL			2	/* Write enable latch */
#define SR_BP0			4	/* Block protect 0 */
#define SR_BP1			8	/* Block protect 1 */
#define SR_BP2			0x10	/* Block protect 2 */
#define SR_EPE			0x20	/* Erase/Program error */
#define SR_SRWD			0x80	/* SR write protect */


/*#define ra_dbg(args...)*/
#define ra_dbg(args...) do { if (1) os_printf(args); } while(0)

#define SPI_FIFO_SIZE 16

#if defined (MT7620_ASIC_BOARD) || defined (MT7620_FPGA_BOARD)
#define COMMAND_MODE		// define this for SPI flash command/user mode support
#endif
//#define COMMAND_MODE		// define this for SPI flash command/user mode support
//#define ADDR_4B			// if all instruction use 4B address mode
//#define RD_MODE_FAST		// use Fast Read instead of normal Read
//#define RD_MODE_DIOR		// use DIOR (0xBB)instead of normal Read
//#define RD_MODE_DOR		// use DOR (0x3B) instead of normal Read
//#define RD_MODE_QIOR		// use QIOR (0xEB) instead of normal Read
//#define RD_MODE_QOR		// use QOR (0x6B) instead of normal Read

//#define READ_BY_PAGE

#if defined(RD_MODE_QOR) || defined(RD_MODE_QIOR)
#define RD_MODE_QUAD
#endif



#define NO_4B_ADDRESS_SUPPORT


#define SPIC_READ_BYTES (1<<0)
#define SPIC_WRITE_BYTES (1<<1)
#define SPIC_USER_MODE (1<<2)
#define SPIC_4B_ADDR (1<<3)

#define MIN(X, Y) ((X < Y)?X:Y)

#define udelay(x) os_delay_us(x)
#define GET_CURRENT_SYSTEMTICK() system_get_time()

/*********************************************************************
* TYPEDEFS
*/
struct chip_info {
    char	*name;
    u16		id;
    u32 	jedec_id;
    unsigned long	sector_size;
    unsigned int	n_sectors;
    char		addr4b;
};

typedef enum {
    SPI_FLASH_RESULT_OK,
    SPI_FLASH_RESULT_ERR,
    SPI_FLASH_RESULT_TIMEOUT
} UserSpiFlashOpResult;

typedef struct {
    uint32	deviceId;
    uint32	chip_size;	  // chip size in byte
    uint32	block_size;
    uint32	sector_size;
    uint32	page_size;
    uint32	status_mask;
} UserSpiFlashChip;



/*********************************************************************
* GLOBAL VARIABLES
*/


/*********************************************************************
* LOCAL VARIABLES
*/

LOCAL int  spic_transfer(const u8 *cmd, int n_cmd, u8 *buf, int n_buf, int flag);
uint32 spi_user_flash_get_id(void);
UserSpiFlashOpResult spi_user_flash_erase_sector(uint16 sec);
UserSpiFlashOpResult spi_user_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size);
UserSpiFlashOpResult spi_user_flash_read(uint32 src_addr, uint32 *des_addr, uint32 size);


struct chip_info *spi_chip_info = NULL;

static struct chip_info chips_data [] = {
    /* REVISIT: fill in JEDEC ids, for parts that have them */
    { "AT25DF321",		0x1f, 0x47000000, 64 * 1024, 64,  0 },
    { "AT26DF161",		0x1f, 0x46000000, 64 * 1024, 32,  0 },
    { "FL016AIF",		0x01, 0x02140000, 64 * 1024, 32,  0 },
    { "FL064AIF",		0x01, 0x02160000, 64 * 1024, 128, 0 },
    { "MX25L1605D", 	0xc2, 0x2015c220, 64 * 1024, 32,  0 },
    { "MX25L3205D", 	0xc2, 0x2016c220, 64 * 1024, 64,  0 },
    { "MX25L6405D", 	0xc2, 0x2017c220, 64 * 1024, 128, 0 },
    { "MX25L12805D",	0xc2, 0x2018c220, 64 * 1024, 256, 0 },
#ifndef NO_4B_ADDRESS_SUPPORT
    { "MX25L25635E",	0xc2, 0x2019c220, 64 * 1024, 512, 1 },
    { "S25FL256S",		0x01, 0x02194D01, 64 * 1024, 512, 1 },
#endif
    { "S25FL128P",		0x01, 0x20180301, 64 * 1024, 256, 0 },
    { "S25FL129P",		0x01, 0x20184D01, 64 * 1024, 256, 0 },
    { "S25FL032P",		0x01, 0x02154D00, 64 * 1024, 64,  0 },
    { "S25FL064P",		0x01, 0x02164D00, 64 * 1024, 128, 0 },
    { "F25L64QA",			0x8c, 0x41170000, 64 * 1024, 128, 0 }, //ESMT
    { "EN25F16",		0x1c, 0x31151c31, 64 * 1024, 32,  0 },
    { "EN25Q32B",		0x1c, 0x30161c30, 64 * 1024, 64,  0 },
    { "EN25F32",		0x1c, 0x31161c31, 64 * 1024, 64,  0 },
    { "EN25F64",		0x1c, 0x20171c20, 64 * 1024, 128,  0 }, //EN25P64
    { "EN25Q64",		0x1c, 0x30171c30, 64 * 1024, 128,  0 },
    { "W25Q80BV",		0xef, 0x40140000, 4 * 1024, 256,  0 }, //S25FL032K
    { "W25Q32BV",		0xef, 0x40160000, 64 * 1024, 64,  0 }, //S25FL032K
    { "W25Q64BV",		0xef, 0x40170000, 64 * 1024, 128,  0 }, //S25FL064K
};


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
spi_user_flash_init()
{

    PIN_FUNC_SELECT(CLING_FLASH_CS_IO_MUX, CLING_FLASH_CS_IO_FUNC);
    PIN_PULLUP_EN(CLING_FLASH_CS_IO_MUX);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);
#if 0
    while(1) {
        GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);
        udelay(500);
        GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 0);
        udelay(500);
    }
#endif
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
    os_printf("flash_size = %d\n", raspi_init());
#endif


    return TRUE;
}
/******************************************************************************
 * FunctionName : spi_user_flash_get_id
 * Description  : implement function to get flash id
 * Parameters   :
*******************************************************************************/
uint32 spi_user_flash_get_id(void)
{
    return spi_transaction(USER_FLASH_SPI, 8, 0x90, 24, 0, 0, 0, 16, 0);

}

UserSpiFlashOpResult spi_user_flash_erase_sector(uint16 sec);
UserSpiFlashOpResult spi_user_flash_write(uint32 des_addr, uint32 *src_addr, uint32 size);
UserSpiFlashOpResult spi_user_flash_read(uint32 src_addr, uint32 *des_addr, uint32 size);

LOCAL int ICACHE_FLASH_ATTR  raspi_wait_ready(int sleep_ms);
static unsigned int spi_wait_nsec = 0;




LOCAL int ICACHE_FLASH_ATTR  spic_busy_wait(void)
{
    spi_wait_nsec = GET_CURRENT_SYSTEMTICK();
    uint32 spi_wait_nsec_new = 0;
    do {
        if (spi_busy(USER_FLASH_SPI)) {
        } else
            return 0;
        spi_wait_nsec_new = GET_CURRENT_SYSTEMTICK();

    } while (spi_wait_nsec_new - spi_wait_nsec < 10*1000000); /*if the eclapesed time is less than 1 second*/

    os_printf("%s: fail \n", __func__);
    return -1;
}






/*
 * @cmd: command and address
 * @n_cmd: size of command, in bytes
 * @buf: buffer into which data will be read/written
 * @n_buf: size of buffer, in bytes
 * @flag: tag as READ/WRITE
 *
 * @return: if write_onlu, -1 means write fail, or return writing counter.
 * @return: if read, -1 means read fail, or return reading counter.
 */
LOCAL int ICACHE_FLASH_ATTR
spic_transfer(const u8 *cmd, int n_cmd, u8 *buf, int n_buf, int flag)
{

    int retval = -1;

    //udelay(1);
    ra_dbg("cmd(%x): %x %x %x %x , buf:%x len:%x, flag:%s \n",
           n_cmd, cmd[0], cmd[1], cmd[2], cmd[3],
           (buf)? (*buf) : 0, n_buf,
           (flag == SPIC_READ_BYTES)? "read" : "write");
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 0);

    // assert CS and we are already CLK normal high
    //ra_and(RT2880_SPI0_CTL_REG, ~(SPICTL_SPIENA_HIGH));
    //GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 0);
    // write command
    for (retval = 0; retval < n_cmd; retval++) {

        //spi_mast_byte_write(USER_FLASH_SPI ,cmd[retval]);
        spi_tx8(USER_FLASH_SPI, cmd[retval]);
        if (spic_busy_wait()) {
            retval = -1;
            goto end_trans;
        }
    }

    // read / write  data
    if (flag & SPIC_READ_BYTES) {
        for (retval = 0; retval < n_buf; retval++) {

#ifndef READ_BY_PAGE
            if (n_cmd != 1 && (retval & 0xffff) == 0) {
                os_printf(".");
            }
#endif
            if (spic_busy_wait()) {
                os_printf("\n");
                goto end_trans;
            }

            //buf[retval] = (u8) spi_mast_byte_read(USER_FLASH_SPI);//ra_inl(RT2880_SPI0_DATA_REG);
            buf[retval] = spi_rx8(USER_FLASH_SPI);
        }

    } else if (flag & SPIC_WRITE_BYTES) {
        for (retval = 0; retval < n_buf; retval++) {
            //spi_mast_byte_write(USER_FLASH_SPI ,buf[retval]);
            spi_tx8(USER_FLASH_SPI, buf[retval]);
            if (spic_busy_wait()) {
                goto end_trans;
            }
        }
    }
    //GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);

end_trans:
    // de-assert CS and
    //ra_or (RT2880_SPI0_CTL_REG, (SPICTL_SPIENA_HIGH));
    //udelay(1);
    GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);

    return retval;

}




LOCAL int ICACHE_FLASH_ATTR
spic_read(const u8 *cmd, size_t n_cmd, u8 *rxbuf, size_t n_rx)
{
    int ret;
    //GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 0);
    ret = spic_transfer(cmd, n_cmd, rxbuf, n_rx, SPIC_READ_BYTES);
    // GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);
    return ret;
}


LOCAL int ICACHE_FLASH_ATTR
spic_write(const u8 *cmd, size_t n_cmd, const u8 *txbuf, size_t n_tx)
{
    int ret;

// GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 0);
    ret = spic_transfer(cmd, n_cmd, (u8 *)txbuf, n_tx, SPIC_WRITE_BYTES);
//  GPIO_OUTPUT_SET(GPIO_ID_PIN(CLING_FLASH_CS_IO_NUM), 1);
    return ret;
}



#ifdef COMMAND_MODE
LOCAL int ICACHE_FLASH_ATTR  raspi_cmd(const u8 cmd, const u32 addr, const u8 mode, u8 *buf, const size_t n_buf, const u32 user, const int flag)
{
}

#endif


#if defined (RD_MODE_QUAD)
LOCAL int ICACHE_FLASH_ATTR  raspi_set_quad()
{

}

#endif
/*
 * read SPI flash device ID
 */
LOCAL int ICACHE_FLASH_ATTR
raspi_read_devid(u8 *rxbuf, int n_rx)
{
    u8 code = OPCODE_READ_ID;
    int retval;

#ifdef COMMAND_MODE
#else
    retval = spic_read(&code, 1, rxbuf, n_rx);
#endif

    if (retval != n_rx) {
        os_printf("%s: ret: %x\n", __func__, retval);
        return retval;
    }

    return retval;
}



#ifndef NO_4B_ADDRESS_SUPPORT
LOCAL int ICACHE_FLASH_ATTR
raspi_read_rg(u8 *val, u8 opcode)
{
    size_t retval;
    u8 code = opcode;
    u32 user;

    if (!val)
        os_printf("NULL pointer\n");


#ifdef COMMAND_MODE
    user = SPIUSR_SINGLE | (SPIUSR_SINGLE << 3) | (SPIUSR_SINGLE << 6) | (SPIUSR_SINGLE << 9) | (SPIUSR_READ_DATA << 12) | (SPIUSR_NO_DUMMY << 14) | (SPIUSR_NO_MODE << 16) | (SPIUSR_NO_ADDR << 17) | (SPIUSR_ONE_INSTRU << 20) | (1 << 21);
    retval = raspi_cmd(code, 0, 0, val, 1, user, SPIC_READ_BYTES | SPIC_USER_MODE);
#else
    retval = spic_read(&code, 1, val, 1);
#endif

    return 0;
}

LOCAL int ICACHE_FLASH_ATTR
raspi_write_rg(u8 *val, u8 opcode)
{
    size_t retval;
    u8 code = opcode;
    u32 user, dr;

    if (!val)
        os_printf("NULL pointer\n");

    dr = ra_inl(RT2880_SPI_DMA);
    ra_outl(RT2880_SPI_DMA, 0); // Set TxBurstSize to 'b00: 1 transfer

#ifdef COMMAND_MODE
    user = SPIUSR_SINGLE | (SPIUSR_SINGLE << 3) | (SPIUSR_SINGLE << 6) | (SPIUSR_SINGLE << 9) | (SPIUSR_WRITE_DATA << 12) | (SPIUSR_NO_DUMMY << 14) | (SPIUSR_NO_MODE << 16) | (SPIUSR_NO_ADDR << 17) | (SPIUSR_ONE_INSTRU << 20) | (1 << 21);
    retval = raspi_cmd(code, 0, 0, val, 1, user, SPIC_WRITE_BYTES | SPIC_USER_MODE);
#else
    retval = spic_write(&code, 1, val, 1);
#endif
    ra_outl(RT2880_SPI_DMA, dr);
    return 0;
}
#endif



/*
 * read status register
 */
LOCAL int ICACHE_FLASH_ATTR
raspi_read_sr(u8 *val)
{
    size_t retval;
    u8 code = OPCODE_RDSR;


    retval = spic_read(&code, 1, val, 1);
    if (retval != 1) {
        os_printf("%s: ret: %x\n", __func__, retval);
        return -1;
    }
    return 0;
}

/*
 * write status register
 */
LOCAL int ICACHE_FLASH_ATTR
raspi_write_sr(u8 *val)
{
    size_t retval;
    u8 code = OPCODE_WRSR;


    retval = spic_write(&code, 1, val, 1);

    if (retval != 1) {
        os_printf("%s: ret: %x\n", __func__, retval);
        return -1;
    }
    return 0;
}




#ifndef NO_4B_ADDRESS_SUPPORT
LOCAL int ICACHE_FLASH_ATTR
raspi_read_scur(u8 *val)
{
    size_t retval;
    u8 code = 0x2b;

    retval = spic_read(&code, 1, val, 1);

    if (retval != 1) {
        os_printf("%s: ret: %x\n", __func__, retval);
        return -1;
    }
    return 0;
}

LOCAL int ICACHE_FLASH_ATTR  raspi_4byte_mode(int enable)
{
    if (spi_chip_info->id == 0x01) { // Spansion
        u8 br, br_cfn; // bank register

        raspi_wait_ready(1);

        if (enable) {
            br = 0x81;

        } else {
            br = 0x0;

        }

        raspi_write_rg(&br, OPCODE_BRWR);
        raspi_read_rg(&br_cfn, OPCODE_BRRD);
        if (br_cfn != br) {
            os_printf("4B mode switch failed\n");
            return -1;
        }
    } else {
        size_t retval;
        u8 code;

        raspi_wait_ready(1);

        if (enable) {
            code = 0xB7; /* EN4B, enter 4-byte mode */
        } else {
            code = 0xE9; /* EX4B, exit 4-byte mode */
        }

        retval = spic_read(&code, 1, 0, 0);

        if (retval != 0) {
            os_printf("%s: ret: %x\n", __func__, retval);
            return -1;
        }
    }
    return 0;
}
#endif



/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
LOCAL inline int raspi_write_enable(void)
{
    u8 code = OPCODE_WREN;

    return spic_write(&code, 1, NULL, 0);

}

static inline int raspi_write_disable(void)
{
    u8 code = OPCODE_WRDI;
    return spic_write(&code, 1, NULL, 0);

}

/*
 * Set all sectors (global) unprotected if they are protected.
 * Returns negative if error occurred.
 */
LOCAL inline int raspi_unprotect(void)
{
    u8 sr = 0;

    if (raspi_read_sr(&sr) < 0) {
        os_printf("%s: read_sr fail: %x\n", __func__, sr);
        return -1;
    }

    if ((sr & (SR_BP0 | SR_BP1 | SR_BP2)) != 0) {
        sr = 0;
        raspi_write_sr(&sr);
    }
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
LOCAL int ICACHE_FLASH_ATTR
raspi_wait_ready(int sleep_ms)
{
    int count;
    int sr = 0;

    udelay(1000 * sleep_ms);

    /* one chip guarantees max 5 msec wait here after page writes,
     * but potentially three seconds (!) after page erase.
     */
    for (count = 0;  count < ((sleep_ms+1) *1000); count++) {

        if ((raspi_read_sr((u8 *)&sr)) < 0)
            break;
        else if (!(sr & (SR_WIP | SR_EPE))) {
            CLING_DEBUG("status register = %2x\n",sr);
            return 0;
        }
        /* REVISIT sometimes sleeping would be best */
    }

    os_printf("%s: read_sr fail: %x\n", __func__, sr);
    return -1;
}

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
LOCAL int ICACHE_FLASH_ATTR
raspi_erase_sector(u32 offset)
{
    u8 buf[5];

    /* Wait until finished previous write command. */
    if (raspi_wait_ready(3))
        return -1;

    /* Send write enable, then erase commands. */
    raspi_write_enable();
    raspi_unprotect();


#ifdef ADDRESS_4B_MODE

#endif
    /* Set up command buffer. */
    buf[0] = OPCODE_SE;
    buf[1] = offset >> 16;
    buf[2] = offset >> 8;
    buf[3] = offset;

    spic_write(buf, 4, 0 , 0);
    raspi_wait_ready(100);
//#endif // COMMAND_MODE

    raspi_write_disable();

    return 0;
}


struct chip_info *chip_prob(void) {
    struct chip_info *info, *match;
    u8 buf[5];
    u32 jedec = 0, weight;
    int i;


    raspi_read_devid(buf, 5);
    jedec = ((u32)buf[1] <<24 | buf[2] <<16 | buf[3] <<8 | (u32)buf[4]);
    os_printf("spi device id: %x %x %x %x %x (%x)\n", buf[0], buf[1], buf[2], buf[3], buf[4], jedec);

    // FIXME, assign default as AT25D
    weight = 0xffffffff;
    match = &chips_data[0];
    for (i = 0; i < sizeof(chips_data)/sizeof(chips_data[0]); i++) {
        info = &chips_data[i];
        if (info->jedec_id == (jedec&info->jedec_id)) {

            os_printf("find flash: %s\n", info->name);
            return info;

        }
        match = info;
    }

    os_printf("Warning: un-recognized chip ID, please update bootloader!\n");

    return match;
}

unsigned long ICACHE_FLASH_ATTR
raspi_init(void)
{
    // spic_init();
    spi_chip_info = chip_prob();
    return spi_chip_info->sector_size * spi_chip_info->n_sectors;
}


int ICACHE_FLASH_ATTR
raspi_erase(unsigned int offs, int len)
{
    ra_dbg("%s: offs:%x len:%x\n", __func__, offs, len);
    /* sanity checks */
    if (len == 0)
        return 0;

    /* now erase those sectors */
    while (len > 0) {
        if (raspi_erase_sector(offs)) {
            return -1;
        }

        offs += spi_chip_info->sector_size;
        len -= spi_chip_info->sector_size;
        os_printf(".");
    }
    os_printf("\n");

    return 0;
}

int ICACHE_FLASH_ATTR
raspi_read(char *buf, unsigned int from, int len)
{
    u8 cmd[5], code;
    int rdlen;
    //u32 start_time, end_time;

    ra_dbg("%s: from:%x len:%x \n", __func__, from, len);

    /* sanity checks */
    if (len == 0)
        return 0;

    /* Wait till previous write/erase is done. */
    if (raspi_wait_ready(1)) {
        /* REVISIT status return?? */
        return -1;
    }

    /* NOTE: OPCODE_FAST_READ (if available) is faster... */

    //start_time = get_timer(0);

#ifdef COMMAND_MODE
#else // COMMAND_MODE
    /* Set up the write data buffer. */
    cmd[0] = OPCODE_READ;

#ifndef READ_BY_PAGE

    {
        cmd[1] = from >> 16;
        cmd[2] = from >> 8;
        cmd[3] = from;
        rdlen = spic_read(cmd, 4, buf, len);
    }
    if (rdlen != len)
        os_printf("warning: rdlen != len\n");

#else // READ_BY_PAGE

#ifdef ADDRESS_4B_MODE

#endif
    {
        u32 page_size;

        rdlen = 0;
        while (len > 0) {

            cmd[1] = from >> 16;
            cmd[2] = from >> 8;
            cmd[3] = from;

            page_size = MIN(len, FLASH_PAGESIZE);
            rdlen += spic_read(cmd, 4, buf, page_size);
            buf += page_size;
            len -= page_size;
            from += page_size;
        }
    }

#endif // READ_BY_PAGE

#endif // COMMAND_MODE

    return rdlen;
}


int ICACHE_FLASH_ATTR
raspi_write(char *buf, unsigned int to, int len)
{
    u32 page_offset, page_size;
    int rc = 0, retlen = 0;
    u8 cmd[5];

    ra_dbg("%s: to:%x len:%x \n", __func__, to, len);

    /* sanity checks */
    if (len == 0)
        return 0 ;
    if (to + len > spi_chip_info->sector_size * spi_chip_info->n_sectors)
        return -1;

    /* Wait until finished previous write command. */
    if (raspi_wait_ready(1)) {
        return -1;
    }

    /* Set up the opcode in the write buffer. */
    cmd[0] = OPCODE_PP;
#ifdef ADDRESS_4B_MODE

#endif
    {
        cmd[1] = to >> 16;
        cmd[2] = to >> 8;
        cmd[3] = to;
    }

    /* what page do we start with? */
    page_offset = to % FLASH_PAGESIZE;

#ifdef ADDRESS_4B_MODE

#endif

    /* write everything in PAGESIZE chunks */
    while (len > 0) {
        page_size = MIN(len, FLASH_PAGESIZE-page_offset);
        page_offset = 0;
        /* write the next page to flash */
#ifdef ADDRESS_4B_MODE

#endif
        {
            cmd[1] = to >> 16;
            cmd[2] = to >> 8;
            cmd[3] = to;
        }

        raspi_wait_ready(1);
        raspi_write_enable();
        raspi_unprotect();

#ifdef COMMAND_MODE


#else // COMMAND_MODE
#ifdef ADDRESS_4B_MODE

#endif
        rc = spic_write(cmd, 4, buf, page_size);

#endif // COMMAND_MODE

        //os_printf("%s:: to:%x page_size:%x ret:%x\n", __func__, to, page_size, rc);
        if ((retlen & 0xffff) == 0)
            os_printf(".");

        if (rc > 0) {
            retlen += rc;
            if (rc < page_size) {
                os_printf("%s: rc:%x page_size:%x\n",
                          __func__, rc, page_size);
                raspi_write_disable();
                return retlen;
            }
        }

        len -= page_size;
        to += page_size;
        buf += page_size;
    }

#ifdef ADDRESS_4B_MODE

#endif

    os_printf("\n");

    raspi_write_disable();



    return retlen;
}

int ICACHE_FLASH_ATTR
raspi_erase_write(char *buf, unsigned int offs, int count)
{
    int blocksize = spi_chip_info->sector_size;
    int blockmask = blocksize - 1;

    ra_dbg("%s: offs:%x, count:%x\n", __func__, offs, count);

    if (count > (spi_chip_info->sector_size * spi_chip_info->n_sectors)) {
        os_printf("Abort: image size larger than %d!\n\n", (spi_chip_info->sector_size * spi_chip_info->n_sectors));
        udelay(10*1000*1000);
        return -1;
    }

    while (count > 0) {
#define BLOCK_ALIGNE(a) (((a) & blockmask))
        if (BLOCK_ALIGNE(offs) || (count < blocksize)) {
            char *block;
            unsigned int piece, blockaddr;
            int piece_size;
            char *temp;

            block = (char*)os_malloc(blocksize);
            if (!block)
                return -1;
            temp = (char*)os_malloc(blocksize);
            if (!temp)
                return -1;

            blockaddr = offs & ~blockmask;

            if (raspi_read(block, blockaddr, blocksize) != blocksize) {
                os_free(block);
                os_free(temp);
                return -2;
            }

            piece = offs & blockmask;
            piece_size = MIN(count, blocksize - piece);
            memcpy(block + piece, buf, piece_size);

            if (raspi_erase(blockaddr, blocksize) != 0) {
                os_free(block);
                os_free(temp);
                return -3;
            }
            if (raspi_write(block, blockaddr, blocksize) != blocksize) {
                os_free(block);
                os_free(temp);
                return -4;
            }
#ifdef RALINK_SPI_UPGRADE_CHECK
            if (raspi_read(temp, blockaddr, blocksize) != blocksize) {
                os_free(block);
                os_free(temp);
                return -2;
            }


            if(memcmp(block, temp, blocksize) == 0) {
                // os_printf("block write ok!\n\r");
            } else {
                os_printf("block write incorrect!\n\r");
                os_free(block);
                os_free(temp);
                return -2;
            }
#endif
            os_free(temp);
            os_free(block);

            buf += piece_size;
            offs += piece_size;
            count -= piece_size;
        } else {
            unsigned int aligned_size = count & ~blockmask;
            char *temp;
            int i;
            temp = (char*)os_malloc(blocksize);
            if (!temp)
                return -1;

            if (raspi_erase(offs, aligned_size) != 0) {
                os_free(temp);
                return -1;
            }
            if (raspi_write(buf, offs, aligned_size) != aligned_size) {
                os_free(temp);
                return -1;
            }

#ifdef RALINK_SPI_UPGRADE_CHECK
            for( i=0; i< (aligned_size/blocksize); i++) {
                if (raspi_read(temp, offs+(i*blocksize), blocksize) != blocksize) {
                    os_free(temp);
                    return -2;
                }
                if(memcmp(buf+(i*blocksize), temp, blocksize) == 0) {
                    //	os_printf("blocksize write ok i=%d!\n\r", i);
                } else {
                    os_printf("blocksize write incorrect block#=%d!\n\r",i);
                    os_free(temp);
                    return -2;
                }
            }
#endif
            os_free(temp);

            buf += aligned_size;
            offs += aligned_size;
            count -= aligned_size;
        }
    }
    os_printf("Done!\n");
    return 0;
}


#endif

/******************************************************************************
 * FunctionName : write
 * Description  : write buffer into specific adress
 * Parameters   : arg -- spi user falsh object poitner
 * Returns      : dthe amount of data numbers writed successfully
*******************************************************************************/
#if 1
LOCAL int ICACHE_FLASH_ATTR
write(uint32 start_addr, char* pbuffer, size_t lenth)
{

    return raspi_write(pbuffer, start_addr, lenth);
}
#endif
/******************************************************************************
 * FunctionName : read
 * Description  : read buffer from  specific adress
 * Parameters   : arg -- spi user falsh object poitner
 * Returns      : data numbers read successfully
*******************************************************************************/
#if 1
int ICACHE_FLASH_ATTR
LOCAL read(uint32 start_addr, char* pbuffer, size_t lenth)
{

    return raspi_read(pbuffer, start_addr, lenth);
}
#endif

/******************************************************************************
 * FunctionName : erease
 * Description  : erase sector
 * Parameters   : arg -- spi user falsh object poitner
 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
erase(uint32 start_addr, size_t lenth)
{
    if (raspi_erase(start_addr, lenth) == 0)
        return TRUE;
    else
        return FALSE;
}
#endif

/******************************************************************************
 * FunctionName : init_spi_user_flash
 * Description  : initiate spi flash interface
 * Parameters   : arg -- spi user falsh object poitner
 * Returns      : none
*******************************************************************************/
LOCAL bool delete_spi_user_flash(CLASS(spi_user_flash) *arg);
#if 1
bool ICACHE_FLASH_ATTR
init_spi_user_flash(CLASS(spi_user_flash) *arg)
{
    static bool hw_flag_init = FALSE;/*used to mark if hardware has been initialize or not*/
    assert(NULL != arg);
    arg->init = init_spi_user_flash;
    arg->de_init = delete_spi_user_flash;
    arg->erase = erase;
    arg->write = write ;
    arg->read =  read;

    if (hw_flag_init == FALSE) {
        spi_user_flash_init();
        hw_flag_init = TRUE;
    }
    return TRUE;
}
#endif
/******************************************************************************
 * FunctionName : delete_spi_user_flash
 * Description  : delete cling spi user flash interface object
 * Parameters   : CLASS(cling_fota) *arg : object pointer
 * Returns      : none
*******************************************************************************/
#if 1
LOCAL bool ICACHE_FLASH_ATTR
delete_spi_user_flash(CLASS(spi_user_flash) *arg)
{
    assert(NULL != arg);
    /*malloc corresponed parameter buffer*/
    os_free(arg);
    return TRUE;
}
#endif



