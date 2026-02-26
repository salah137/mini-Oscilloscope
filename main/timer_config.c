#include "driver/gptimer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TIMER_FRQ     1000000 // 1 MHz resolution (1 tick = 1 microsecond)
#define ALARM_TICKS   10000   // Alarm every 1000 ticks (1ms interval)

static TaskHandle_t my_task_handle;
static gptimer_handle_t gptimer = NULL;

static bool IRAM_ATTR alarm_cb(gptimer_handle_t timer,
                               const gptimer_alarm_event_data_t *edata,
                               void *user_ctx)
{
    BaseType_t hp = pdFALSE;
    if (my_task_handle != NULL) {
            vTaskNotifyGiveFromISR(my_task_handle, &hp);
        }
    return hp == pdTRUE;
}

void init_timer(TaskHandle_t task_handle)
{   
    my_task_handle = task_handle;
    
    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_FRQ,
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&config, &gptimer));

    gptimer_alarm_config_t alarm_cfg = {
        .alarm_count = ALARM_TICKS,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };


    gptimer_event_callbacks_t cbs = {
        .on_alarm = alarm_cb,
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_cfg));

    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}