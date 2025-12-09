#ifndef CONFIG_H
#define CONFIG_H

// general
#define MSerial Serial1

// display

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#define DISPLAY_SDA 16
#define DISPLAY_SCL 17

#define DISPLAY_ADDR 0x3D

// sensors
#define MAX_SENSORS 3
#define SENSORS 2

#define DEFAULT_MOISTURE_THRESHOLD 800
#define SCAN_INTERVAL_MS 100
#define SAVE_STORAGE_INTERVAL_MS 5000
#define MAX_SENSOR_VALUE 1023
#define THRESHOLD_SHORT_STEP 100

static const int ADC_GPIOS[SENSORS] = {26, 27};
static const int PUMP_GPIOS[SENSORS] = {2, 3};

// keyboard
#define LONG_PRESS_START_MS 1000UL
#define LONG_PRESS_REPEAT_MS 100UL
#define DEBOUNCE_MS 40UL

enum kbd_pins_func { KEY_LEFT = 0, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_MAX };
typedef void (*kbd_callback_t)(int btn);
#define SOURCE_PIN 5
static const int KBD_PIN_MAP[KEY_MAX] = {6, 8, 10, 12};

#endif
