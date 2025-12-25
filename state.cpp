#include "state.h"
#include "config.h"
#include <Arduino.h>
#include <ArduinoJson.h>

void State::updateSettings(int newInterval, const JsonArray &newDevices) {
  this->scanInterval = newInterval;

  Device devTmp[MAX_DEVICES];
  int devCounter = 0;

  for (JsonObject devJson : newDevices) {
    if (devCounter >= MAX_DEVICES)
      break;

    Device *newDev = &devTmp[devCounter];

    newDev->id = devJson["id"];
    newDev->sensorPin = devJson["sensor_pin"];
    newDev->pumpPin = devJson["pump_pin"];
    newDev->threshold = devJson["threshold"];

    newDev->pumping = false;
    newDev->sensorValue = 0;

    bool foundOld = false;

    for (int i = 0; i < this->deviceCount; ++i) {
      if (this->devices[i].id == newDev->id) {
        newDev->pumping = this->devices[i].pumping;
        newDev->sensorValue = this->devices[i].sensorValue;

        // set old pin to low if changed
        if (this->devices[i].pumpPin != newDev->pumpPin) {
          pinMode(this->devices[i].pumpPin, OUTPUT);
          digitalWrite(this->devices[i].pumpPin, LOW);
        }

        foundOld = true;
        break;
      }
    }

    pinMode(newDev->pumpPin, OUTPUT);
    digitalWrite(newDev->pumpPin, newDev->pumping ? HIGH : LOW);

    ++devCounter;
  }

  for (int i = 0; i < this->deviceCount; ++i) {
    bool stillExists = false;
    for (int j = 0; j < devCounter; ++j) {
      if (devTmp[j].id == this->devices[i].id) {
        stillExists = true;
        break;
      }
    }

    if (!stillExists) {
      pinMode(this->devices[i].pumpPin, OUTPUT);
      digitalWrite(this->devices[i].pumpPin, LOW);
    }
  }

  memcpy(this->devices, devTmp, sizeof(Device) * devCounter);
  this->deviceCount = devCounter;
  inited = true;
}

int State::getDeviceCount() { return deviceCount; }

Device *State::getDevices() { return devices; }

int State::getScanInterval() { return scanInterval; }

bool State::isInited() { return inited; }
