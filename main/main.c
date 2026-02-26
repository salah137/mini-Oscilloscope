#include "adc_reading.h"
#include "draw_ui.h"
#include "drawing.h"
#include "driver/adc_types_legacy.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "i2c_connection.h"
#include "portmacro.h"
#include "timer_config.h"
#include "driver/dac.h"
#include "math.h"

#define DAC_CHANNEL DAC_CHANNEL_1   // GPIO25
#define SAMPLES     100             // samples per period
#define FREQUENCY   1000            // 1 kHz


static TaskHandle_t fetching_task_handle;
static TaskHandle_t draw_ui_task_handle;
static sample_t sample1;
static sample_t sample2;

void draw_ui_task() {
  screen_t oled;

  init(21, 22, &oled);

  while (1) {    
      ssd1306_clear_fb();
      draw_grid();
      write_data(sample1.max_value, sample1.frequency);
      draw_graph((int *)sample1.values,sample1.max_value,sample1.min);
      ssd1306_update(&oled);
      vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void test(){
    dac_output_enable(DAC_CHANNEL);

    uint8_t sine_table[SAMPLES];

    // Generate lookup table (0–255 for 8-bit DAC)
    for (int i = 0; i < SAMPLES; i++) {
        float angle = (2 * M_PI * i) / SAMPLES;
        sine_table[i] = (uint8_t)((sin(angle) + 1.0) * 127.5);
    }

    const TickType_t delay_ticks =
        pdMS_TO_TICKS(1000 / (FREQUENCY * SAMPLES));

    while (1) {
        for (int i = 0; i < SAMPLES; i++) {
            dac_output_voltage(DAC_CHANNEL, sine_table[i]);
            vTaskDelay(delay_ticks);
        }
    }

}

void fetching_task() {
    sample1.channel = ADC1_CHANNEL_6;
  while (1) {
    uint32_t thread_notification = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (thread_notification > 0 && sample1.initialized == 1) {
      get_sample(&sample1);
      ESP_LOGI("data => ", "%d", sample1.values[0]);
      xTaskNotifyGive(draw_ui_task_handle);
    } else {
      ESP_LOGI("data => ", "noo noo");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void app_main(void) {
  xTaskCreate(fetching_task, "fetching", 4096 * 4, NULL, 6,
              &fetching_task_handle);
  init_timer(fetching_task_handle);
  adc_continous_init(&sample1);

  xTaskCreate(draw_ui_task, "draw_ui", 4096, NULL, 5, &draw_ui_task_handle);
  xTaskCreate(test,"test", 2048, NULL, 4, NULL);
}
