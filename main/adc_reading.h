#ifndef ADC_READING_H
#define ADC_READING_H

#include "esp_adc/adc_continuous.h"
#include <stdint.h>
#define READ_LEN 1024

typedef struct {
  float frequency;
  float max_value;
  float avg;
  float min;
  int channel;
  uint16_t values[READ_LEN];
  int initialized;
} sample_t;



void adc_continous_init(sample_t* sample);
void read_adc_continuous(uint16_t *data, int channel);
void get_sample(sample_t *s);

#endif
