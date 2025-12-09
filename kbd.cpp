#include "kbd.h"
#include "HardwareSerial.h"
#include "config.h"
#include <Arduino.h>
#include <cmath>

static volatile uint32_t irq_flags_mask = 0;

static bool pressed[KEY_MAX] = {false};
static bool long_started[KEY_MAX] = {false};
static unsigned long press_start[KEY_MAX] = {0};
static unsigned long last_change[KEY_MAX] = {0};
static unsigned long last_repeat_time[KEY_MAX] = {0};

static kbd_callback_t short_cb = nullptr;
static kbd_callback_t long_repeat_cb = nullptr;

static void gpio_irq_trampoline(void *arg) {
  const int gpio = *(const int *)arg;

  for (uint8_t i = 0; i < KEY_MAX; ++i) {
    if (gpio == KBD_PIN_MAP[i]) {
      noInterrupts();
      irq_flags_mask = (1u << i);
      interrupts();
      break;
    }
  }
}

void kbd_init() {
  pinMode(SOURCE_PIN, OUTPUT);
  digitalWrite(SOURCE_PIN, HIGH);

  for (uint8_t i = 0; i < KEY_MAX; ++i) {
    int pin = KBD_PIN_MAP[i];

    pinMode(pin, INPUT_PULLDOWN);
    attachInterruptParam(pin, gpio_irq_trampoline, CHANGE,
                         (void *)&KBD_PIN_MAP[i]);

    // initialize state
    pressed[i] = false;
    long_started[i] = false;
    press_start[i] = 0;
    last_change[i] = 0;
    last_repeat_time[i] = 0;
  }

  noInterrupts();
  irq_flags_mask = 0;
  interrupts();
}

void kbd_handle_input() {
  unsigned long now = millis();

  // grab and clear pending flags
  uint32_t pending;
  noInterrupts();
  pending = irq_flags_mask;
  irq_flags_mask = 0;
  interrupts();

  // process all pending IRQs
  for (uint8_t i = 0; i < KEY_MAX; ++i) {
    if (!(pending & (1u << i)))
      continue;

    bool level = digitalRead(KBD_PIN_MAP[i]);

    // debouncing
    if (now - last_change[i] < DEBOUNCE_MS) {
      // skip, but update last_change so that another edge restarts debounce
      // timer
      last_change[i] = now;
      continue;
    }

    last_change[i] = now;

    if (level) {
      // pressed (adjust depending on pull direction: HIGH means pressed here)
      pressed[i] = true;
      long_started[i] = false;
      press_start[i] = now;
      last_repeat_time[i] = 0;
    } else {
      // released
      if (pressed[i]) {
        if (!long_started[i]) {
          if (short_cb)
            short_cb(i);
        } else {
          // long press released (optional callback)
        }
      }

      // reset state
      pressed[i] = false;
      long_started[i] = false;
      press_start[i] = 0;
      last_repeat_time[i] = 0;
    }
  }

  // handle long press repeat for keys that remain pressed
  for (uint8_t i = 0; i < KEY_MAX; ++i) {
    if (!pressed[i])
      continue;

    unsigned long held = now - press_start[i];

    if (!long_started[i]) {
      if (held >= LONG_PRESS_START_MS) {
        long_started[i] = true;
        last_repeat_time[i] = now;
        // optional: long press start callback
      }
    } else {
      if (now - last_repeat_time[i] >= LONG_PRESS_REPEAT_MS) {
        last_repeat_time[i] = now;
        if (long_repeat_cb)
          long_repeat_cb(i);
      }
    }
  }
}

void kbd_set_short_press_callback(kbd_callback_t cb) { short_cb = cb; }
void kbd_set_long_repeat_callback(kbd_callback_t cb) { long_repeat_cb = cb; }
