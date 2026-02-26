#include "drawing.h"
#include "esp_log.h"
#include "i2c_connection.h"
#include "string.h"
#include <stdio.h>

#define READ_LEN 2048

void draw_grid() {
  for (int i = 0; i < 130; i += 10) {
    ssd1306_draw_verticale_line(i, 15, 60, 1, 1);
  }

  for (int i = 16; i < 100; i += 10) {
    ssd1306_draw_horizental_line(1, i, 127, 1, 1);
  }
}

void draw_graph(uint16_t *data, float max, float min) {
  float range = max - min;
  int scale = range/45;
  
  for (int i = 0; i < READ_LEN; i++) {
    float percentage = ((float)data[i] / 4095.0f) * scale;
    int y_pos = (45 - (int)(percentage * 45.0f)) + 17;

    ssd1306_draw_horizental_line(i, y_pos, 2, 2, 1);
  }
}

void write_data(float max, float frq) {
  char frq_str[20];
  char max_str[20];

  snprintf(frq_str, sizeof(frq_str), "%.2f HZ", frq);
  snprintf(max_str, sizeof(max_str), "%.2f V", max);

  ESP_LOGI("max =>", "%f", max);

  draw_string(2, 1, frq_str, 1, 1);
  draw_string(80, 1, max_str, 1, 1);
}
