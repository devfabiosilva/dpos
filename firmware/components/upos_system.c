#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "soc/efuse_reg.h"
#include "esp32/rom/crc.h"
#include "upos_system.h"

#define TAG "UPOS_TCPIP"

int IRAM_ATTR upos_set_mac(uint8_t *mac, int use_custom_mac)
{
   uint8_t fuse_buf[7];
   uint32_t val1;
   uint32_t val2;

   if (use_custom_mac) {

      if (mac)
         return (int)esp_base_mac_addr_set(mac);

      val1=REG_READ(EFUSE_BLK3_RDATA1_REG);
      val2=REG_READ(EFUSE_BLK3_RDATA2_REG);

      asm volatile (
         "extui a14, %2, 0, 8" TAB
         "s8i a14, %4, 0" TAB
         "extui a14, %2, 8, 8" TAB
         "s8i a14, %4, 1" TAB
         "extui a14, %2, 16, 8" TAB
         "s8i a14, %4, 2" TAB
         "extui a14, %2, 24, 8" TAB
         "s8i a14, %4, 3" TAB
         "extui a14, %3, 0, 8" TAB
         "s8i a14, %4, 4" TAB
         "extui a14, %3, 8, 8" TAB
         "s8i a14, %4, 5" TAB
         "extui a14, %3, 16, 8" TAB
         "s8i a14, %4, 6" TAB
         "l8ui %0, %4, 0" TAB
         "l8ui %1, %4, 1" TAB
         :"=r"(fuse_buf[0]), "=r"(fuse_buf[1]):"r"(val1), "r"(val2), "r"(fuse_buf)
      );

      if (fuse_buf[0]^crc8_le(0, &fuse_buf[1], 6))
         return UPOS_MAC_CRC8_ERROR;

      if (fuse_buf[1]^crc8_le(0, &fuse_buf[2], 5))
         return UPOS_MAC_READ_EFUSE;

      return (int)esp_base_mac_addr_set(&fuse_buf[1]);

   }

   return UPOS_MAC_OK;
}

int IRAM_ATTR upos_init_nvs()
{
   int err;
   err = (int)nvs_flash_init();

   if (err == (int)ESP_ERR_NVS_NO_FREE_PAGES || err == (int)ESP_ERR_NVS_NEW_VERSION_FOUND) {
      if ((err = (int)nvs_flash_erase()) != (int)ESP_OK)
         return err;

      err = (int)nvs_flash_init();

   }

   return err;
}

