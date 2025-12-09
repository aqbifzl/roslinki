#include "Arduino.h"
#include "config.h"
#include "sensor.h"
#include "state.h"
#include <cstdint>

void watering_init() {
  const moisture_sensor_t *sensors = state_get_sensors();
  int gpio;
  for (int i = 0; i < SENSORS; ++i) {
    gpio = sensors[i].pump_gpio;
    pinMode(gpio, OUTPUT);
    digitalWrite(gpio, LOW);
  }
}

void watering_start(int sensor) {
  int gpio = state_get_sensors()[sensor].pump_gpio;
  digitalWrite(gpio, HIGH);
  state_set_pumping_state(sensor, true);
}

void watering_stop(int sensor) {
  int gpio = state_get_sensors()[sensor].pump_gpio;
  digitalWrite(gpio, LOW);
  state_set_pumping_state(sensor, false);
}

void watering_handle_state() {
  const moisture_sensor_t *sensors = state_get_sensors();

  for (int i = 0; i < SENSORS; ++i) {
    const moisture_sensor_t *sensor = &sensors[i];
    bool should_pump = sensor->adc_value > sensor->threshold;
    if (should_pump && !sensor->pumping)
      watering_start(i);
    else if (!should_pump && sensor->pumping)
      watering_stop(i);
  }
}
