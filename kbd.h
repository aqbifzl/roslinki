#ifndef KBD_H
#define KBD_H

#include "config.h"
#include <Arduino.h>

void kbd_init();
void kbd_handle_input();
void kbd_set_short_press_callback(kbd_callback_t cb);
void kbd_set_long_repeat_callback(kbd_callback_t cb);

#endif // !KBD_H
