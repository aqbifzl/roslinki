#ifndef STATE_H
#define STATE_H

#include "config.h"
#include "sensor.h"
#include "storage.h"
#include <cstdint>

enum { OPT_SENSOR1, OPT_SENSOR2, OPT_POWEROFF, OPT_MAX };

typedef struct {
  moisture_sensor_t sensors[SENSORS];
  int selected;
  bool display_active;
} state_t;

void state_init(storage_t *storage);

const moisture_sensor_t *state_get_sensors();
void state_set_sensor_adc_value(int sensor, uint16_t value);

int state_get_selected();
void state_set_selected(int sensor);

void state_handle_short_input(int key);
void state_handle_long_repeat(int key);

void state_set_pumping_state(int sensor, bool pumping);

void state_set_display_active(bool active);
bool state_get_display_active();

#endif
