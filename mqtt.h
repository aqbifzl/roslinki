#ifndef MQTT_H
#define MQTT_H

#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <cstdint>

class Mqtt {
private:
  bool inited = false;
  WiFiClientSecure espClient;
  PubSubClient client;
  uint64_t lastReconnectAttempt = 0;

  static constexpr char mqttClientId[] = "RP2350";
  static constexpr char configTopic[] = "roslinki/config";
  static constexpr char pumpLogsTopic[] = "roslinki/pump_logs";
  static constexpr char sensorLogsTopic[] = "roslinki/sensor_logs";

  Mqtt();
  static void mqttCallback(char *topic, uint8_t *payload, unsigned int length);

public:
  static Mqtt *instance() {
    static Mqtt w;
    return &w;
  }

  void init();
  bool reconnect();
  void readPackets();
  bool isInited();

  void sendSensorsValues();
  void sendPumpState(int id, bool value);
};

#endif
