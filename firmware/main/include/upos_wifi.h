#include "esp_event.h"

typedef enum upos_wifi_err_t {
   UPOS_WIFI_OK = 0,
   UPOS_WIFI_SSID_TOO_LONG = 5200,
   UPOS_WIFI_PASSWORD_TOO_LONG,
   UPOS_WIFI_SSID_EMPTY_STRING
} UPOS_WIFI_ERR;

typedef struct upos_wifi_status_t {
   int err;
   const char *tag;
} UPOS_WIFI;

typedef void (*upos_wifi_cb)(void *);
typedef struct upos_wifi_cb_ctx_t {
   void *on_connect_ctx;
   upos_wifi_cb on_connect;
   void *on_disconnect_ctx;
   upos_wifi_cb on_disconnect;
   void *on_ipv4_ctx;
   upos_wifi_cb on_ipv4;
   void *on_ipv6_ctx;
   upos_wifi_cb on_ipv6;
} UPOS_WIFI_CB_CTX;

typedef enum upos_wifi_event_type_e {
   WIFI_EV_DISCONNECT = 1,
   WIFI_EV_DISCONNECT_ERROR,
   WIFI_EV_RECONNECT,
   WIFI_EV_RECONNECT_ERROR,
   WIFI_EV_CONNECT,
   WIFI_EV_CONNECT_ERROR,
   WIFI_EV_GOT_IP,
   WIFI_EV_GOT_IPV6_IP
} UPOS_WIFI_EVENT_ENUM;

typedef struct upos_wifi_event_cb_ctx_t {
   int err;
   const char *event_name;
   uint32_t event_type;
   void *initial_ctx;
//   void *ctx;
} UPOS_WIFI_EVENT_CTX;


#define UPOS_WIFI_EVENT_CALLBACK(param) ((size_t)offsetof(struct upos_wifi_cb_ctx_t, param))

int upos_wifi_start(const char *, const char *, UPOS_WIFI_CB_CTX *);
int upos_wifi_disconnect();
//int upos_wifi_stop();
const char *get_ip_string();
const char *get_ip6_string();
int upos_wait_connect(TickType_t);
int upos_get_wifi_error();
const char *upos_get_error_message();
const char *get_ip_string_util(int);
int upos_wifi_stop_util(int destroy);
void upos_wifi_set_event_error_cb(upos_wifi_cb);
void upos_wifi_delete_event_error_cb();
const char *get_ssid();
int upos_is_wifi_enabled();
#define _UPOS_IP6 (int)0
#define _UPOS_IP4 (int)1
#define upos_get_ip_string() get_ip_string_util(_UPOS_IP4)
#define upos_get_ip6_string() get_ip_string_util(_UPOS_IP6)

#define upos_wifi_disconnect() upos_wifi_stop_util(0)
#define upos_wifi_stop() upos_wifi_stop_util(-2)


