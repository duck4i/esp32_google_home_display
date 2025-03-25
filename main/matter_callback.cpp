#include "matter_callback.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define WAIT_TICKS 0 // non blocking

static QueueHandle_t g_ui_update_queue;

void ghome_matter_events_init(void)
{
    g_ui_update_queue = xQueueCreate(64, sizeof(matter_ui_update_msg_t));
}

void ghome_matter_events_enqueue(matter_ui_update_msg_t msg)
{
    xQueueSend(g_ui_update_queue, &msg, WAIT_TICKS);
}

void ghome_matter_events_consume(matter_event_cb cb)
{
    matter_ui_update_msg_t msg;
    if (xQueueReceive(g_ui_update_queue, &msg, WAIT_TICKS) == pdTRUE)
    {
        if (cb != nullptr)
        {
            cb(msg);
        }
    }
}
