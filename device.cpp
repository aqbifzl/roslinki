#include "device.h"
#include "mqtt.h"
#include <Arduino.h>

void Device::updateSensorValue() { sensorValue = analogRead(sensorPin); }

void Device::loop() {
  bool should_pump = sensorValue > threshold;
  if (should_pump && !pumping) {
    wateringStart();
  } else if (!should_pump && pumping) {
    wateringStop();
  }
}

void Device::wateringStart() {
  Mqtt::instance()->sendPumpState(id, 1);
  digitalWrite(pumpPin, HIGH);
  pumping = true;
}

void Device::wateringStop() {
  Mqtt::instance()->sendPumpState(id, 0);
  digitalWrite(pumpPin, LOW);
  pumping = false;
}
