#ifndef MOISTURE_SENSOR_H
#define MOISTURE_SENSOR_H

typedef struct {
  int adc_gpio;
  int threshold;
  int pump_gpio;
  int adc_value;
  int pumping;
} moisture_sensor_t;

void sensor_update_values();

#endif
