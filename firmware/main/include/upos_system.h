
#include "tcpip_adapter.h"

#define upos_init_tcp_ip() tcpip_adapter_init()
int IRAM_ATTR upos_init_nvs();
int IRAM_ATTR upos_set_mac(uint8_t *, int);
#define TAB "\n\t"
#define UPOS_NULL_STRING ""

typedef enum upos_mac_err_t {
   UPOS_MAC_OK = 0,
   UPOS_MAC_READ_EFUSE = 1100,
   UPOS_MAC_CRC8_ERROR
} UPOS_MAC_ERR;

