#ifndef ESP_EVENT_H
 #define ESP_EVENT_H 
 #include "esp_event.h"
#endif
#define UPOS_EVENT_TAG_STRING_MAX (size_t)96
typedef enum upos_events_err_t {
   UPOS_EVENTS_OK = 0,
   UPOS_EVENTS_FAIL = 30000,
   UPOS_EVENT_ALREADY_EXISTS,
   UPOS_EVENT_INIT_MALLOC,
   UPOS_EVENTS_INIT_FAIL
} UPOS_EVENTS_ERR;

typedef enum upos_register_event_err_t {
   UPOS_REGISTER_EVT_OK = 0,
   UPOS_REGISTER_EVT_MISSING_FUNCTION_NAME = 30100,
   UPOS_REGISTER_MISSING_FUNCTION,
   UPOS_REGISTER_EVT_CREATE_TASK
} UPOS_REGISTER_EVENTS_ERR;

typedef void (*fn_evt)(void *);

typedef struct upos_register_events_t {
   int err;
   const char *tag;
   esp_event_base_t event_base;
   int32_t event_id;
   esp_event_handler_t event_handler;
   void *arg;
   fn_evt ev_cb;
} upos_register_events;

int upos_init_events();
void *upos_get_global_events_group();
void upos_unregister_event(void *fn_handle);
int upos_register_event(void **, fn_evt, void *, const char *, uint32_t);
void UPOS_WAIT(TickType_t);

#define UPOS_TIME_EVENT_SECONDS(time) time*1000/portTICK_RATE_MS
#define UPOS_TIME_EVENT_MILLISECONDS(time) time/portTICK_RATE_MS
#define UPOS_TIME_EVENT_MICROSECONDS(time) pdMS_TO_TICKS(time)
#define UPOS_TIME_WAIT_FOR_EVER portMAX_DELAY
#define UPOS_MIN_EVENT_TO_WAIT_US 200
#define UPOS_TIME_EVENT_EPOCH_MINUTES(time) time*60
#define UPOS_TIME_EVENT_EPOCH_HOURS(time) 60*UPOS_TIME_EVENT_EPOCH_MINUTES(time)
#define UPOS_TIME_EVENT_EPOCH_DAY(time) 24*UPOS_TIME_EVENT_EPOCH_HOURS(time)
#define UPOS_TIME_EVENT_EPOCH_WEEK(time) 7*UPOS_TIME_EVENT_EPOCH_DAY(time)
#define UPOS_TIME_EVENT_EPOCH_YEAR(time) 365*UPOS_TIME_EVENT_EPOCH_DAY(time)


