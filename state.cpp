#include "state.h"
#include "config.h"
#include "kbd.h"
#include "sensor.h"
#include "storage.h"
#include "terminal.h"
#include <Arduino.h>

static state_t state;

void state_init(storage_t *storage) {
  for (int i = 0; i < SENSORS; ++i) {
    state.sensors[i].adc_value = 0;
    state.sensors[i].adc_gpio = ADC_GPIOS[i];
    state.sensors[i].pump_gpio = PUMP_GPIOS[i];
    state.sensors[i].threshold = storage->threshold[i];
    state.sensors[i].pumping = false;
  }

  state.selected = 0;
}

const moisture_sensor_t *state_get_sensors() { return state.sensors; }

void state_set_sensor_adc_value(int sensor, uint16_t value) {
  state.sensors[sensor].adc_value = value;
}

int state_get_selected() { return state.selected; }

void state_set_selected(int sensor) { state.selected = sensor; }

void state_handle_short_input(int key) {
  int pin = KBD_PIN_MAP[key];
  MSerial.printf("short press key=%d pin=%d\r\n", key, pin);

  if (!state.display_active) {
    terminal_toogle_display_power();
    return;
  }

  if (pin == KBD_PIN_MAP[KEY_DOWN]) {
    if (state.selected < OPT_MAX - 1)
      state.selected++;
  } else if (pin == KBD_PIN_MAP[KEY_UP]) {
    if (state.selected > 0) {
      state.selected--;
    }
  } else if (pin == KBD_PIN_MAP[KEY_LEFT] || pin == KBD_PIN_MAP[KEY_RIGHT]) {
    if (state.selected == OPT_POWEROFF) {
      terminal_toogle_display_power();
      return;
    }

    moisture_sensor_t *sensor = &state.sensors[state.selected];
    int step = pin == KBD_PIN_MAP[KEY_LEFT] ? (-THRESHOLD_SHORT_STEP)
                                            : (THRESHOLD_SHORT_STEP);

    int new_threshold = sensor->threshold + step;
    if (new_threshold < 0)
      new_threshold = 0;
    if (new_threshold > MAX_SENSOR_VALUE)
      new_threshold = MAX_SENSOR_VALUE;

    sensor->threshold = new_threshold;
    storage.threshold[state.selected] = new_threshold;
    storage_save();
  }
}

void state_handle_long_repeat(int key) {
  int pin = KBD_PIN_MAP[key];
  MSerial.printf("long press repeat key=%d pin=%d\r\n", key, pin);

  if (!state.display_active) {
    terminal_toogle_display_power();
    return;
  }
}

void state_set_pumping_state(int sensor, bool pumping) {
  state.sensors[sensor].pumping = pumping;
}

void state_set_display_active(bool active) { state.display_active = active; }

bool state_get_display_active() { return state.display_active; }
