#include "ets_sys.h"
#include "spi_flash.h"

//#include "net80211/ieee80211_var.h"
#include "mem.h"

#include "upgrade.h"



#define  ble_upgrade_flag_check() ble_upgrade_flag
#define  ble_upgrade_flag_set(x)  ble_upgrade_flag = x

#define   BLE_ROM_START_SECTOR  
struct upgrade_param {
    uint32 fw_bin_addr;

    uint8 fw_bin_sec;
    uint8 fw_bin_sec_num;

    uint8 fw_bin_sec_earse;

    uint8 extra;

    uint8 save[4];

    uint8 *buffer;
};


/*********************************************************************
* LOCAL
*/
LOCAL bool (*ble_upgrade_flow)(char *, size_t) = NULL;
LOCAL struct upgrade_param *upgrade;
LOCAL uint8  ble_upgrade_flag  = UPGRADE_FLAG_IDLE;

/******************************************************************************
 * FunctionName : system_upgrade_internal
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
ble_upgrade_internal(struct upgrade_param *upgrade, uint8 *data, uint16 len)
{
    bool ret = false;
    if(data == NULL || len == 0) {
        return true;
    }
    upgrade->buffer = (uint8 *)os_zalloc(len + upgrade->extra);

    os_memcpy(upgrade->buffer, upgrade->save, upgrade->extra);
    os_memcpy(upgrade->buffer + upgrade->extra, data, len);

    len += upgrade->extra;
    upgrade->extra = len & 0x03;
    len -= upgrade->extra;

    os_memcpy(upgrade->save, upgrade->buffer + len, upgrade->extra);

    do {
        if (upgrade->fw_bin_addr + len >= (upgrade->fw_bin_sec + upgrade->fw_bin_sec_num) * SPI_FLASH_SEC_SIZE) {
            break;
        }

        if (len > SPI_FLASH_SEC_SIZE) {

        } else {
     		//os_printf("%x %x\n",upgrade->fw_bin_sec_earse,upgrade->fw_bin_addr);
            /* earse sector, just earse when first enter this zone */
            if (upgrade->fw_bin_sec_earse != (upgrade->fw_bin_addr + len) >> 12) {
                upgrade->fw_bin_sec_earse = (upgrade->fw_bin_addr + len) >> 12;
                spi_flash_erase_sector(upgrade->fw_bin_sec_earse);
				//os_printf("%x\n",upgrade->fw_bin_sec_earse);
            }
        }
			os_printf("*\n");
        if (spi_flash_write(upgrade->fw_bin_addr, (uint32 *)upgrade->buffer, len) != SPI_FLASH_RESULT_OK) {
            break;
        }

        ret = true;
        upgrade->fw_bin_addr += len;
    } while (0);

    os_free(upgrade->buffer);
    upgrade->buffer = NULL;
    return ret;
}

/******************************************************************************
 * FunctionName : system_upgrade
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL bool ICACHE_FLASH_ATTR
ble_upgrade(uint8 *data, uint16 len)
{
    bool ret;
#if 0
	/*if user control flow has been set*/
	if (ble_upgrade_flow != NULL){
		 ret = ble_upgrade_flow(data, len);
	}else{
		//ret = ble_upgrade_internal(upgrade, data, len);
	}
#endif
  ret = ble_upgrade_internal(upgrade, data, len);

    return ret;
}

/******************************************************************************
 * FunctionName : system_upgrade_init
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
ble_upgrade_init(void)
{
    uint32 user_bin2_start;
    uint8 flash_buf[4];
    uint8 high_half;

    spi_flash_read(0, (uint32 *)flash_buf, 4);
    high_half = (flash_buf[3] & 0xF0) >> 4;

    if (upgrade == NULL) {
        upgrade = (struct upgrade_param *)os_zalloc(sizeof(struct upgrade_param));
    }
	 os_memset(upgrade, 0 ,sizeof(struct upgrade_param));
     ble_upgrade_flag_set(UPGRADE_FLAG_IDLE);

    if (high_half == 2 || high_half == 3 || high_half == 4) {
        user_bin2_start = 229;	// 128 + 1
        upgrade->fw_bin_sec_num = 23;	// 128 - 1 - 4
    } else {

	    user_bin2_start = 229;	// 128 + 1
        upgrade->fw_bin_sec_num = 23;	// 128 - 1 - 4
      /* user_bin2_start = 65;	// 64 + 1
        upgrade->fw_bin_sec_num = 59;	// 64 - 1 - 4
        */
    }

    upgrade->fw_bin_sec = user_bin2_start;

    upgrade->fw_bin_addr = upgrade->fw_bin_sec * SPI_FLASH_SEC_SIZE;
}

/******************************************************************************
 * FunctionName : system_upgrade_deinit
 * Description  : a
 * Parameters   :
 * Returns      :
*******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
ble_upgrade_deinit(void)
{
    os_free(upgrade);
    upgrade = NULL;
}
