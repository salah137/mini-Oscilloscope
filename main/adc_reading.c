#include "adc_reading.h"
#include "driver/adc.h"
#include "driver/adc_types_legacy.h"
#include "esp_adc/adc_continuous.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/adc_types.h"
#include <stdbool.h>

#define READ_LEN 2048

#define SAMPLE_FRQ 500 * 1000
#define READ_PERIOD 1 / SAMPLE_FRQ

#define CHANNEL_1 ADC1_CHANNEL_6
#define CHANNEL_2 ADC1_CHANNEL_5

adc_continuous_handle_t adc_handle;

void adc_continous_init(sample_t *sample) {

  adc_continuous_handle_cfg_t handle_cfg = {.max_store_buf_size = 4096,
                                            .conv_frame_size = READ_LEN};

  ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc_handle));

  adc_continuous_config_t adc_cfg = {
      .sample_freq_hz = SAMPLE_FRQ,
      .conv_mode = ADC_CONV_SINGLE_UNIT_1,
      .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
  };

  adc_digi_pattern_config_t pattern[2] = {{.atten = ADC_ATTEN_DB_12,
                                           .channel = CHANNEL_1,
                                           .bit_width = ADC_BITWIDTH_12,
                                           .unit = ADC_UNIT_1},
                                          {.atten = ADC_ATTEN_DB_12,
                                           .channel = CHANNEL_2,
                                           .bit_width = ADC_BITWIDTH_12,
                                           .unit = ADC_UNIT_1}};
  adc_cfg.adc_pattern = pattern;
  adc_cfg.pattern_num = 2;

  ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &adc_cfg));

  ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
  sample->initialized = 1;
}

void read_adc_continuous(uint16_t *data, int channel) {
  uint32_t ret_len = 0;
  uint8_t buffer[READ_LEN * 4];

  esp_err_t ret =
      adc_continuous_read(adc_handle, buffer, READ_LEN, &ret_len, 100);

  if (ret == ESP_OK) {
    int j = 0;
    for (int i = 0; i < ret_len; i += SOC_ADC_DIGI_RESULT_BYTES) {
      adc_digi_output_data_t *p = (adc_digi_output_data_t *)&buffer[i];

      if (channel == p->type1.channel) {
        data[j++] = p->type1.data;
      }
      if (j == READ_LEN)
        return;
    }
  }
}

static float calculate_frequency_zero_crossing(sample_t *s) {
  uint32_t sum = 0;
  for (int i = 0; i < READ_LEN; i++) {
    sum += s->values[i];
  }
  float offset = (float)sum / READ_LEN;
  const int hysteresis = 50;
  int crossings = 0;

  bool above = s->values[0] > offset;

  for (int i = 0; i < READ_LEN; i++) {
    if (above && (s->values[i] < (offset - hysteresis))) {
      crossings++;
      above = false;
    }
    if (!above && (s->values[i] > (offset - hysteresis))) {
      crossings++;
      above = true;
    }
  }

  float cycles = (float)crossings / 2;
  float time = (float)READ_LEN / SAMPLE_FRQ;

  return time / cycles;
}

void get_sample(sample_t *s) {
    // 1. Clear the values first to prevent "ghost" data
    memset(s->values, 0, sizeof(s->values));

    // 2. Perform the read
    read_adc_continuous(s->values, s->channel);

    // 3. Use standard 'int' for the search to avoid unsigned weirdness
    int current_max = 0; 
    int current_min = 4095;

    for (int i = 0; i < READ_LEN; i++) {
        // Sanity check: ensure the ADC value is within 12-bit range
        if (s->values[i] > 4095) continue; 

        if (s->values[i] > current_max) current_max = s->values[i];
        if (s->values[i] < current_min) current_min = s->values[i];
    }

    // 4. Calculate using explicit float casting
    s->max_value = ((float)current_max * 3.3f) / 4095.0f;
    s->min = ((float)current_min * 3.3f) / 4095.0f;
    s->avg = (s->max_value + s->min) / 2.0f;

    // 5. Correct Logging (Use %d for ints, %f for floats)
    ESP_LOGI("ADC_DEBUG", "Raw Max: %d, Volts Max: %.2fV", current_max, s->max_value);

    s->frequency = calculate_frequency_zero_crossing(s);
}