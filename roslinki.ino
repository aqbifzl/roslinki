#include "Wire.h"
#include "config.h"
#include "hardware/i2c.h"
#include "kbd.h"
#include "state.h"
#include "storage.h"
#include "terminal.h"
#include "watering.h"
#include <Arduino.h>

unsigned long last_update_ms = 0;

void setup() {
  MSerial.begin(115200);
  while (!MSerial) {
  }
  delay(1000);

  if (!storage_load()) {
    MSerial.println("restoring data has failed");
  }
  state_init(&storage);
  terminal_init();
  kbd_init();
  kbd_set_long_repeat_callback(state_handle_long_repeat);
  kbd_set_short_press_callback(state_handle_short_input);
  watering_init();

  MSerial.println("ROSLINKI");
}

void loop() {
  kbd_handle_input();
  unsigned long now = millis();

  if (now - last_update_ms >= SCAN_INTERVAL_MS) {
    last_update_ms = now;
    sensor_update_values();
    watering_handle_state();
  }

  terminal_redraw_all();
}
