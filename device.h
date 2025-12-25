#ifndef DEVICE_H
#define DEVICE_H

struct Device {
  int id;
  int sensorPin;
  int threshold;
  int pumpPin;
  int sensorValue;
  bool pumping;

  void updateSensorValue();
  void loop();
  void wateringStart();
  void wateringStop();
};

#endif
