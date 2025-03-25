#include "matter_callback.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

static QueueHandle_t g_ui_update_queue;

void ghome_matter_events_init(void)
{
    g_ui_update_queue = xQueueCreate(24, sizeof(matter_ui_update_msg_t));
}

void ghome_matter_events_enqueue(matter_ui_update_msg_t msg)
{
    xQueueSend(g_ui_update_queue, &msg, 0);
}

void ghome_matter_events_consume(matter_event_cb cb)
{
    matter_ui_update_msg_t msg;
    if (xQueueReceive(g_ui_update_queue, &msg, 0))
    {
        if (cb != nullptr)
        {
            cb(msg);
        }
    }
}
