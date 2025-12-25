#ifndef CONFIG_H
#define CONFIG_H

// general
#define MSerial Serial1

// sensors
#define MAX_DEVICES 3

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "secrets.h file is missing"
#endif

#ifndef SECRET_WIFI_SSID
#error "please define SECRET_WIFI_SSID"
#endif

#ifndef SECRET_WIFI_PASS
#error "please define SECRET_WIFI_PASS"
#endif

#ifndef SECRET_MQTT_HOST
#error "please define SECRET_MQTT_HOST"
#endif

#ifndef SECRET_MQTT_PORT
#error "please define SECRET_MQTT_PORT"
#endif

#ifndef SECRET_MQTT_USER
#error "please define SECRET_MQTT_USER"
#endif

#ifndef SECRET_MQTT_PASS
#error "please define SECRET_MQTT_PASS"
#endif

#endif
