#ifndef STATE_H
#define STATE_H

#include <ArduinoJson.h>

#include "config.h"
#include "device.h"

class State {
private:
  Device devices[MAX_DEVICES];
  int deviceCount = 0;
  int scanInterval = 5000;
  bool inited = false;

public:
  static State *instance() {
    static State s;
    return &s;
  }

  void updateSettings(int newInterval, const JsonArray &newDevices);
  int getDeviceCount();
  Device *getDevices();
  int getScanInterval();
  bool isInited();
};

#endif
