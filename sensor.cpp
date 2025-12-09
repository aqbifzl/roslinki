#include "config.h"
#include "hardware/platform_defs.h"
#include "state.h"
#include <Arduino.h>

void sensor_update_values() {
  for (int i = 0; i < SENSORS; ++i) {
    state_set_sensor_adc_value(i, analogRead(ADC_BASE_PIN + i));
  }
}
