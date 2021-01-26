#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "upos_system.h"
#include "esp_wifi.h"
#include "upos_wifi.h"
#include "upos_events.h"
#include "esp_log.h"

#define UPOS_MAX_WIFI_SSID (size_t) sizeof(((wifi_config_t *)0)->sta.ssid)
#define UPOS_MAX_WIFI_PASSWORD (size_t) sizeof(((wifi_config_t *)0)->sta.password)

#define GOT_IPV4_BIT (1<<0)
#define GOT_IPV6_BIT (1<<1)
#define WIFI_ENABLED (1<<2)
#define WIFI_CONNECTED (1<<3)
#define WIFI_LOCK_EVENT (1<<4)

#define CONNECTED_BITS (GOT_IPV4_BIT|GOT_IPV6_BIT)

#define TAG "UPOS_WIFI"

static volatile int _upos_wifi_error;
static volatile char *_upos_wifi_error_tag = UPOS_NULL_STRING;
static volatile upos_wifi_cb upos_wifi_on_error_cb = NULL;
static char DRAM_ATTR _ssid[UPOS_MAX_WIFI_SSID];
static ip4_addr_t _ip_addr;
static ip6_addr_t _ipv6_addr;

static volatile char DRAM_ATTR _ip_str[64] = {0};
static volatile char DRAM_ATTR _ip6_str[64] = {0};

static upos_register_events *_upos_wifi_register_events = NULL;
static volatile char DRAM_ATTR _upos_wifi_evt_error[UPOS_EVENT_TAG_STRING_MAX];

#define UPOS_WIFI_ON_DISCONNECT_IDX (size_t)0
#define UPOS_WIFI_ON_GOT_IP_IDX (size_t)1
#define UPOS_WIFI_ON_WIFI_CONNECT_IDX (size_t)2
#define UPOS_WIFI_ON_GOT_IPV6_IDX (size_t)3
#define UPOS_WIFI_EVENTS_NUM (size_t)4
#define URL_WIFI_EVENTS_BUF_SZ (size_t)UPOS_WIFI_EVENTS_NUM*sizeof(upos_register_events)

#define CLEAR_SSID memset(_ssid, 0, sizeof(_ssid))

static int IRAM_ATTR upos_wifi_register_events();
static int IRAM_ATTR upos_wifi_unregister_events();
void IRAM_ATTR on_wifi_connect(void *, esp_event_base_t, int32_t, void *);
void IRAM_ATTR on_wifi_disconnect(void *, esp_event_base_t, int32_t, void *);
void IRAM_ATTR on_got_ip(void *, esp_event_base_t, int32_t, void *);
void IRAM_ATTR on_got_ipv6(void *, esp_event_base_t, int32_t, void *);

inline int IRAM_ATTR upos_wait_connect(TickType_t time_to_wait)
{

   return (int)(xEventGroupWaitBits(
      (EventGroupHandle_t)upos_get_global_events_group(),
      WIFI_CONNECTED,
      pdFALSE,
      pdFALSE,
      time_to_wait)&WIFI_CONNECTED);

}

inline int IRAM_ATTR upos_is_wifi_enabled()
{
   return (int)(xEventGroupWaitBits(
      (EventGroupHandle_t)upos_get_global_events_group(),
      WIFI_ENABLED,
      pdFALSE,
      pdFALSE,
      UPOS_TIME_EVENT_MILLISECONDS(250))&WIFI_ENABLED);

}

int IRAM_ATTR upos_wifi_start(const char *ssid, const char *password, UPOS_WIFI_CB_CTX *wifi_ctx)
{

   int err;
   size_t sz_tmp;
   wifi_config_t wifi_config;

   if (!(sz_tmp = strnlen(ssid, UPOS_MAX_WIFI_SSID)))
      return UPOS_WIFI_SSID_EMPTY_STRING;
   else if (sz_tmp == UPOS_MAX_WIFI_SSID)
      return UPOS_WIFI_SSID_TOO_LONG;

   if (strnlen(password, UPOS_MAX_WIFI_PASSWORD) == UPOS_MAX_WIFI_PASSWORD)
      return UPOS_WIFI_PASSWORD_TOO_LONG;

   if ((err = (int)esp_event_loop_create_default()) != (int)ESP_OK)
      return err;

   if ((err = upos_wifi_stop_util(-1)))
      return err;

   if (!(_upos_wifi_register_events = malloc(URL_WIFI_EVENTS_BUF_SZ)))
      return UPOS_EVENT_INIT_MALLOC;

   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

   if ((err = (int)esp_wifi_init(&cfg)) != (int)ESP_OK) {
      free(_upos_wifi_register_events);
      _upos_wifi_register_events = NULL;
      return err;
   }

   memset(_upos_wifi_register_events, 0, URL_WIFI_EVENTS_BUF_SZ);

   if ((err = upos_wifi_register_events(
      &_upos_wifi_register_events[UPOS_WIFI_ON_DISCONNECT_IDX],
      "on_wifi_disconnect",
      WIFI_EVENT,
      WIFI_EVENT_STA_DISCONNECTED,
      &on_wifi_disconnect,
      wifi_ctx,
      UPOS_WIFI_EVENT_CALLBACK(on_disconnect)
   ))) goto upos_wifi_start_EXIT1;

   if ((err = upos_wifi_register_events(
      &_upos_wifi_register_events[UPOS_WIFI_ON_GOT_IP_IDX],
      "on_got_ip",
      IP_EVENT,
      IP_EVENT_STA_GOT_IP,
      &on_got_ip,
      wifi_ctx,
      UPOS_WIFI_EVENT_CALLBACK(on_ipv4)
   ))) goto upos_wifi_start_EXIT1;

   if ((err = upos_wifi_register_events(
      &_upos_wifi_register_events[UPOS_WIFI_ON_WIFI_CONNECT_IDX],
      "on_wifi_connect",
      WIFI_EVENT,
      WIFI_EVENT_STA_CONNECTED,
      &on_wifi_connect,
      wifi_ctx,
      UPOS_WIFI_EVENT_CALLBACK(on_connect)
   ))) goto upos_wifi_start_EXIT1;

   if ((err = upos_wifi_register_events(
      &_upos_wifi_register_events[UPOS_WIFI_ON_GOT_IPV6_IDX],
      "on_got_ipv6",
      IP_EVENT,
      IP_EVENT_GOT_IP6,
      &on_got_ipv6,
      wifi_ctx,
      UPOS_WIFI_EVENT_CALLBACK(on_ipv6)
   ))) goto upos_wifi_start_EXIT1;

   if ((err = (int)esp_wifi_set_storage(WIFI_STORAGE_RAM)))
      goto upos_wifi_start_EXIT1;

   if ((err = (int)esp_wifi_set_mode(WIFI_MODE_STA)) != (int)ESP_OK)
      goto upos_wifi_start_EXIT1;

   strcpy((char *)wifi_config.sta.ssid, ssid);
   strcpy((char *)wifi_config.sta.password, password);

   if ((err = (int)esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)) != (int)ESP_OK)
      goto upos_wifi_start_EXIT2;

   if ((err = (int)esp_wifi_start()) != (int)ESP_OK)
      goto upos_wifi_start_EXIT2;

   xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_ENABLED);

   if ((err = (int)esp_wifi_connect()) != (int)ESP_OK)
      goto upos_wifi_start_EXIT2;

   strcpy((char *)_ssid, ssid);
   memset(&wifi_config, 0, sizeof(wifi_config));
   return UPOS_WIFI_OK;

upos_wifi_start_EXIT2:
   memset(&wifi_config, 0, sizeof(wifi_config));

upos_wifi_start_EXIT1:
   if (upos_wifi_unregister_events())
      err = -err;

   return err;

}

int IRAM_ATTR upos_wifi_stop_util(int destroy)
{
    EventBits_t upos_evt;

   if (destroy)
      if ((_upos_wifi_error = upos_wifi_unregister_events()))
         return _upos_wifi_error;

   xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT);

    if ((upos_evt = xEventGroupWaitBits(
       (EventGroupHandle_t)upos_get_global_events_group(), WIFI_ENABLED|WIFI_CONNECTED|CONNECTED_BITS,
       pdFALSE,
       pdFALSE,
       UPOS_TIME_EVENT_MICROSECONDS(UPOS_MIN_EVENT_TO_WAIT_US))&(WIFI_ENABLED|WIFI_CONNECTED|CONNECTED_BITS))
    ) {

      if (upos_evt&WIFI_CONNECTED)
         if ((_upos_wifi_error = (int)esp_wifi_disconnect()) != (int)ESP_OK) {
            _upos_wifi_error_tag = "Error on disconnect upos_wifi_stop_util()";
            return _upos_wifi_error;
         }

       if (destroy)
          if (upos_evt&WIFI_ENABLED) {
             if ((_upos_wifi_error = (int)esp_wifi_stop()) != (int)ESP_OK) {
                _upos_wifi_error_tag = "Error trying to stop wifi upos_wifi_stop_util()";
                return _upos_wifi_error;
             }

             if ((_upos_wifi_error = (int)esp_wifi_deinit()) != (int)ESP_OK) {
                _upos_wifi_error_tag = "Error on deinit wifi upos_wifi_stop_util()";
                return _upos_wifi_error;
             }

             if ((_upos_wifi_error = (int)esp_event_loop_delete_default())) {
                _upos_wifi_error_tag = "Error on esp_event_loop_delete_default()";
                return _upos_wifi_error;
             }

             xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_ENABLED);
             CLEAR_SSID;
          }
   }

   xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT|CONNECTED_BITS);

   _upos_wifi_error_tag = UPOS_NULL_STRING;
   return ((_upos_wifi_error = 0));

}

static int IRAM_ATTR is_wifi_event_locked_util(EventBits_t *upos_evt)
{ 
   return ((*upos_evt = xEventGroupWaitBits(
       (EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT|WIFI_CONNECTED|WIFI_ENABLED|CONNECTED_BITS,
       pdFALSE,
       pdFALSE,
       UPOS_TIME_EVENT_MICROSECONDS(UPOS_MIN_EVENT_TO_WAIT_US)))&WIFI_LOCK_EVENT);
}

void IRAM_ATTR on_wifi_disconnect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
   static UPOS_WIFI_EVENT_CTX on_disconnect_event;
   upos_register_events *on_disconnect_event_register = &_upos_wifi_register_events[UPOS_WIFI_ON_DISCONNECT_IDX];
   EventBits_t upos_evt;

   if (is_wifi_event_locked_util(&upos_evt))
      return;

   if ((on_disconnect_event.err = (_upos_wifi_error = (int)esp_wifi_connect())) == (int)ESP_OK) {

      if (on_disconnect_event_register->ev_cb) {

         if (upos_evt&WIFI_CONNECTED) {
            on_disconnect_event.event_name = "on_reconnect_event";
            on_disconnect_event.event_type = WIFI_EV_RECONNECT;
         } else {
            on_disconnect_event.event_name = "on_disconnect_event";
            on_disconnect_event.event_type = WIFI_EV_DISCONNECT;
         }
         on_disconnect_event.initial_ctx = on_disconnect_event_register->arg;
         on_disconnect_event_register->ev_cb(&on_disconnect_event);
      }

      xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_CONNECTED);

      _upos_wifi_error_tag = UPOS_NULL_STRING;

      return;
   }

   if (upos_evt&WIFI_CONNECTED) {
      on_disconnect_event.event_name = (const char *)(_upos_wifi_error_tag = "on_reconnect_event_error");
      on_disconnect_event.event_type = WIFI_EV_RECONNECT_ERROR;
   } else {
      on_disconnect_event.event_name = (const char *)(_upos_wifi_error_tag = "on_disconnect_event_error");
      on_disconnect_event.event_type = WIFI_EV_DISCONNECT_ERROR;
   }

   if (upos_wifi_on_error_cb) {
      on_disconnect_event.initial_ctx = on_disconnect_event_register->arg;
      upos_wifi_on_error_cb(&on_disconnect_event);
   } else if (on_disconnect_event_register->ev_cb) {
      on_disconnect_event.initial_ctx = on_disconnect_event_register->arg;
      on_disconnect_event_register->ev_cb(&on_disconnect_event);
   }

}

void IRAM_ATTR on_wifi_connect(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

   static UPOS_WIFI_EVENT_CTX on_connect_event;
   upos_register_events *on_connect_event_register = &_upos_wifi_register_events[UPOS_WIFI_ON_WIFI_CONNECT_IDX];
   EventBits_t upos_evt;

   if (is_wifi_event_locked_util(&upos_evt))
      return;

   if ((on_connect_event.err = (_upos_wifi_error = (int)tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA))) == (int)ESP_OK) {
      if (on_connect_event_register->ev_cb) {
         on_connect_event.event_name = "on_connect_event";
         on_connect_event.event_type = WIFI_EV_CONNECT;
         on_connect_event.initial_ctx = on_connect_event_register->arg;
         on_connect_event_register->ev_cb(&on_connect_event);
      }
      _upos_wifi_error_tag = UPOS_NULL_STRING;
      return;
   }

   on_connect_event.event_name = (const char *)(_upos_wifi_error_tag = "on_connect_event_error");
   on_connect_event.event_type = WIFI_EV_CONNECT_ERROR;

   if (upos_wifi_on_error_cb)
      upos_wifi_on_error_cb(&on_connect_event);
   else if (on_connect_event_register->ev_cb) {
      on_connect_event.initial_ctx = on_connect_event_register->arg;
      on_connect_event_register->ev_cb(&on_connect_event);
   }

}

void IRAM_ATTR on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

   static UPOS_WIFI_EVENT_CTX on_got_ip_event;
   upos_register_events *on_got_ip_event_register = &_upos_wifi_register_events[UPOS_WIFI_ON_GOT_IP_IDX];
   EventBits_t upos_evt;

   if (is_wifi_event_locked_util(&upos_evt))
      return;

   if (upos_evt&GOT_IPV6_BIT) {
      xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_CONNECTED);
      xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), CONNECTED_BITS);
   } else {
      xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), GOT_IPV4_BIT);
      xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_CONNECTED);
   }

    memcpy(&_ip_addr, &((ip_event_got_ip_t *)event_data)->ip_info.ip, sizeof(_ip_addr));

   if (on_got_ip_event_register->ev_cb) {
      on_got_ip_event.event_name = "on_got_ip_event";
      on_got_ip_event.event_type = WIFI_EV_GOT_IP;
      on_got_ip_event.initial_ctx = on_got_ip_event_register->arg;
      on_got_ip_event_register->ev_cb(&on_got_ip_event);
   }

    _upos_wifi_error_tag = UPOS_NULL_STRING;
    _upos_wifi_error = 0;
}

void IRAM_ATTR on_got_ipv6(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
   static UPOS_WIFI_EVENT_CTX on_got_ip6_event;
   upos_register_events *on_got_ip6_event_register = &_upos_wifi_register_events[UPOS_WIFI_ON_GOT_IPV6_IDX];
   EventBits_t upos_evt;

   if (is_wifi_event_locked_util(&upos_evt))
      return;

   if (upos_evt&GOT_IPV4_BIT) {
      xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_CONNECTED);
      xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), CONNECTED_BITS);
   } else {
      xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), GOT_IPV6_BIT);
      xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_CONNECTED);
   }

   memcpy(&_ipv6_addr, &((ip_event_got_ip6_t *)event_data)->ip6_info.ip, sizeof(_ipv6_addr));

   if (on_got_ip6_event_register->ev_cb) {
      on_got_ip6_event.event_name = "on_got_ipv6_event";
      on_got_ip6_event.event_type = WIFI_EV_GOT_IPV6_IP;
      on_got_ip6_event.initial_ctx = on_got_ip6_event_register->arg;
      on_got_ip6_event_register->ev_cb(&on_got_ip6_event);
   }

    _upos_wifi_error_tag = UPOS_NULL_STRING;
    _upos_wifi_error = 0;
}

static int IRAM_ATTR upos_wifi_register_events(
   upos_register_events *event,
   const char *tag,
   esp_event_base_t event_base,
   int32_t event_id,
   esp_event_handler_t event_handler,
   UPOS_WIFI_CB_CTX *cb,
   size_t cb_offset
)
{

   _upos_wifi_error_tag = "upos_wifi_register_events error";

   if (!event)
      return (_upos_wifi_error = UPOS_EVENTS_FAIL);

   if (event->event_handler)
      return (_upos_wifi_error = UPOS_EVENT_ALREADY_EXISTS);

   if ((_upos_wifi_error = (int)esp_event_handler_register(
      event->event_base = event_base, 
      event->event_id = event_id,
      event->event_handler = event_handler,
      NULL) != (int)ESP_OK))
      return _upos_wifi_error;

   event->err = 0;
   event->tag = tag;

   if (cb) {
      asm volatile (
         "sub %3, %2, %3" TAB
         "l32i.n %2, %2, 0" TAB
         "s32i.n %2, %0, 0" TAB
         "l32i.n %3, %3, 0" TAB
         "s32i.n %3, %1, 0" TAB
      ::"r"(&event->ev_cb), "r"(&event->arg), "r"(&((uint8_t *)cb)[cb_offset]), "r"(sizeof(void *)));
   }

   _upos_wifi_error_tag = UPOS_NULL_STRING;
   return _upos_wifi_error;
}

static int IRAM_ATTR upos_wifi_unregister_events()
{
   int err;

   _upos_wifi_error = 0;
   _upos_wifi_error_tag = UPOS_NULL_STRING;

   if (_upos_wifi_register_events) {

      xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT);

      for (err = 0; err < UPOS_WIFI_EVENTS_NUM;) {
         if (_upos_wifi_register_events->event_handler)
            if (
                 (_upos_wifi_error = (int)esp_event_handler_unregister(

                     _upos_wifi_register_events->event_base,
                     _upos_wifi_register_events->event_id,
                     _upos_wifi_register_events->event_handler

                 )) != ESP_OK
            ) {

               _upos_wifi_error_tag = strncat(

                  strcpy((char *)_upos_wifi_evt_error, "UNREGISTER_EVENT_ERROR: "),
                  (_upos_wifi_register_events->tag)?_upos_wifi_register_events->tag:"",
                  UPOS_EVENT_TAG_STRING_MAX-1

               );

               _upos_wifi_evt_error[UPOS_EVENT_TAG_STRING_MAX-1] = 0;

               break;

            }
         err++;
      }

      if (err == UPOS_WIFI_EVENTS_NUM) {
         memset(_upos_wifi_register_events, 0, URL_WIFI_EVENTS_BUF_SZ);
         free(_upos_wifi_register_events);
         _upos_wifi_register_events = NULL;
      }

      xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT);
   }

   return _upos_wifi_error;
}

const char *get_ip_string_util(int ip_type)
{
   const char *ip_type_str;

   if (xEventGroupWaitBits(
       (EventGroupHandle_t)upos_get_global_events_group(), WIFI_CONNECTED|CONNECTED_BITS,
       pdFALSE,
       pdFALSE,
       UPOS_TIME_EVENT_MICROSECONDS(UPOS_MIN_EVENT_TO_WAIT_US))&(WIFI_CONNECTED|CONNECTED_BITS)) {

       if (ip_type)
          sprintf((char *)(ip_type_str = (const char *)_ip_str), "IPv4:"IPSTR, IP2STR(&_ip_addr));
       else
          sprintf((char *)(ip_type_str = (const char *)_ip6_str), "IPv6:"IPV6STR, IPV62STR(_ipv6_addr));

       return ip_type_str;
   }

   return UPOS_NULL_STRING;
}

int upos_get_wifi_error()
{
   return _upos_wifi_error;
}

const char *upos_get_error_message()
{
   return (const char *)_upos_wifi_error_tag;
}

void IRAM_ATTR upos_wifi_set_event_error_cb(upos_wifi_cb callback)
{
   xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT);
   upos_wifi_on_error_cb = callback;
   xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT);
}

void IRAM_ATTR upos_wifi_delete_event_error_cb()
{
   xEventGroupSetBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT);
   upos_wifi_on_error_cb = NULL;
   xEventGroupClearBits((EventGroupHandle_t)upos_get_global_events_group(), WIFI_LOCK_EVENT);
}

const char *get_ssid()
{
   return (const char *)_ssid;
}

