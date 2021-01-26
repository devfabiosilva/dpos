#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "upos_events.h"
#include "upos_conf.h"

#define TAG "UPOS_EVENTS"

static EventGroupHandle_t __upos_event_group = NULL;

int IRAM_ATTR upos_init_events()
{
   if ((__upos_event_group = xEventGroupCreate()))
      return UPOS_EVENTS_OK;

   return UPOS_EVENTS_INIT_FAIL;
}

void IRAM_ATTR *upos_get_global_events_group()
{
   return (void *)__upos_event_group;
}

int IRAM_ATTR upos_register_event(void **fn_handle, fn_evt fn, void *fn_ctx, const char *fn_name, uint32_t priority)
{
   int err;

   if (!fn_name)
      return UPOS_REGISTER_EVT_MISSING_FUNCTION_NAME;

   if (!fn)
      return UPOS_REGISTER_MISSING_FUNCTION;

   err = UPOS_REGISTER_EVT_OK;
   if (xTaskCreate(fn, fn_name, UPOS_MONITORE_STACK_SIZE, fn_ctx, (UBaseType_t)priority, (TaskHandle_t)fn_handle) != pdPASS)
      err = UPOS_REGISTER_EVT_CREATE_TASK;

   return err;
}

inline void IRAM_ATTR upos_unregister_event(void *fn_handle)
{
   vTaskDelete(fn_handle);
}

inline void IRAM_ATTR UPOS_WAIT(TickType_t wait)
{
   vTaskDelay(wait);
}

