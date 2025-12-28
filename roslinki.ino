#include "config.h"
#include "mqtt.h"
#include "state.h"

void setup() {
  MSerial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Mqtt::instance()->init();
  digitalWrite(LED_BUILTIN, LOW);

  analogReadResolution(12);
  pinMode(26, INPUT);
  pinMode(27, INPUT);
  pinMode(28, INPUT);
}

uint64_t lastSensorRead = 0;

void loop() {
  if (!Mqtt::instance()->isInited() || !Mqtt::instance()->reconnect())
    return;

  Mqtt::instance()->readPackets();

  Device *devices = State::instance()->getDevices();
  const int count = State::instance()->getDeviceCount();

  if (millis() - lastSensorRead > State::instance()->getScanInterval()) {
    for (int i = 0; i < count; ++i) {
      devices[i].updateSensorValue();
    }
    Mqtt::instance()->sendSensorsValues();

    lastSensorRead = millis();
  }

  for (int i = 0; i < count; ++i) {
    devices[i].loop();
  }
}
