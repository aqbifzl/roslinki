#include "mqtt.h"
#include "config.h"
#include "secrets.h"
#include "state.h"
#include "wl_definitions.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <cstring>
#include <time.h>

Mqtt::Mqtt() : client(espClient) {}

void Mqtt::mqttCallback(char *topic, uint8_t *payload, unsigned int length) {
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    MSerial.print("JSON error: ");
    MSerial.println(error.c_str());
    return;
  }

  if (strcmp(topic, Mqtt::configTopic) == 0) {
    const int newInterval = doc["config"]["scan_interval"];
    JsonArray devicesArray = doc["devices"];
    State::instance()->updateSettings(newInterval, devicesArray);

    MSerial.println("Settings has been updated");
  }
}

bool Mqtt::isInited() { return inited; }

bool Mqtt::reconnect() {
  if (client.connected())
    return true;

  digitalWrite(LED_BUILTIN, HIGH);

  unsigned long now = millis();
  if (now - lastReconnectAttempt < 5000) {
    return false;
  }

  lastReconnectAttempt = now;

  MSerial.print("connecting to MQTT...");

  if (client.connect(mqttClientId, SECRET_MQTT_USER, SECRET_MQTT_PASS)) {
    MSerial.println(" connected!");
    client.subscribe(configTopic);
    digitalWrite(LED_BUILTIN, LOW);
    return true;
  } else {
    int state = client.state();
    MSerial.print("failed, rc=");
    MSerial.print(state);

    switch (state) {

    case -4:
      MSerial.print(" (MQTT_CONNECTION_TIMEOUT)");
      break;
    case -3:
      MSerial.print(" (MQTT_CONNECTION_LOST)");
      break;
    case -2:
      MSerial.print(" (MQTT_CONNECT_FAILED)");
      break;
    case -1:
      MSerial.print(" (MQTT_DISCONNECTED)");
      break;
    case 0:
      MSerial.print(" (MQTT_CONNECTED)");
      break;
    case 1:
      MSerial.print(" (MQTT_CONNECT_BAD_PROTOCOL)");
      break;
    case 2:
      MSerial.print(" (MQTT_CONNECT_BAD_CLIENT_ID)");
      break;
    case 3:
      MSerial.print(" (MQTT_CONNECT_UNAVAILABLE)");
      break;
    case 4:
      MSerial.print(" (MQTT_CONNECT_BAD_CREDENTIALS)");
      break;
    case 5:
      MSerial.print(" (MQTT_CONNECT_UNAUTHORIZED)");
      break;
    }

    if (state == -2) {
      char buf[128];
      espClient.getLastSSLError(buf, 128);

      if (strlen(buf) > 0) {
        MSerial.print(" SSL Details: ");
        MSerial.println(buf);
      } else {
        MSerial.println();
      }
    } else {
      MSerial.println();
    }
  }

  return false;
}

void Mqtt::readPackets() {
  if (!client.connected()) {
    return;
  }
  client.loop();
}

void Mqtt::sendPumpState(int id, bool value) {
  if (!client.connected())
    return;

  StaticJsonDocument<128> doc;

  doc["plant_id"] = id;
  doc["action"] = value ? 1 : 0;

  char jsonBuffer[128];
  serializeJson(doc, jsonBuffer);

  MSerial.printf("Sending pump logs: %s\n", jsonBuffer);
  client.publish(pumpLogsTopic, jsonBuffer);
}

void Mqtt::sendSensorsValues() {
  if (!client.connected())
    return;

  State *state = State::instance();
  if (!state->isInited())
    return;

  Device *devices = state->getDevices();
  int count = state->getDeviceCount();

  for (int i = 0; i < count; i++) {
    if (devices[i].id > 0) {
      StaticJsonDocument<128> doc;

      doc["plant_id"] = devices[i].id;
      doc["value"] = devices[i].sensorValue;

      char jsonBuffer[128];
      serializeJson(doc, jsonBuffer);

      MSerial.printf("Sending sensor logs: %s\n", jsonBuffer);
      client.publish(sensorLogsTopic, jsonBuffer);
    }
  }
}

void Mqtt::init() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);

  int tries = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    MSerial.print(".");
    ++tries;

    if (tries > 20) {
      MSerial.printf("\nCouldn't connect to %s, retrying", SECRET_WIFI_SSID);

      WiFi.disconnect();
      delay(100);
      WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);

      tries = 0;
    }
  }

  MSerial.println("Connected to WiFi!");

  MSerial.printf("IP: %s\n", WiFi.localIP().toString().c_str());

  MSerial.print("Setting NTP for TLS...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  int i = 0;
  while (now < 1000000000 && i < 20) {
    delay(500);
    MSerial.print(".");
    now = time(nullptr);
    ++i;
  }
  MSerial.println(" The time is set");

  espClient.setInsecure();
  espClient.setTimeout(2000);

  client.setServer(SECRET_MQTT_HOST, SECRET_MQTT_PORT);
  client.setCallback(mqttCallback);
  client.setBufferSize(2048);

  reconnect();

  inited = true;
}
