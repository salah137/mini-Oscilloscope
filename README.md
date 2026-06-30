# ESP32 Mini Oscilloscope

ESP32/FreeRTOS project: samples a signal via continuous ADC, computes Vmax/Vmin/frequency (zero-crossing), and draws the waveform live on an SSD1306 OLED. Includes a DAC sine generator for self-test.

## Hardware
- ESP32, SSD1306 OLED (I2C SDA=21, SCL=22)
- ADC1 CH6/CH5 input, DAC1 (GPIO25) test output

## Tasks
- `fetching_task` — timer-triggered ADC read + stats
- `draw_ui_task` — redraws OLED every 10ms
- `test` — outputs 1kHz sine on DAC

## Build
```
idf.py set-target esp32
idf.py build flash monitor
```

## Known issues
- `sample2` unused
- `READ_PERIOD` (`1/SAMPLE_FRQ`) always 0
- Timer comment says 1ms, actual alarm is 10us (`ALARM_TICKS=10` @ 1MHz)